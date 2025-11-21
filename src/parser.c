#include <stdio.h>
#include <stdlib.h>
#include <string.h> // 为了 strcmp 和 strdup
#include "parser.h"
#include "lexer.h"

// -----------
// 模块私有变量和函数声明
// -----------

static Token* current_token;
static void eat(TokenType type);

// 向前声明 (Forward Declaration)
// 因为函数之间存在相互调用，我们需要提前告诉编译器这些函数的存在。
ASTNode* parse_statement();
ASTNode* parse_expression();
ASTNode* parse_block_statement();

// -----------
// 辅助函数
// -----------

static void eat(TokenType type) {
    if (current_token->type == type) {
        // 对于需要其值的Token (如INT, IDENTIFIER)，它的value指针已经被AST节点接管，
        // 我们只需要释放Token结构体本身。
        // 对于不需要其值的Token，我们也要释放其value（如果它是动态分配的）。
        // 目前我们的词法分析器对所有动态分配的value都交给了AST，所以这里逻辑简化。
        if (current_token->value != NULL && (type != TOKEN_INT && type != TOKEN_IDENTIFIER && type != TOKEN_KEYWORD)) {
             // 对于那些我们没有在AST中保存其value的动态分配的token，需要在这里处理
             // 但目前的设计下，这个分支暂时不会被走到。
        }
        free(current_token); 
        current_token = get_next_token();
    } else {
        fprintf(stderr, "Syntax Error: Expected token %d, but got %d\n", type, current_token->type);
        exit(1);
    }
}

// -----------
// 解析函数 - 从具体到抽象 (自底向上)
// -----------

// 解析表达式。目前，我们的“表达式”只能是一个数字字面量。
ASTNode* parse_expression() {
    // 检查当前token是否为数字
    if (current_token->type == TOKEN_INT) {
        return (ASTNode*)create_numeric_literal(current_token->value);
    }
    // 如果不是数字，则是语法错误
    fprintf(stderr, "Syntax Error: Expected an expression (e.g., a number), but got token type %d\n", current_token->type);
    exit(1);
}

// 解析返回语句: "return" <expression> ";"
ASTNode* parse_return_statement() {
    // TODO (1/5): 完成 return 语句的解析
    // 1. 确认当前 token 是 "return" 关键字，然后消费它
    eat(TOKEN_KEYWORD); // 简化：我们假设这里的KEYWORD一定是return

    // 2. 解析 return 后面跟着的表达式
    ASTNode* argument = parse_expression();
    // 消费掉 INT Token
    eat(TOKEN_INT);

    // 3. 消费句末的分号
    eat(TOKEN_SEMICOLON);

    // 4. 创建并返回一个 ReturnStatementNode
    return (ASTNode*)create_return_statement_node(argument);

    return NULL; // 占位符
}

// 解析一个语句。目前一个语句只能是 "return" 语句。
ASTNode* parse_statement() {
    // TODO (2/5): 完成语句的解析
    // 检查当前 token 是否是关键字 "return"
    if (current_token->type == TOKEN_KEYWORD && strcmp(current_token->value, "return") == 0) {
        // 如果是，就调用 parse_return_statement()
        return parse_return_statement();
    }
    
    // 如果不是我们认识的语句，就报错
    fprintf(stderr, "Syntax Error: Unexpected statement starting with token value '%s'\n", current_token->value);
    exit(1);
}

// 解析代码块: "{" {statement} "}"
ASTNode* parse_block_statement() {
    eat(TOKEN_LBRACE);
    BlockStatementNode* block_node = create_block_statement();
    
    // 循环解析块内的所有语句，直到遇到 '}'
    while (current_token->type != TOKEN_RBRACE) {
        ASTNode* statement = parse_statement();
        add_statement_to_block(block_node, statement);
    }
    
    eat(TOKEN_RBRACE);
    return (ASTNode*)block_node;
}

// 解析函数声明: "int" <identifier> "(" ")" <block_statement>
FunctionDeclarationNode* parse_function_declaration() {
    // TODO (3/5): 完成函数声明的解析
    // 1. 消费 "int" 关键字
    eat(TOKEN_KEYWORD);

    // 2. 获取函数名
    char* function_name = strdup(current_token->value); // 使用 strdup 复制一份字符串
    eat(TOKEN_IDENTIFIER);

    // 3. 消费左右括号
    eat(TOKEN_LPAREN);
    eat(TOKEN_RPAREN);

    // 4. 解析函数体
    BlockStatementNode* body = (BlockStatementNode*)parse_block_statement();
    
    // 5. 创建并返回 FunctionDeclarationNode
    // return create_function_declaration_node(...);

    return create_function_declaration_node(function_name, body);
}

// -----------
// 语法分析器主入口
// -----------

ASTNode* parse() {
    current_token = get_next_token();
    ProgramNode* prog = create_program_node();

    // 循环解析所有顶层声明 (目前只有函数)，直到文件末尾
    while (current_token->type != TOKEN_EOF) {
        FunctionDeclarationNode* func_decl = parse_function_declaration();
        add_declaration_to_program(prog, func_decl);
    }
    
    return (ASTNode*)prog;
}


// -----------
// AST 节点工厂函数
// -----------
NumericLiteralNode* create_numeric_literal(char* value) {
    NumericLiteralNode* node = (NumericLiteralNode*)malloc(sizeof(NumericLiteralNode));
    if (!node) { exit(1); }
    node->type = NODE_NUMERIC_LITERAL;
    node->value = value;
    return node;
}

BlockStatementNode* create_block_statement() {
    BlockStatementNode* node = (BlockStatementNode*)malloc(sizeof(BlockStatementNode));
    if (!node) { exit(1); }
    node->type = NODE_BLOCK_STATEMENT;
    node->statements = NULL;
    node->count = 0;
    return node;
}

void add_statement_to_block(BlockStatementNode* block, ASTNode* statement) {
    block->count++;
    block->statements = realloc(block->statements, block->count * sizeof(ASTNode*));
    if (!block->statements) { exit(1); }
    block->statements[block->count - 1] = statement;
}
// 创建一个空的 Program 节点
ProgramNode* create_program_node() {
    ProgramNode* node = (ProgramNode*)malloc(sizeof(ProgramNode));
    if (!node) { exit(1); }
    node->type = NODE_PROGRAM;
    node->declarations = NULL;
    node->count = 0;
    return node;
}

// 创建一个函数声明节点
FunctionDeclarationNode* create_function_declaration_node(char* name, BlockStatementNode* body) {
    FunctionDeclarationNode* node = (FunctionDeclarationNode*)malloc(sizeof(FunctionDeclarationNode));
    if (!node) { exit(1); }
    node->type = NODE_FUNCTION_DECL;
    node->name = name; // 接管 name 指针
    node->body = body; // 接管 body 指针
    return node;
}

// 创建一个返回语句节点
ReturnStatementNode* create_return_statement_node(ASTNode* argument) {
    ReturnStatementNode* node = (ReturnStatementNode*)malloc(sizeof(ReturnStatementNode));
    if (!node) { exit(1); }
    node->type = NODE_RETURN_STATEMENT;
    node->argument = argument; // 接管 argument 指针
    return node;
}

// 将一个函数声明添加到 Program 节点的列表中
void add_declaration_to_program(ProgramNode* prog, FunctionDeclarationNode* decl) {
    prog->count++;
    prog->declarations = realloc(prog->declarations, prog->count * sizeof(FunctionDeclarationNode*));
    if (!prog->declarations) { exit(1); }
    prog->declarations[prog->count - 1] = decl;
}