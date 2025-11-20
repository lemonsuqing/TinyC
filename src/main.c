#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "parser.h"
#include "ast.h"

// -----------
// 调试与清理函数
// -----------

/**
 * @brief 递归地打印抽象语法树 (AST)。
 * @param node 要打印的 AST 节点。
 * @param indent 当前的缩进级别，用于美化输出。
 */
void print_ast(ASTNode* node, int indent) {
    if (node == NULL) return;

    for (int i = 0; i < indent; i++) printf("  ");

    switch (node->type) {
        case NODE_BLOCK_STATEMENT: {
            printf("BlockStatement:\n");
            BlockStatementNode* block = (BlockStatementNode*)node;
            for (int i = 0; i < block->count; i++) {
                // 递归打印代码块中的每一个语句
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

/**
 * @brief 递归地释放整个抽象语法树 (AST) 的内存。
 *
 * 这是一个非常重要的函数，用于防止内存泄漏。
 * 它必须以“后序遍历”的方式工作：先释放所有子节点，然后释放父节点本身。
 * @param node 要释放的 AST 树的根节点。
 */
void free_ast(ASTNode* node) {
    if (node == NULL) return;

    switch (node->type) {
        case NODE_BLOCK_STATEMENT: {
            BlockStatementNode* block = (BlockStatementNode*)node;
            // 1. 先递归地释放代码块中的每一个语句（子节点）
            for (int i = 0; i < block->count; i++) {
                free_ast(block->statements[i]);
            }
            // 2. 释放存储语句指针的数组本身
            free(block->statements);
            break; // 在 switch 中不要忘了 break!
        }
        case NODE_NUMERIC_LITERAL: {
            NumericLiteralNode* num_node = (NumericLiteralNode*)node;
            // 1. 释放数字字符串 `value` 的内存
            //    这块内存是在词法分析器中为数字 malloc 的
            free(num_node->value);
            break;
        }
        default:
            // 其他节点类型目前没有需要特殊释放的内存
            break;
    }

    // 最后，释放节点结构体本身
    free(node);
}

// -----------
// 主函数
// -----------

int main() {
    char* source_code = "{ 123 }";
    printf("正在分析: %s\n", source_code);
    
    // 1. 初始化词法分析器，传入源代码
    lexer_init(source_code);

    // 2. 调用语法分析器，生成 AST
    // printf("开始语法分析...\n");
    ASTNode* root = parse();
    printf("语法分析完成！\n");

    // 3. 打印 AST 来验证结果
    printf("\n生成的 AST 树:\n");
    print_ast(root, 0);

    // 4. 释放整个 AST 树的内存
    // printf("\n开始释放 AST 内存...\n");
    free_ast(root);
    printf("内存已释放。\n");

    return 0;
}