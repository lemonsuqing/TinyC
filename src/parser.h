#ifndef PARSER_H
#define PARSER_H

#include "ast.h"

// 语法分析器的职责是生成 AST
// 这个函数是语法分析器的入口
// 它将返回整个程序的 AST 的根节点
ASTNode* parse();

#endif // PARSER_H