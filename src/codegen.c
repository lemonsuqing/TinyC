#include <stdio.h>
#include "codegen.h"
#include <string.h>

// 一个全局计数器，用于生成唯一的标签
static int label_counter = 0;

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
static void gen_lvalue(ASTNode* node);

// 辅助：重置符号表
void reset_symbol_table() {
    symbol_count = 0;
}

// --- AST 节点代码生成函数 ---

// 为 "Program" 节点生成代码
static void codegen_program(ProgramNode* node) {
    // 汇编程序的起点
    printf(".intel_syntax noprefix\n"); // 使用更常见的 Intel 语法（可选，但对初学者更友好）

    printf(".data\n"); 
    
    for (int i = 0; i < node->count; i++) {
        ASTNode* child = node->declarations[i];
        // 只有变量声明才在 .data 段处理
        if (child->type == NODE_VAR_DECL) {
            VarDeclNode* var = (VarDeclNode*)child;
            
            // 生成标签: "g_val:"
            printf("%s:\n", var->name);
            
            // 生成初始值
            if (var->initial_value != NULL && var->initial_value->type == NODE_NUMERIC_LITERAL) {
                // 如果有初始值: .quad 10
                NumericLiteralNode* val = (NumericLiteralNode*)var->initial_value;
                printf("  .quad %s\n", val->value);
            } else {
                // 如果没有初始值: .quad 0
                printf("  .quad 0\n");
            }
        }
    }
    printf("\n");

    printf(".text\n");
    printf(".globl _start\n"); // 声明 _start 为全局入口点
    printf("_start:\n");
    printf("  call main\n");    // 调用主角 main 函数

    // --- main 返回后，处理退出的逻辑 ---
    printf("  mov rdi, rax\n"); // 将 main 的返回值 (在rax) 放入 rdi，作为 exit 的参数
    printf("  mov rax, 60\n");  // 将 exit 的系统调用号 (60) 放入 rax
    printf("  syscall\n");     // 调用内核，退出程序

    // --- 分隔线，下面是我们的函数实现 ---
    printf("\n");
    
    // 遍历并生成函数代码
    for (int i = 0; i < node->count; i++) {
        ASTNode* child = node->declarations[i];
        // 只有函数声明才在这里处理
        if (child->type == NODE_FUNCTION_DECL) {
            codegen_node(child);
        }
    }
}

// 为 "Function Declaration" 节点生成代码
static void codegen_function_declaration(FunctionDeclarationNode* node) {
    reset_symbol_table();
    // 声明一个全局可链接的函数标签
    // printf(".globl %s\n", node->name);
    // 函数不再需要是 .globl，因为只有 _start 是外部可见的
    printf("%s:\n", node->name);    // 定义函数标签

    // --- 函数序言 (Prologue) ---
    printf("  push rbp\n");
    printf("  mov rbp, rsp\n");

    // --- 1. 计算栈空间 ---
    // 包含参数(node->args) 和 函数体内的变量(node->body中的VarDecl)
    // 简单起见，我们遍历所有参数和所有语句，统统加到符号表里。
    
    // 1.1 先处理参数：把参数注册到符号表
    // 必须按照寄存器顺序：rdi, rsi, rdx, rcx, r8, r9
    char* arg_regs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
    
    int current_stack_offset = 0;

    for (int i = 0; i < node->arg_count; i++) {
        VarDeclNode* param = (VarDeclNode*)node->args[i];
        
        // 分配栈位置
        current_stack_offset += 8;
        symbol_table[symbol_count].name = param->name;
        symbol_table[symbol_count].stack_offset = current_stack_offset;
        symbol_count++;
    }

    // 1.2 再处理函数体内的局部变量
    // (这部分逻辑和你之前写的差不多，遍历 body 找 VarDecl)
    for (int i = 0; i < node->body->count; i++) {
        if (node->body->statements[i]->type == NODE_VAR_DECL) {
            VarDeclNode* var = (VarDeclNode*)node->body->statements[i];
            current_stack_offset += 8;
            symbol_table[symbol_count].name = var->name;
            symbol_table[symbol_count].stack_offset = current_stack_offset;
            symbol_count++;
        }
    }

    // 1.3 分配栈空间 (16字节对齐)
    int stack_size = (current_stack_offset + 15) / 16 * 16;
    if (stack_size > 0) printf("  sub rsp, %d\n", stack_size);

    // --- 2. 将寄存器中的参数值，搬运到栈里 ---
    // 因为参数是局部变量，代码中会通过 [rbp-N] 访问它们。
    // 但值现在在 rdi, rsi... 里，所以要搬进去。
    for (int i = 0; i < node->arg_count; i++) {
        VarDeclNode* param = (VarDeclNode*)node->args[i];
        // 查找它在栈里的位置
        Symbol* sym = find_symbol(param->name); 
        // 生成: mov [rbp-8], rdi
        printf("  mov [rbp-%d], %s\n", sym->stack_offset, arg_regs[i]);
    }

    // --- 3. 生成函数体代码 ---
    codegen_node((ASTNode*)node->body);
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
    printf("  mov [rbp-%d], rax\n", symbol->stack_offset);
}

