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
ASTNode* parse_additive_expression();
ASTNode* parse_term();
ASTNode* parse_if_statement();
ASTNode* parse_assignment_statement();

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

// 新增一个解析 "项" 的函数，目前一个项就是一个数字或变量
ASTNode* parse_term() {
    ASTNode* node = NULL;
    char* value_ptr = current_token->value;
    TokenType type = current_token->type;

    if (type == TOKEN_INT) {
        node = (ASTNode*)create_numeric_literal(value_ptr);
    } else if (type == TOKEN_IDENTIFIER) {
        node = (ASTNode*)create_identifier_node(value_ptr);
    } else {
        fprintf(stderr, "Syntax Error: Expected a number or an identifier\n");
        exit(1);
    }
    eat(type);
    return node;
}

// 解析表达式。
ASTNode* parse_additive_expression() {
    // 1. 先解析左边的一个 "term"
    ASTNode* left = parse_term();

    // 2. 循环检查后面是否跟着 '+' 或 '-'
    while (current_token->type == TOKEN_PLUS || current_token->type == TOKEN_MINUS) {
        // a. 获取操作符 token
        TokenType op = current_token->type;
        // b. 消费掉操作符 token
        eat(op);
        // c. 解析右边的 "term"
        ASTNode* right = parse_term();
        
        // d. 将左边的节点、操作符、右边的节点，组合成一个新的、更大的左节点
        //    这样，下一次循环时，这个新的组合体就成了新的 "left"
        //    例如，解析 1 + 2 + 3 时：
        //    - 第一次循环后, left 变成 (1 + 2)
        //    - 第二次循环时, left 是 (1 + 2), right 是 3, 最终 left 变成 ((1 + 2) + 3)
        left = (ASTNode*)create_binary_op_node(left, op, right);
    }

    // 3. 返回最终构建好的表达式树的根节点
    return left;
}

// 新增一个解析比较表达式的函数
ASTNode* parse_comparison_expression() {
    // TODO:
    // 1. 先解析一个加减法级别的表达式
    ASTNode* left = parse_additive_expression();
    // 2. 检查后面是否跟着一个比较操作符 (现在只有 '>')
    if (current_token->type == TOKEN_GT) {
    //        a. 获取操作符
    TokenType op = current_token->type;
    //        b. 消费操作符
    eat(op);
    //        c. 解析右边的加减法表达式
    ASTNode* right = parse_additive_expression();
    //        d. 将它们组合成一个新的 BinaryOpNode
    left = (ASTNode*)create_binary_op_node(left, op, right);
    }
    // 3. 返回最终的表达式树
    return left;
}

// 把 parse_expression 作为总入口
ASTNode* parse_expression() {
    return parse_comparison_expression();
}

// 解析变量声明语句: "int" <identifier> "=" <expression> ";"
ASTNode* parse_variable_declaration() {
    eat(TOKEN_KEYWORD); // 消费 "int"

    // --- 核心修正：不再使用 strdup ---
    // 1. 先保存指针
    char* variable_name = current_token->value;
    // 2. 再消费 token
    eat(TOKEN_IDENTIFIER);

    eat(TOKEN_ASSIGN);

    ASTNode* expr = parse_expression(); // parse_expression 内部已经消费了它自己的token

    eat(TOKEN_SEMICOLON);

    // 3. 将保存的指针传递给 AST 节点
    return (ASTNode*)create_var_decl_node(variable_name, expr);
}

// 解析返回语句: "return" <expression> ";"
ASTNode* parse_return_statement() {
    // TODO (1/5): 完成 return 语句的解析
    // 1. 确认当前 token 是 "return" 关键字，然后消费它
    eat(TOKEN_KEYWORD); // 简化：我们假设这里的KEYWORD一定是return

    // 2. 解析 return 后面跟着的表达式
    ASTNode* argument = parse_expression();

    // 3. 消费句末的分号
    eat(TOKEN_SEMICOLON);

    // 4. 创建并返回一个 ReturnStatementNode
    return (ASTNode*)create_return_statement_node(argument);
}

ASTNode* parse_assignment_statement() {
    // 左边是一个已存在的变量
    char* var_name = current_token->value;
    ASTNode* left = (ASTNode*)create_identifier_node(var_name);
    eat(TOKEN_IDENTIFIER);

    eat(TOKEN_ASSIGN);
    
    ASTNode* right = parse_expression();
    
    // 我们把赋值也建模成一个二元操作节点
    ASTNode* assignment_node = (ASTNode*)create_binary_op_node(left, TOKEN_ASSIGN, right);
    
    eat(TOKEN_SEMICOLON);
    
    return assignment_node;
}

