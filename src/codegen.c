#include <stdio.h>
#include "codegen.h"

// 声明我们将要使用的递归函数
static void codegen_node(ASTNode* node);

// --- AST 节点代码生成函数 ---

// 为 "Program" 节点生成代码
static void codegen_program(ProgramNode* node) {
    // 汇编程序的起点
    printf(".intel_syntax noprefix\n"); // 使用更常见的 Intel 语法（可选，但对初学者更友好）
    
    // 遍历并为程序中的每个声明（函数）生成代码
    for (int i = 0; i < node->count; i++) {
        codegen_node((ASTNode*)node->declarations[i]);
    }
}

// 为 "Function Declaration" 节点生成代码
static void codegen_function_declaration(FunctionDeclarationNode* node) {
    // 声明一个全局可链接的函数标签
    printf(".globl %s\n", node->name);
    // 定义函数标签
    printf("%s:\n", node->name);

    // --- 函数序言 (Prologue) ---
    printf("  push rbp\n");
    printf("  mov rbp, rsp\n");
    
    // 为函数体（一个代码块）生成代码
    codegen_node((ASTNode*)node->body);

    // 注意：函数尾声 (epilogue) 将在 return 语句中生成，
    // 因为 return 之后就不再有其他代码了。
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
    // TODO (1/2): 为 return 语句生成代码
    
    // 思考一下 return 语句的汇编要做什么：
    // 1. 计算要返回的表达式的值，并把它放入 %rax / %eax 寄存器。
    //    为了做到这一点，你需要递归地调用 codegen_node() 来处理它的子节点 (node->argument)。
    codegen_node(node->argument);

    // 2. 生成函数尾声 (Epilogue) 和返回指令。
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
    printf("  mov eax, %s\n", node->value);
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
        default:
            fprintf(stderr, "Codegen Error: Unknown AST node type %d\n", node->type);
            exit(1);
    }
}

// --- 代码生成器主入口 ---
void codegen(ASTNode* root) {
    codegen_node(root);
}