#ifndef AST_H
#define AST_H

#include <stdlib.h> // 为了 size_t
#include "lexer.h"

typedef enum {
    NODE_NUMERIC_LITERAL,   // 数字字面量
    NODE_BLOCK_STATEMENT,   // 代码块语句 { ... }
    NODE_PROGRAM,           // 程序的根节点
    NODE_FUNCTION_DECL,     // 函数声明
    NODE_RETURN_STATEMENT,  // return 语句
    NODE_VAR_DECL,          // 变量声明
    NODE_ASSIGN,            // 赋值表达式
    NODE_IDENTIFIER,        // 标识符 (当它作为一个值被使用时)
    NODE_BINARY_OP,         // 二元操作, e.g., +, -
    NODE_IF_STATEMENT,      // if 语句
    NODE_WHILE_STATEMENT,   // while 语句
    NODE_UNARY_OP,          // 一元操作, e.g. -x, !x
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

// 标识符节点 (e.g., 在 "return x;" 中的 x)
typedef struct {
    NodeType type; // 值为 NODE_IDENTIFIER
    char* name;
} IdentifierNode;

// 变量声明节点 (e.g., "int x = 5;")
typedef struct {
    NodeType type; // 值为 NODE_VAR_DECL
    char* name;
    ASTNode* initial_value; // 赋值的初始值表达式
} VarDeclNode;

// 二元运算符结点
typedef struct {
    NodeType type;      // 值为 NODE_BINARY_OP
    struct ASTNode* left;   // 左边的表达式
    struct ASTNode* right;  // 右边的表达式
    TokenType op;       // 操作符 (e.g., TOKEN_PLUS)
} BinaryOpNode;

// If 语句节点
typedef struct {
    NodeType type;               // 值为 NODE_IF_STATEMENT
    struct ASTNode* condition;   // 条件表达式 (e.g., x > 2)
    struct ASTNode* body;        // if 为真时执行的语句或代码块
    struct ASTNode* else_branch; // if 为假时执行的代码 (可以为 NULL)
} IfStatementNode;

// while 语句节点
typedef struct {
    NodeType type;              // 值为 NODE_WHILE_STATEMENT
    struct ASTNode* condition; // 循环条件
    struct ASTNode* body;      // 循环体
} WhileStatementNode;

// 一元操作符 结点
typedef struct {
    NodeType type;              // 值为 NODE_UNARY_OP
    TokenType op;               // 操作符: TOKEN_MINUS, TOKEN_BANG 等
    struct ASTNode* operand;    // 操作数
} UnaryOpNode;

// 原有工厂函数
NumericLiteralNode* create_numeric_literal(char* value);
BlockStatementNode* create_block_statement();
void add_statement_to_block(BlockStatementNode* block, ASTNode* statement);
ProgramNode* create_program_node();
FunctionDeclarationNode* create_function_declaration_node(char* name, BlockStatementNode* body);
ReturnStatementNode* create_return_statement_node(ASTNode* argument);
void add_declaration_to_program(ProgramNode* prog, FunctionDeclarationNode* decl);
VarDeclNode* create_var_decl_node(char* name, ASTNode* initial_value);
IdentifierNode* create_identifier_node(char* name);
BinaryOpNode* create_binary_op_node(ASTNode* left, TokenType op, ASTNode* right);
IfStatementNode* create_if_statement_node(ASTNode* condition, ASTNode* body, ASTNode* else_branch);
WhileStatementNode* create_while_statement_node(ASTNode* condition, ASTNode* body);

// 新工厂函数
UnaryOpNode* create_unary_op_node(TokenType op, ASTNode* operand);

#endif // AST_H