// 解析一个语句。目前一个语句只能是 "return" 语句。
ASTNode* parse_statement() {
    // TODO (2/5): 完成语句的解析
    // 检查当前 token 是否是关键字 "return"
    if (current_token->type == TOKEN_KEYWORD && strcmp(current_token->value, "return") == 0) {
        // 如果是，就调用 parse_return_statement()
        return parse_return_statement();
    }
    // 如果是 "int" 关键字，说明这是一个变量声明
    if (current_token->type == TOKEN_KEYWORD && strcmp(current_token->value, "int") == 0) {
        return parse_variable_declaration();
    }

    // 如果是 "if" 关键字
    if (strcmp(current_token->value, "if") == 0) {
        return parse_if_statement();
    }

    // 注意：赋值语句 (x = 5;) 也是一种语句，我们需要在这里处理
    if (current_token->type == TOKEN_IDENTIFIER) {
        return (ASTNode*)parse_assignment_statement(); // 我们需要一个新的解析函数
    }

    // 如果是左大括号，说明是一个代码块
    if (current_token->type == TOKEN_LBRACE) {
        return parse_block_statement();
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
    eat(TOKEN_KEYWORD); // 消费 "int"

    // --- 核心修正：不再使用 strdup ---
    // 1. 先保存指针
    char* function_name = current_token->value;
    // 2. 再消费 token
    eat(TOKEN_IDENTIFIER);

    eat(TOKEN_LPAREN);
    eat(TOKEN_RPAREN);

    BlockStatementNode* body = (BlockStatementNode*)parse_block_statement();
    
    // 3. 将保存的指针传递给 AST 节点
    return create_function_declaration_node(function_name, body);
}

// 解析 If 语句: "if" "(" <expression> ")" <statement>
ASTNode* parse_if_statement() {
    // TODO:
    // 1. 消费 "if" 关键字
    eat(TOKEN_KEYWORD); // 消费 "if"
    // 2. 消费 "("
    eat(TOKEN_LPAREN);
    // 3. 解析括号内的条件表达式 (调用 parse_expression())
    ASTNode* condition = parse_expression();
    // 4. 消费 ")"
    eat(TOKEN_RPAREN);
    // 5. 解析 if 为真时要执行的语句 (调用 parse_statement())
    ASTNode* body = parse_statement();
    //    注意：这里允许 if (x>2) y=3; 这种单语句，也允许 if (x>2) { ... } 这种代码块
    //    而 parse_statement() 恰好可以解析这两种情况！
    // 6. 创建并返回一个 IfStatementNode
    return (ASTNode*)create_if_statement_node(condition, body);
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

// 创建一个 “变量声明” 节点
VarDeclNode* create_var_decl_node(char* name, ASTNode* initial_value){
    VarDeclNode* node = (VarDeclNode*)malloc(sizeof(VarDeclNode));
    if (!node) { exit(1); }
    node->type = NODE_VAR_DECL;
    node->name = name;                      // 接管 name 指针
    node->initial_value = initial_value;    // 接管 body 指针
    return node;
}

// 创建一个 “标识符” 节点
IdentifierNode* create_identifier_node(char* name){
    IdentifierNode* node = (IdentifierNode*)malloc(sizeof(IdentifierNode));
    if (!node) { exit(1); }
    node->type = NODE_IDENTIFIER;
    node->name = name;
    return node;
}

// 创建一个 “二元运算” 节点
BinaryOpNode* create_binary_op_node(ASTNode* left, TokenType op, ASTNode* right){
    BinaryOpNode* node = (BinaryOpNode*)malloc(sizeof(BinaryOpNode));
    if (!node) { exit(1); }
    node->type = NODE_BINARY_OP;
    node->left = left;
    node->op = op;
    node->right = right;
    return node;
}

IfStatementNode* create_if_statement_node(ASTNode* condition, ASTNode* body){
    IfStatementNode* node = (IfStatementNode*)malloc(sizeof(IfStatementNode));
    if (!node) { exit(1); }
    node->type = NODE_IF_STATEMENT;
    node->body = body;
    node->condition = condition;
    return node;
}