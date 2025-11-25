#include <stdio.h>
#include "codegen.h"
#include <string.h>

// --- 简易符号表 ---
typedef struct {
    char* name;
    int stack_offset; // 变量在栈上的偏移量
} Symbol;

Symbol symbol_table[100]; // 假设最多100个局部变量
int symbol_count = 0;

// 在符号表中查找一个变量
Symbol* find_symbol(char* name) {
    for (int i = 0; i < symbol_count; i++) {
        if (strcmp(symbol_table[i].name, name) == 0) {
            return &symbol_table[i];
        }
    }
    return NULL;
}

// 声明我们将要使用的递归函数
static void codegen_node(ASTNode* node);

// --- AST 节点代码生成函数 ---

// 为 "Program" 节点生成代码
static void codegen_program(ProgramNode* node) {
    // 汇编程序的起点
    printf(".intel_syntax noprefix\n"); // 使用更常见的 Intel 语法（可选，但对初学者更友好）

    printf(".globl _start\n"); // 声明 _start 为全局入口点
    printf("_start:\n");
    printf("  call main\n");    // 调用主角 main 函数

    // --- main 返回后，处理退出的逻辑 ---
    printf("  mov rdi, rax\n"); // 将 main 的返回值 (在rax) 放入 rdi，作为 exit 的参数
    printf("  mov rax, 60\n");  // 将 exit 的系统调用号 (60) 放入 rax
    printf("  syscall\n");     // 调用内核，退出程序

    // --- 分隔线，下面是我们的函数实现 ---
    printf("\n");
    
    // 遍历并为程序中的每个声明（函数）生成代码
    for (int i = 0; i < node->count; i++) {
        codegen_node((ASTNode*)node->declarations[i]);
    }
}

// 为 "Function Declaration" 节点生成代码
static void codegen_function_declaration(FunctionDeclarationNode* node) {
    // 声明一个全局可链接的函数标签
    // printf(".globl %s\n", node->name);
    // 函数不再需要是 .globl，因为只有 _start 是外部可见的
    printf("%s:\n", node->name);    // 定义函数标签

    // --- 函数序言 (Prologue) ---
    printf("  push rbp\n");
    printf("  mov rbp, rsp\n");

    // TODO: 在这里为所有局部变量分配栈空间
    // 1. 遍历函数体(node->body)，找出有多少个变量声明。
    // 2. 为每个变量计算偏移量 (第一个是 -4, 第二个是 -8, ...)
    // 3. 将变量信息存入符号表。
    // 4. 计算总共需要的栈空间 (比如 2 个 int 变量需要 8 字节)。
    // 5. 生成 sub rsp, <total_size> 指令。

    // 1. 遍历函数体(node->body)的所有语句，找出有多少个变量声明。
    symbol_count = 0;
    int var_count = 0;
    for (int i = 0; i < node->body->count; i++) {
        if (node->body->statements[i]->type == NODE_VAR_DECL) {
            var_count++;
            // 2. 为每个变量计算偏移量并存入符号表。
            VarDeclNode* var_decl = (VarDeclNode*)node->body->statements[i];
            symbol_table[symbol_count].name = var_decl->name;
            // 第一个变量偏移量是 -8 (我们的int是8字节对齐的)，第二个是 -16，以此类推。
            symbol_table[symbol_count].stack_offset = (var_count) * 8;
            symbol_count++;
        }
    }

    // 3. 计算总共需要的栈空间，并确保16字节对齐 (x86-64 ABI要求)
    // 计算所有变量需要的总字节数
    int total_bytes_needed = var_count * 8;
    // 向上取整到最近的 16 的倍数
    // 这是一个经典算法: (value + alignment - 1) / alignment * alignment
    int stack_size_to_allocate = (total_bytes_needed + 15) / 16 * 16;
    if (stack_size_to_allocate == 0 && var_count > 0) {
        stack_size_to_allocate = 16;
    }

    if (stack_size_to_allocate > 0) {
        printf("  sub rsp, %d\n", stack_size_to_allocate);
    }
    
    // 为函数体（一个代码块）生成代码
    codegen_node((ASTNode*)node->body);

    // 注意：函数尾声 (epilogue) 将在 return 语句中生成，
    // 因为 return 之后就不再有其他代码了。
}

// 为 "Variable Declaration" 节点生成代码
static void codegen_variable_declaration(VarDeclNode* node) {
    // TODO:
    // 1. 为等号右边的表达式生成代码 (调用 codegen_node(node->initial_value))
    //    执行后，表达式的结果会存放在 eax 寄存器中。
    // 2. 从符号表中查找变量的偏移量。
    // 3. 生成 mov 指令，将 eax 中的值存到栈上对应的位置。
    //    printf("  mov DWORD PTR [rbp-%d], eax\n", offset);

    // 1. 为等号右边的表达式生成代码 (调用 codegen_node)。
    //    执行后，表达式的结果会存放在 eax 寄存器中。
    codegen_node(node->initial_value);

    // 2. 从符号表中查找变量的偏移量。
    Symbol* symbol = find_symbol(node->name);
    if (symbol == NULL) {
        fprintf(stderr, "Codegen Error: Undefined variable '%s'\n", node->name);
        exit(1);
    }

    // 3. 生成 mov 指令，将 eax 中的值存到栈上对应的位置。
    //    e.g., mov DWORD PTR [rbp-8], eax
    printf("  mov [rbp-%d], eax\n", symbol->stack_offset);
}

