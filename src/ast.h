#ifndef AST_H
#define AST_H

#include <stdlib.h> // 为了 size_t

typedef enum {
    NODE_NUMERIC_LITERAL, // 数字字面量
    NODE_BLOCK_STATEMENT,   // 代码块语句 { ... }
    NODE_PROGRAM,           // 程序的根节点
    NODE_FUNCTION_DECL,     // 函数声明
    NODE_RETURN_STATEMENT   // return 语句
} NodeType;

// AST 节点的通用结构体
// 所有具体的节点都会包含这个作为头部，以便我们识别它的类型
typedef struct ASTNode {
    NodeType type;
} ASTNode;

// 数字节点
typedef struct {
    NodeType type;
    char* value;
} NumericLiteralNode;

// 代码块节点
typedef struct {
    NodeType type;
    struct ASTNode** statements;
    int count;
} BlockStatementNode;


// Return 语句节点
typedef struct {
    NodeType type; // 值为 NODE_RETURN_STATEMENT
    struct ASTNode* argument; // return 后面跟着的表达式
} ReturnStatementNode;

// 函数声明节点, e.g., "int main() { ... }"
typedef struct {
    NodeType type; // 值为 NODE_FUNCTION_DECL
    char* name; // 函数名, e.g., "main"
    BlockStatementNode* body; // 函数体 (一个代码块)
    // (为了简化，我们暂时省略返回类型和参数)
} FunctionDeclarationNode;

// 程序根节点，它是所有顶层声明的容器
typedef struct {
    NodeType type; // 值为 NODE_PROGRAM
    FunctionDeclarationNode** declarations; // 存储程序中所有的函数声明
    int count; // 声明的数量
} ProgramNode;


// 原有工厂函数
NumericLiteralNode* create_numeric_literal(char* value);
BlockStatementNode* create_block_statement();
void add_statement_to_block(BlockStatementNode* block, ASTNode* statement);

// 添加新工厂函数的声明
ProgramNode* create_program_node();
FunctionDeclarationNode* create_function_declaration_node(char* name, BlockStatementNode* body);
ReturnStatementNode* create_return_statement_node(ASTNode* argument);
void add_declaration_to_program(ProgramNode* prog, FunctionDeclarationNode* decl);

#endif // AST_H