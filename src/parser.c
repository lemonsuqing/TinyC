#include <stdio.h>
#include <stdlib.h>
#include "parser.h"
#include "lexer.h"

// -----------
// 模块级（私有）变量
// -----------

// 'static' 关键字意味着这个变量只在当前文件 (parser.c) 中可见。
// 它像一个全局变量，但不会污染其他文件。
// `current_token` 保存了词法分析器刚刚分析出的、我们正要处理的 Token。
static Token* current_token;

// -----------
// 辅助函数
// -----------

/**
 * @brief "吃掉"并消费一个 Token。
 * 
 * 这个函数是语法分析器的核心驱动力之一。它的作用是：
 * 1. 验证当前的 Token 是否是我们期望的类型。
 * 2. 如果是，就释放当前 Token 的内存，并让 `current_token` 指向下一个 Token。
 * 3. 如果不是，说明代码存在语法错误，程序将报错并退出。
 * 
 * @param type 我们期望 `current_token` 应该具有的 TokenType。
 */
static void eat(TokenType type) {
    // 检查当前 Token 的类型是否与期望的类型匹配
    if (current_token->type == type) {
        // 匹配成功！
        // 释放掉已经被我们处理完毕的 Token 结构体
        free(current_token);
        // 调用词法分析器，获取下一个 Token，并更新 current_token
        current_token = get_next_token();
    } else {
        // 匹配失败！代码有语法错误。
        fprintf(stderr, "Syntax Error: Expected token %d, but got %d\n", type, current_token->type);
        exit(1);
    }
}

// -----------
// 解析函数 (Parsing Functions)
// 每个函数负责解析一种特定的语法结构，并返回一个对应的 AST 节点。
// -----------

/**
 * @brief 解析一个数字字面量。
 * 
 * 当我们遇到一个 TOKEN_INT 类型的 Token 时，调用此函数。
 * 它会创建一个 NumericLiteralNode 来代表这个数字。
 * @return 指向新创建的 ASTNode 的指针。
 */
ASTNode* parse_numeric_literal() {
    // 1. 创建 AST 节点
    // 我们将当前 Token 的值 (例如 "123") 传递给工厂函数。
    // 注意：`current_token->value` 是在词法分析阶段动态分配 (malloc) 的内存。
    // AST 节点接管了这块内存的所有权，所以之后需要由 AST 的清理函数来释放。
    NumericLiteralNode* node = create_numeric_literal(current_token->value);

    // 2. 消费 Token
    // 这个整数 Token 的使命已经完成，我们 "吃掉" 它，让解析器前进。
    eat(TOKEN_INT);

    // 3. 返回新创建的节点
    return (ASTNode*)node;
}

/**
 * @brief 解析一个代码块 (Block Statement)。
 * 
 * 代码块是由 '{' 和 '}' 包围的一系列语句。
 * 例如: { 123 }
 * @return 指向新创建的 BlockStatementNode 的指针。
 */
ASTNode* parse_block_statement() {
    // 1. 消费 '{'
    // 我们期望代码块的开始是一个左大括号，所以我们先消费掉它。
    eat(TOKEN_LBRACE);

    // 2. 创建代码块节点
    // 这个节点将作为容器，存放代码块内部的所有语句。
    BlockStatementNode* block_node = create_block_statement();

    // 3. 循环解析内部的语句
    // 只要下一个 Token 不是右大括号 '}'，我们就认为还有语句需要解析。
    while (current_token->type != TOKEN_RBRACE) {
        // 调用相应的解析函数来解析语句。
        // 目前我们的语言很简单，块里只允许有数字，所以直接调用 parse_numeric_literal()。
        ASTNode* statement = parse_numeric_literal();

        // 将解析出的语句 AST 节点添加到代码块节点的列表中。
        add_statement_to_block(block_node, statement);
    }

    // 4. 消费 '}'
    // 循环结束，意味着我们遇到了 '}'，代码块解析完毕。消费掉它。
    eat(TOKEN_RBRACE);

    // 5. 返回代码块节点
    return (ASTNode*)block_node;
}

// -----------
// 语法分析器主入口
// -----------

/**
 * @brief 语法分析器的入口函数。
 * 
 * 开始整个语法分析过程。
 * @return 指向整个程序 AST 的根节点。
 */
ASTNode* parse() {
    // 1. 获取第一个 Token，启动解析流程
    current_token = get_next_token();

    // 2. 开始解析
    // 根据我们最顶层的语法规则（目前是“一个程序就是一个代码块”），
    // 调用相应的解析函数。
    ASTNode* root = parse_block_statement();

    // 3. 检查文件是否已结束
    // 一个合法的程序在代码块结束后，应该紧跟着文件结束符 (EOF)。
    // 如果不是，说明代码块后面还有多余的内容，这是一个语法错误。
    if (current_token->type != TOKEN_EOF) {
        fprintf(stderr, "Syntax Error: Unexpected token after block statement.\n");
        exit(1);
    }

    return root;
}


// -----------
// AST 节点工厂函数 (Factory Functions)
// 这些函数负责为 AST 节点分配内存和初始化。
// -----------

NumericLiteralNode* create_numeric_literal(char* value) {
    NumericLiteralNode* node = (NumericLiteralNode*)malloc(sizeof(NumericLiteralNode));
    if (!node) { fprintf(stderr, "Memory allocation failed!\n"); exit(1); }
    node->type = NODE_NUMERIC_LITERAL;
    node->value = value; // 浅拷贝，接管了 value 指针的所有权
    return node;
}

BlockStatementNode* create_block_statement() {
    BlockStatementNode* node = (BlockStatementNode*)malloc(sizeof(BlockStatementNode));
    if (!node) { fprintf(stderr, "Memory allocation failed!\n"); exit(1); }
    node->type = NODE_BLOCK_STATEMENT;
    node->statements = NULL; // 初始化为空指针，这对于 realloc 首次调用很重要
    node->count = 0;
    return node;
}

void add_statement_to_block(BlockStatementNode* block, ASTNode* statement) {
    // 语句数量加一
    block->count++;
    // 重新分配 `statements` 数组的内存，使其大小能容纳新的语句指针
    block->statements = realloc(block->statements, block->count * sizeof(ASTNode*));
    if (!block->statements) { fprintf(stderr, "Memory allocation failed!\n"); exit(1); }
    // 将新的语句指针存放到数组的末尾
    block->statements[block->count - 1] = statement;
}