// 为 "Identifier" (变量使用) 节点生成代码
static void codegen_identifier(IdentifierNode* node) {
    // 1. 先尝试在局部符号表中查找
    Symbol* symbol = find_symbol(node->name);
    
    if (symbol != NULL) {
        // 局部变量 -> 从栈加载
        printf("  mov rax, [rbp-%d]\n", symbol->stack_offset);
    } else {
        // 全局变量 -> 从数据段加载
        printf("  mov rax, [rip + %s]\n", node->name);
    }
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
    if (node->op == TOKEN_ASSIGN) {
        // 1. 计算左边的地址 (L-value)，压栈
        // 比如左边是 x，rax 就得到 &x
        // 比如左边是 *p，rax 就得到 p 的值
        gen_lvalue(node->left);
        printf("  push rax\n"); // 保存地址

        // 2. 计算右边的值 (R-value)
        codegen_node(node->right);
        // 此时 rax 是右边的值 (例如 20)

        // 3. 赋值
        printf("  pop rdi\n");      // 弹出的地址放到 rdi
        printf("  mov [rdi], rax\n"); // 把值写入该地址
        return;
    }

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
        case TOKEN_STAR:
            printf("  imul rax, rdi\n"); // 有符号乘法: rax = rax * rdi
            break;
        case TOKEN_SLASH:
            // 除法比较特殊：
            // 被除数在 rax 中 (来自 pop rdi 前的计算结果, 但这里 rax 是左操作数, rdi 是右操作数? 等等)
            
            // 让我们理一下栈：
            // 1. 右操作数 B 计算完 -> push
            // 2. 左操作数 A 计算完 -> 在 rax 中
            // 3. pop rdi -> B 在 rdi 中
            // 此时：rax = A (被除数), rdi = B (除数)

            // idiv 指令是用 rdx:rax (128位) 除以操作数。
            // 我们只有 64 位的 rax，所以需要把 rax 的符号位扩展到 rdx 中。
            // cqo 指令就是做这个的 (Convert Quad-word to Oct-word)。
            printf("  cqo\n"); 
            printf("  idiv rdi\n"); // rax = rdx:rax / rdi
            break;
        case TOKEN_EQ:
        case TOKEN_NEQ:
        case TOKEN_LT:
        case TOKEN_LE:
        case TOKEN_GT:
        case TOKEN_GE:
            printf("  cmp rax, rdi\n"); // 比较 rax 和 rdi
            
            // 根据不同的操作符，设置 al 寄存器 (rax 的低8位)
            switch (node->op) {
                case TOKEN_EQ:  printf("  sete al\n"); break;  // Equal
                case TOKEN_NEQ: printf("  setne al\n"); break; // Not Equal
                case TOKEN_LT:  printf("  setl al\n"); break;  // Less
                case TOKEN_LE:  printf("  setle al\n"); break; // Less or Equal
                case TOKEN_GT:  printf("  setg al\n"); break;  // Greater
                case TOKEN_GE:  printf("  setge al\n"); break; // Greater or Equal
                default: break;
            }

            // 关键一步：将 8 位的 al 零扩展为 64 位的 rax
            // 这样 rax 的值就变成了真正的 0 或 1
            printf("  movzb rax, al\n");
            break;
        default:
            fprintf(stderr, "Codegen: Unsupported binary operator\n");
            exit(1);
    }
}

// 为 "If Statement" 节点生成代码
static void codegen_if_statement(IfStatementNode* node) {
    // 1. 为我们这个 if 语句创建一个唯一的标签 ID
    int label_id = label_counter++;
    
    // 2. 为条件表达式生成代码
    codegen_node(node->condition); // 执行后，比较结果会在 CPU 状态标志中

    // 3. 生成条件跳转指令
    //    我们生成的 BinaryOpNode (x > 2) 会比较 eax 和 edi
    //    如果 x > 2 为假 (即 x <= 2)，我们就应该跳过 if 的 body
    //    所以我们用 jle (Jump if Less or Equal)
    printf("  cmp rax, 0\n");
    printf("  je  _L_else_%d\n", label_id); // 如果是 0 (Equal)，跳转到 else

    // 4. 生成 if 为真时的代码
    codegen_node(node->body);

    // 如果执行完了 if 块，必须强制跳转到结束标签，跳过 else 块
    printf("  jmp  _L_end_%d\n", label_id);

    // 5. 生成 else 标签
    printf("_L_else_%d:\n", label_id);

    // 6. 如果存在 else 分支，生成它的代码
    if (node->else_branch != NULL) {
        codegen_node(node->else_branch);
    }

    // 生成结束标签
    printf("_L_end_%d:\n", label_id);
}