// 为 "Identifier" (变量使用) 节点生成代码
static void codegen_identifier(IdentifierNode* node) {
    // TODO:
    // 1. 从符号表中查找变量的偏移量。
    // 2. 生成 mov 指令，将栈上变量的值加载到 eax 寄存器中。
    //    printf("  mov eax, DWORD PTR [rbp-%d]\n", offset);

    // 1. 从符号表中查找变量的偏移量。
    Symbol* symbol = find_symbol(node->name);
    if (symbol == NULL) {
        fprintf(stderr, "Codegen Error: Undefined variable '%s'\n", node->name);
        exit(1);
    }

    // 2. 生成 mov 指令，将栈上变量的值加载到 eax 寄存器中。
    //    e.g., mov eax, DWORD PTR [rbp-8]
    printf("  mov eax, [rbp-%d]\n", symbol->stack_offset);
}

// 为 "Block Statement" 节点生成代码
static void codegen_block_statement(BlockStatementNode* node) {
    // 依次为代码块中的每个语句生成代码
    for (int i = 0; i < node->count; i++) {
        codegen_node(node->statements[i]);
    }
}

// 为 "Return Statement" 节点生成代码
static void codegen_return_statement(ReturnStatementNode* node) {
    // 1. 为要返回的表达式生成代码，执行后，结果会在 eax 中。
    codegen_node(node->argument);

    // 2. 生成函数尾声 (Epilogue) 和返回指令。
    //    注意：这里我们简单地用 mov rsp, rbp 来恢复栈指针，
    //    这在没有动态栈分配（如alloca）的情况下是安全的。
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret\n");
}

// 为 "Numeric Literal" 节点生成代码
static void codegen_numeric_literal(NumericLiteralNode* node) {
    // TODO (2/2): 为数字字面量生成代码

    // 思考一下：当 codegen_return_statement 调用我们时，我们的任务是什么？
    // 任务是把这个数字的值放入返回值寄存器 %eax 中。
    // 使用 movl 指令来完成这个任务。
    // 提示：node->value 是一个字符串，所以你需要使用 %s 来打印它。
    printf("  mov rax, %s\n", node->value);
}

// 为 "Binary Operation" 节点生成代码
static void codegen_binary_op(BinaryOpNode* node) {
    // 1. 生成右子树的代码 (计算 B)
    codegen_node(node->right);
    //    现在 B 的结果在 eax 中

    // 2. 将 B 的结果压入栈中保存
    printf("  push rax\n");

    // 3. 生成左子树的代码 (计算 A)
    codegen_node(node->left);
    //    现在 A 的结果在 eax 中

    // 4. 将 B 的结果从栈中弹出到 rdi
    printf("  pop rdi\n");

    // 5. 根据操作符，生成对应的汇编指令
    switch (node->op) {
        case TOKEN_PLUS:
            printf("  add rax, rdi\n");
            break;
        case TOKEN_MINUS:
            printf("  sub rax, rdi\n");
            break;
        default:
            fprintf(stderr, "Codegen: Unsupported binary operator\n");
            exit(1);
    }
}

/**
 * @brief 递归的 AST 节点访问者函数。
 * 
 * 根据节点的类型，分发到相应的 codegen_... 函数。
 * @param node 当前要生成代码的 AST 节点。
 */
static void codegen_node(ASTNode* node) {
    if (node == NULL) return;

    switch (node->type) {
        case NODE_PROGRAM:
            codegen_program((ProgramNode*)node);
            break;
        case NODE_FUNCTION_DECL:
            codegen_function_declaration((FunctionDeclarationNode*)node);
            break;
        case NODE_BLOCK_STATEMENT:
            codegen_block_statement((BlockStatementNode*)node);
            break;
        case NODE_RETURN_STATEMENT:
            codegen_return_statement((ReturnStatementNode*)node);
            break;
        case NODE_NUMERIC_LITERAL:
            codegen_numeric_literal((NumericLiteralNode*)node);
            break;
        case NODE_VAR_DECL:
            codegen_variable_declaration((VarDeclNode*)node);
            break;
        case NODE_IDENTIFIER:
            codegen_identifier((IdentifierNode*)node);
            break;
        case NODE_BINARY_OP:
            codegen_binary_op((BinaryOpNode*)node);
            break;
        default:
            fprintf(stderr, "Codegen Error: Unknown AST node type %d\n", node->type);
            exit(1);
    }
}

// --- 代码生成器主入口 ---
void codegen(ASTNode* root) {
    codegen_node(root);
}