#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast.h"

/**
 * @brief 代码生成器的入口函数。
 * 
 * 接收 AST 的根节点，并将生成的汇编代码打印到标准输出。
 * @param root AST 的根节点。
 */
void codegen(ASTNode* root);

#endif // CODEGEN_H