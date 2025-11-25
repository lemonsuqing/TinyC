#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "codegen.h"

// -----------
// 调试与清理函数
// -----------
void print_ast(ASTNode* node, int indent) {
    if (node == NULL) return;

    for (int i = 0; i < indent; i++) printf("  ");

    switch (node->type) {
        case NODE_PROGRAM: {
            printf("Program:\n");
            ProgramNode* prog = (ProgramNode*)node;
            for (int i = 0; i < prog->count; i++) {
                print_ast((ASTNode*)prog->declarations[i], indent + 1);
            }
            break;
        }
        case NODE_FUNCTION_DECL: {
            FunctionDeclarationNode* func = (FunctionDeclarationNode*)node;
            printf("FunctionDeclaration: name='%s'\n", func->name);
            print_ast((ASTNode*)func->body, indent + 1);
            break;
        }
        case NODE_RETURN_STATEMENT: {
            printf("ReturnStatement:\n");
            ReturnStatementNode* ret = (ReturnStatementNode*)node;
            print_ast(ret->argument, indent + 1);
            break;
        }
        case NODE_BLOCK_STATEMENT: {
            printf("BlockStatement:\n");
            BlockStatementNode* block = (BlockStatementNode*)node;
            for (int i = 0; i < block->count; i++) {
                print_ast(block->statements[i], indent + 1);
            }
            break;
        }
        case NODE_NUMERIC_LITERAL: {
            NumericLiteralNode* num_node = (NumericLiteralNode*)node;
            printf("NumericLiteral: %s\n", num_node->value);
            break;
        }
        default:
            printf("Unknown Node\n");
    }
}

// 这是一个完整且正确的 free_ast 函数
void free_ast(ASTNode* node) {
    if (node == NULL) return;

    switch (node->type) {
        case NODE_PROGRAM: {
            ProgramNode* prog = (ProgramNode*)node;
            for (int i = 0; i < prog->count; i++) {
                free_ast((ASTNode*)prog->declarations[i]);
            }
            free(prog->declarations);
            break;
        }
        case NODE_FUNCTION_DECL: {
            FunctionDeclarationNode* func = (FunctionDeclarationNode*)node;
            free(func->name);
            free_ast((ASTNode*)func->body);
            break;
        }
        case NODE_BLOCK_STATEMENT: {
            BlockStatementNode* block = (BlockStatementNode*)node;
            for (int i = 0; i < block->count; i++) {
                free_ast(block->statements[i]);
            }
            free(block->statements);
            break;
        }
        case NODE_VAR_DECL: { // <--- 新增 case
            VarDeclNode* var_decl = (VarDeclNode*)node;
            free(var_decl->name); // 释放变量名
            free_ast(var_decl->initial_value); // 递归释放初始值
            break;
        }
        case NODE_RETURN_STATEMENT: {
            ReturnStatementNode* ret = (ReturnStatementNode*)node;
            free_ast(ret->argument);
            break;
        }
        case NODE_IDENTIFIER: { // <--- 新增 case
            IdentifierNode* ident = (IdentifierNode*)node;
            free(ident->name); // 释放标识符名
            break;
        }
        case NODE_NUMERIC_LITERAL: {
            NumericLiteralNode* num_node = (NumericLiteralNode*)node;
            free(num_node->value);
            break;
        }
        case NODE_IF_STATEMENT: { // 假设你有这个 case，如果没有请加上
            IfStatementNode* if_stmt = (IfStatementNode*)node;
            free_ast(if_stmt->condition);
            free_ast(if_stmt->body);
            if (if_stmt->else_branch != NULL) { // 递归释放 else
                free_ast(if_stmt->else_branch);
            }
            break;
        }
        case NODE_WHILE_STATEMENT: {
            WhileStatementNode* while_stmt = (WhileStatementNode*)node;
            free_ast(while_stmt->condition);
            free_ast(while_stmt->body);
            break;
        }
        default:
            break;
    }
    free(node);
}

// -----------
// 主函数
// -----------
int main() {
    char* source_code = "int main() { int x = 5; int sum = 0; while (x > 0) { sum = sum + x; x = x - 1; } return sum; }";
    // printf("--- 正在分析 ---\n%s\n\n", source_code);
    
    lexer_init(source_code);

    ASTNode* root = parse();
    // printf("语法分析完成！\n\n");

    // printf("--- 生成的 AST 树 ---\n");
    // print_ast(root, 0);

    // printf("--- Generating Assembly Code ---\n");
    codegen(root);

    free_ast(root);
    // printf("内存已释放。\n");

    return 0;
}