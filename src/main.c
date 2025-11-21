#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "parser.h"
#include "ast.h"

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
            free(func->name); // 释放 strdup 复制的函数名
            free_ast((ASTNode*)func->body);
            break;
        }
        case NODE_RETURN_STATEMENT: {
            ReturnStatementNode* ret = (ReturnStatementNode*)node;
            free_ast(ret->argument);
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
        case NODE_NUMERIC_LITERAL: {
            NumericLiteralNode* num_node = (NumericLiteralNode*)node;
            free(num_node->value);
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
    char* source_code = "int main() { return 0; }";
    printf("--- 正在分析 ---\n%s\n\n", source_code);
    
    lexer_init(source_code);

    ASTNode* root = parse();
    printf("语法分析完成！\n\n");

    printf("--- 生成的 AST 树 ---\n");
    print_ast(root, 0);

    free_ast(root);
    printf("内存已释放。\n");

    return 0;
}