#ifndef AST_H
#define AST_H

// 定义 AST 节点的类型
typedef enum {
    NODE_NUMERIC_LITERAL, // 数字字面量
    NODE_BLOCK_STATEMENT    // 代码块语句 { ... }
} NodeType;

// AST 节点的通用结构体
// 所有具体的节点都会包含这个作为头部，以便我们识别它的类型
typedef struct ASTNode {
    NodeType type;
} ASTNode;

// 数字字面量节点
typedef struct {
    NodeType type; // 值为 NODE_NUMERIC_LITERAL
    char* value;   // 数字的字符串表示，例如 "123"
} NumericLiteralNode;

// 代码块节点
typedef struct {
    NodeType type;        // 值为 NODE_BLOCK_STATEMENT
    ASTNode** statements; // 存储代码块中所有语句的列表
    int count;            // 语句的数量
} BlockStatementNode;


// --- 函数声明 ---
// 创建各种 AST 节点的工厂函数
NumericLiteralNode* create_numeric_literal(char* value);
BlockStatementNode* create_block_statement();
void add_statement_to_block(BlockStatementNode* block, ASTNode* statement);

#endif // AST_H