#ifndef AST_H
#define AST_H

#include <stdlib.h> // 为了 size_t
#include "lexer.h"

#define MAX_MEMBERS 20
#define MAX_STRUCTS 20

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
    NODE_FUNCTION_CALL,     // 函数调用 add(1, 2)
    NODE_ARRAY_ACCESS,      // 数组访问 a[i]
    NODE_STRING_LITERAL,    // string, such as: "Hello"
    NODE_FOR_STATEMENT,     // for 语句
    NODE_BREAK,             // break
    NODE_CONTINUE,          // continue
    NODE_MEMBER_ACCESS,
} NodeType;

// 数据类型枚举
typedef enum {
    TYPE_INT,
    TYPE_CHAR,
    TYPE_STRUCT,
    // 未来可以在这里加 TYPE_VOID, TYPE_STRUCT 等
} DataType;

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
    NodeType type;              // 值为 NODE_FUNCTION_DECL
    char* name;                 // 函数名, e.g., "main"
    struct ASTNode** args;      // 参数列表 (数组)
    int arg_count;              // 参数个数
    BlockStatementNode* body;   // 函数体 (一个代码块)
} FunctionDeclarationNode;

// 函数调用节点
typedef struct {
    NodeType type;              // 值为 NODE_FUNCTION_CALL
    char* name;                 // 调用的函数名
    struct ASTNode** args;      // 传入的实参列表 (表达式)
    int arg_count;              // 参数个数
} FunctionCallNode;

// 程序根节点，它是所有顶层声明的容器
typedef struct {
    NodeType type; // 值为 NODE_PROGRAM
    struct ASTNode** declarations; // 存储程序中所有的函数声明
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
    int array_size;  // 0 表示标量，>0 表示数组大小
    DataType var_type;
    char* struct_name; // 如果是结构体变量，记录是哪个结构体 (如 "Point")
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
    struct ASTNode* condition;  // 循环条件
    struct ASTNode* body;       // 循环体
} WhileStatementNode;

// 一元操作符 结点
typedef struct {
    NodeType type;              // 值为 NODE_UNARY_OP
    TokenType op;               // 操作符: TOKEN_MINUS, TOKEN_BANG 等
    struct ASTNode* operand;    // 操作数
} UnaryOpNode;

// 数组访问节点
typedef struct {
    NodeType type;      // NODE_ARRAY_ACCESS
    char* array_name;   // 数组名
    struct ASTNode* index;      // 索引表达式
} ArrayAccessNode;

// 字符串节点
typedef struct {
    NodeType type;
    char* value;     // 存储 "Hello"
    int original_id; // [关键] 用于代码生成时标记它是第几个字符串 (.LC0, .LC1...)
} StringLiteralNode;

// for 语句节点
typedef struct {
    NodeType type;             // NODE_FOR_STATEMENT
    struct ASTNode* init;      // int i = 0;
    struct ASTNode* condition; // i < 10;
    struct ASTNode* increment; // i = i + 1;
    struct ASTNode* body;      // { ... }
} ForStatementNode;

typedef struct {
    NodeType type;
} BreakNode;

typedef struct {
    NodeType type;
} ContinueNode;

typedef struct {
    char* name;
    DataType type;
    int offset; // 成员相对于结构体起始位置的偏移量
} MemberInfo;

typedef struct {
    char* name;
    MemberInfo members[MAX_MEMBERS];
    int member_count;
    int size;   // 总大小 (字节)
} StructDef;

extern StructDef struct_table[MAX_STRUCTS];
extern int struct_count;

// 辅助函数：定义结构体、查找结构体、查找成员
StructDef* define_struct(char* name);
void add_struct_member(StructDef* s, char* member_name, DataType type);
StructDef* find_struct(char* name);
MemberInfo* find_struct_member(StructDef* s, char* member_name);

// 新增：成员访问节点 p.x
typedef struct {
    NodeType type;      // NODE_MEMBER_ACCESS
    char* struct_var_name; // 变量名 "p"
    char* member_name;     // 成员名 "x"
} MemberAccessNode;

// 原有工厂函数
NumericLiteralNode* create_numeric_literal(char* value);
BlockStatementNode* create_block_statement();
void add_statement_to_block(BlockStatementNode* block, ASTNode* statement);
ProgramNode* create_program_node();
FunctionDeclarationNode* create_function_declaration_node(char* name, struct ASTNode** args, int arg_count, BlockStatementNode* body);
ReturnStatementNode* create_return_statement_node(ASTNode* argument);
void add_declaration_to_program(ProgramNode* prog, struct ASTNode* decl);
VarDeclNode* create_var_decl_node(char* name, ASTNode* initial_value, int array_size, DataType var_type, char* struct_name);
IdentifierNode* create_identifier_node(char* name);
BinaryOpNode* create_binary_op_node(ASTNode* left, TokenType op, ASTNode* right);
IfStatementNode* create_if_statement_node(ASTNode* condition, ASTNode* body, ASTNode* else_branch);
WhileStatementNode* create_while_statement_node(ASTNode* condition, ASTNode* body);
UnaryOpNode* create_unary_op_node(TokenType op, ASTNode* operand);
FunctionCallNode* create_function_call_node(char* name, struct ASTNode** args, int arg_count);
ArrayAccessNode* create_array_access_node(char* name, struct ASTNode* index);
StringLiteralNode* create_string_literal_node(char* value);
ForStatementNode* create_for_statement_node(ASTNode* init, ASTNode* cond, ASTNode* inc, ASTNode* body);
ASTNode* create_break_node();
ASTNode* create_continue_node();

// 新工厂函数
MemberAccessNode* create_member_access_node(char* var_name, char* member_name);

#endif // AST_H