// 为 "while Statement" 节点生成代码
static void codegen_while_statement(WhileStatementNode* node) {
    int label_id = label_counter++;
    
    // 1. 放置“循环开始”标签
    printf("_L_start_%d:\n", label_id);

    // 2. 生成条件检查代码
    codegen_node(node->condition);

    // 3. 如果条件不满足，跳出循环 (跳到 end)
    // 注意：目前的条件只支持 > (TOKEN_GT)。
    // 如果是 x > 0，汇编比较的是 cmp rax, rdi (即 x, 0)
    // 如果 x <= 0 (即 jle)，则跳出
    printf("  cmp rax, 0\n");
    printf("  je  _L_end_%d\n", label_id); // 如果是 0，跳出循环

    // 4. 生成循环体代码
    codegen_node(node->body);

    // 5. 循环体结束后，无条件跳回开始标签，再次检查条件
    printf("  jmp _L_start_%d\n", label_id);

    // 6. 放置“循环结束”标签
    printf("_L_end_%d:\n", label_id);
}

// 处理一元节点的函数，并在分发器中注册
static void codegen_unary_op(UnaryOpNode* node) {
    // 1. 先计算操作数的值，结果会在 rax 中
    codegen_node(node->operand);

    // 2. 根据操作符处理 rax
    switch (node->op) {
        case TOKEN_AMPERSAND: // 取地址 (&x)
            // &x 的值，就是 x 的 L-value (地址)
            gen_lvalue(node->operand);
            // 此时 rax 已经是地址了，直接返回
            break;
        case TOKEN_STAR: // 解引用 (*p)
            // *p 的值，就是先算出 p 的值(地址)，再读取该地址的内容
            codegen_node(node->operand); // 计算 p，rax = 地址
            printf("  mov rax, [rax]\n"); // 读取地址里的值
            break;
        case TOKEN_MINUS: // 负号 (-x)
            printf("  neg rax\n"); // rax = -rax
            break;
        case TOKEN_BANG:  // 逻辑非 (!x)
            // 逻辑是：如果 rax 是 0，变成 1；如果是非 0，变成 0。
            printf("  cmp rax, 0\n");
            printf("  sete al\n");      // 如果相等(是0)，al=1
            printf("  movzb rax, al\n");// 扩展到 64 位
            break;
        case TOKEN_PLUS:  // 正号 (+x)
            // 什么都不用做，值不变
            break;
        default:
            fprintf(stderr, "Codegen Error: Unknown unary operator\n");
            exit(1);
    }
}

static void codegen_function_call(FunctionCallNode* node) {
    // 寄存器列表
    char* arg_regs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

    // 1. 计算所有参数的值，并压栈保存
    // 为什么要压栈？因为计算第2个参数时，可能会覆盖第1个参数用到的寄存器。
    // 最安全的做法是：算出结果 -> push -> 算出下一个 -> push ... -> 最后 pop 到指定寄存器
    
    for (int i = 0; i < node->arg_count; i++) {
        codegen_node(node->args[i]); // 结果在 rax
        printf("  push rax\n");
    }

    // 2. 将参数弹出到对应的寄存器
    // 注意：栈是后进先出 (LIFO)。
    // 如果我们要赋给 rdi (第1个), rsi (第2个)...
    // 我们 push 的顺序是 arg1, arg2...
    // 栈顶是 argN。
    // 所以 pop 的顺序必须是反的：先 pop 给最后一个参数，最后 pop 给 rdi。
    
    for (int i = node->arg_count - 1; i >= 0; i--) {
        printf("  pop %s\n", arg_regs[i]);
    }

    // 3. 调用函数
    printf("  call %s\n", node->name);
    
    // 4. 结果已经在 rax 里了，完美。
}

// 生成左值（计算变量或指针的内存地址）
// 执行完后，rax 中存放的是一个内存地址
// 生成左值（计算变量或指针的内存地址）
static void gen_lvalue(ASTNode* node) {
    if (node->type == NODE_IDENTIFIER) {
        IdentifierNode* ident = (IdentifierNode*)node;
        
        // 1. 先在局部符号表找
        Symbol* sym = find_symbol(ident->name);
        
        if (sym) {
            // [原有逻辑] 找到了 -> 局部变量 (栈地址)
            // 结果: lea rax, [rbp-8]
            printf("  lea rax, [rbp-%d]\n", sym->stack_offset);
        } else {
            // [新增逻辑] 没找到 -> 默认为全局变量 (RIP 相对寻址)
            // 结果: lea rax, [rip + g_val]
            // 注意：这里直接使用 label，不用判断是否存在，交给汇编器报错（如果拼写错误的话）
            printf("  lea rax, [rip + %s]\n", ident->name);
        }
        return;
    }
    
    // ... 下面处理指针解引用 (*p = ...) 的逻辑保持不变 ...
    if (node->type == NODE_UNARY_OP) {
        UnaryOpNode* unary = (UnaryOpNode*)node;
        if (unary->op == TOKEN_STAR) {
            codegen_node(unary->operand);
            return;
        }
    }

    fprintf(stderr, "Error: Left side of assignment must be a variable or pointer dereference.\n");
    exit(1);
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
        case NODE_IF_STATEMENT:
            codegen_if_statement((IfStatementNode*)node);
            break;
        case NODE_WHILE_STATEMENT:
            codegen_while_statement((WhileStatementNode*)node);
            break;
        case NODE_UNARY_OP:
            codegen_unary_op((UnaryOpNode*)node);
            break;
        case NODE_FUNCTION_CALL:
            codegen_function_call((FunctionCallNode*)node);
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