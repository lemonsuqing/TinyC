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
ASTNode* parse_factor();
ASTNode* parse_if_statement();
ASTNode* parse_assignment_statement();
ASTNode* parse_while_statement();
ASTNode* parse_for_statement();
ASTNode* parse_unary();
ASTNode** parse_parameter_list(int* count);
ASTNode* parse_logical_or();
ASTNode* parse_logical_and();
void parse_struct_definition();

// -----------
// 辅助函数
// -----------


// ===

// 解析类型关键字
DataType parse_type() {
    if (current_token->type == TOKEN_KEYWORD) {
        if (strcmp(current_token->value, "int") == 0) {
            eat(TOKEN_KEYWORD);
            return TYPE_INT;
        }
        if (strcmp(current_token->value, "char") == 0) {
            eat(TOKEN_KEYWORD);
            return TYPE_CHAR;
        }
    }
    fprintf(stderr, "Syntax Error: Expected type specifier (int, char)\n");
    exit(1);
    return TYPE_INT;
}

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
ASTNode* parse_factor() {
    ASTNode* node = NULL;
    char* value_ptr = current_token->value;
    TokenType type = current_token->type;

    if (type == TOKEN_INT) {
        node = (ASTNode*)create_numeric_literal(value_ptr);
        eat(TOKEN_INT);
    } 
    else if (type == TOKEN_LPAREN) {
        // 1. 遇到左括号，先吃掉
        eat(TOKEN_LPAREN);
        
        // 2. 递归调用 parse_expression 来解析括号里面的内容
        //    这会从最低优先级（加减）重新开始解析，非常完美
        node = parse_expression();
        
        // 3. 解析完里面后，必须期望一个右括号
        eat(TOKEN_RPAREN);
    } 
    else if (type == TOKEN_IDENTIFIER) {
        // 关键点：我们需要向前看一个 token
        // 如果是 '('，则是函数调用；否则是变量。
        // 但我们现在的架构 get_next_token 会覆盖 current_token。
        // 简单的办法：先创建 ID 节点，然后看下一个是不是 '('。
        // 更好的办法：在 Lexer 里增加 peek() 功能，或者在这里做一个小 trick。
        
        // 这里的 trick：我们先保存名字，eat(ID)，然后看 current_token
        char* name = current_token->value;
        eat(TOKEN_IDENTIFIER);

        if (current_token->type == TOKEN_DOT) {
            // --- 处理 p.x ---
            eat(TOKEN_DOT);
            char* member_name = current_token->value;
            eat(TOKEN_IDENTIFIER);
            return (ASTNode*)create_member_access_node(name, member_name);
        }

        if (current_token->type == TOKEN_LPAREN) {
            // --- 这是函数调用 ---
            eat(TOKEN_LPAREN);
            
            ASTNode** args = NULL;
            int arg_count = 0;

            if (current_token->type != TOKEN_RPAREN) {
                while (1) {
                    ASTNode* expr = parse_expression();
                    arg_count++;
                    args = realloc(args, sizeof(ASTNode*) * arg_count);
                    args[arg_count - 1] = expr;

                    if (current_token->type == TOKEN_COMMA) {
                        eat(TOKEN_COMMA);
                    } else {
                        break;
                    }
                }
            }
            eat(TOKEN_RPAREN);
            return (ASTNode*)create_function_call_node(name, args, arg_count);
        } 
        else if (current_token->type == TOKEN_LBRACKET) {
            eat(TOKEN_LBRACKET);
            ASTNode* index = parse_expression(); // 解析索引 (支持 a[x+1])
            eat(TOKEN_RBRACKET);
            return (ASTNode*)create_array_access_node(name, index);
        }
        else {
            // --- 这是普通变量 ---
            return (ASTNode*)create_identifier_node(name);
        }
    }
    else if (type == TOKEN_STRING) {
        char* val = current_token->value;
        eat(TOKEN_STRING);
        return (ASTNode*)create_string_literal_node(val);
    }
    else if (type == TOKEN_CHAR) {
        // Lexer 已经把 'A' 变成了 "65"，直接当数字节点创建即可
        ASTNode* node = (ASTNode*)create_numeric_literal(current_token->value);
        eat(TOKEN_CHAR);
        return node;
    }
    else {
        fprintf(stderr, "Syntax Error: Expected number, identifier or '(', but got token type %d\n", type);
        exit(1);
    }
    
    return node;
}

ASTNode* parse_unary() {
    // 检查当前是不是一元操作符 (+, -, !)
    if (current_token->type == TOKEN_PLUS || 
        current_token->type == TOKEN_MINUS || 
        current_token->type == TOKEN_BANG || 
        current_token->type == TOKEN_AMPERSAND ||
        current_token->type == TOKEN_STAR) {
        
        TokenType op = current_token->type;
        eat(op);
        
        // 递归调用自己！因为可能出现 - -5 或 ! ! x 这种情况
        ASTNode* operand = parse_unary();
        
        return (ASTNode*)create_unary_op_node(op, operand);
    }
    
    // 如果不是一元操作符，那就说明是基础因子 (数字/变量/括号)
    // 把控制权交给下一层
    return parse_factor();
}

// 新增：处理乘法和除法
ASTNode* parse_term() {
    // 1. 先解析一个因子 (Factor)
    ASTNode* left = parse_unary();

    // 2. 只要后面跟着 * 或 /，就继续吃
    while (current_token->type == TOKEN_STAR || current_token->type == TOKEN_SLASH) {
        TokenType op = current_token->type;
        eat(op);
        ASTNode* right = parse_unary();
        left = (ASTNode*)create_binary_op_node(left, op, right);
    }
    return left;
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
    // 1. 先解析左侧 (加减法优先)
    ASTNode* left = parse_additive_expression();

    // 2. 检查是否有比较操作符
    while (current_token->type == TOKEN_GT || current_token->type == TOKEN_LT ||
           current_token->type == TOKEN_EQ || current_token->type == TOKEN_NEQ ||
           current_token->type == TOKEN_LE || current_token->type == TOKEN_GE) {
        
        TokenType op = current_token->type;
        eat(op);
        
        // 3. 解析右侧
        ASTNode* right = parse_additive_expression();
        
        // 4. 组合
        left = (ASTNode*)create_binary_op_node(left, op, right);
    }
    // 3. 返回最终的表达式树
    return left;
}

// 1. 最顶层的 parse_expression 现在指向 logical_or (优先级最低)
ASTNode* parse_expression() {
    return parse_logical_or();
}

// 2. 解析 ||
ASTNode* parse_logical_or() {
    // 先解析优先级更高的 &&
    ASTNode* left = parse_logical_and();

    while (current_token->type == TOKEN_LOGIC_OR) {
        TokenType op = current_token->type;
        eat(TOKEN_LOGIC_OR);
        ASTNode* right = parse_logical_and();
        // 仍然使用 BinaryOpNode，因为结构是一样的，只是 op 不同
        left = (ASTNode*)create_binary_op_node(left, op, right);
    }
    return left;
}

// 3. 解析 &&
ASTNode* parse_logical_and() {
    // 先解析优先级更高的比较运算 (==, !=, <, >)
    // 注意：你之前的 parse_comparison_expression 包含了 == 和 < 等
    // 如果你没有把 == 和 < 分开，那就直接调 parse_comparison_expression
    ASTNode* left = parse_comparison_expression();

    while (current_token->type == TOKEN_LOGIC_AND) {
        TokenType op = current_token->type;
        eat(TOKEN_LOGIC_AND);
        ASTNode* right = parse_comparison_expression();
        left = (ASTNode*)create_binary_op_node(left, op, right);
    }
    return left;
}

// 解析变量声明语句: "int" <identifier> "=" <expression> ";"
ASTNode* parse_variable_declaration() {
    // 检查是不是 struct 关键字
    if (current_token->type == TOKEN_KEYWORD && strcmp(current_token->value, "struct") == 0) {
        eat(TOKEN_KEYWORD);
        char* struct_name = current_token->value;
        eat(TOKEN_IDENTIFIER);
        char* var_name = current_token->value;
        eat(TOKEN_IDENTIFIER);
        eat(TOKEN_SEMICOLON);
        
        // 查找结构体定义，获取大小
        StructDef* s = find_struct(struct_name);
        if (!s) { fprintf(stderr, "Undefined struct %s\n", struct_name); exit(1); }
        
        // 我们利用 array_size 字段来存结构体大小？
        // 或者使用上面新增的 array_size 来存总字节数？
        // 这里的技巧是：把结构体当成一个巨大的 int 数组或者 byte 数组处理
        // 传入 struct_name
        return (ASTNode*)create_var_decl_node(var_name, NULL, s->size / 8, TYPE_STRUCT, struct_name); 
        // 注意：这里我复用了 array_size 字段来告诉 codegen 分配多少个 8字节。
        // 你也可以在 VarDeclNode 里加个 size 字段更清晰。
    }

    DataType var_type = parse_type(); // 吃掉 "int"、"char"

    char* variable_name = current_token->value;
    eat(TOKEN_IDENTIFIER);

    int array_size = 0;
    ASTNode* expr = NULL;

    // 检查是不是数组: int a[10];
    if (current_token->type == TOKEN_LBRACKET) {
        eat(TOKEN_LBRACKET);
        if (current_token->type != TOKEN_INT) {
            fprintf(stderr, "Error: Array size must be a constant integer.\n");
            exit(1);
        }
        array_size = atoi(current_token->value);
        eat(TOKEN_INT);
        eat(TOKEN_RBRACKET);
        
        // 数组声明通常直接跟分号
        eat(TOKEN_SEMICOLON);
    } else {
        // 普通变量: int a = 10;
        if (current_token->type == TOKEN_ASSIGN) {
            eat(TOKEN_ASSIGN);
            expr = parse_expression(); 
        }
        eat(TOKEN_SEMICOLON);
    }

    // 传入 array_size 参数
    return (ASTNode*)create_var_decl_node(variable_name, expr, array_size, var_type, NULL);
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
    if (current_token->type == TOKEN_KEYWORD && 
       (strcmp(current_token->value, "int") == 0 || 
        strcmp(current_token->value, "char") == 0 ||
        strcmp(current_token->value, "struct") == 0)) {
        return parse_variable_declaration();
    }

    // 如果是 "if" 关键字
    if (strcmp(current_token->value, "if") == 0) {
        return parse_if_statement();
    }

    // 如果是 "while" 关键字
    if (strcmp(current_token->value, "while") == 0) {
        return parse_while_statement();
    }

    // 如果是 "for" 关键字
    if (strcmp(current_token->value, "for") == 0) {
        return parse_for_statement();
    }

    if (strcmp(current_token->value, "break") == 0) {
        eat(TOKEN_KEYWORD);
        eat(TOKEN_SEMICOLON);
        return create_break_node();
    }
    
    if (strcmp(current_token->value, "continue") == 0) {
        eat(TOKEN_KEYWORD);
        eat(TOKEN_SEMICOLON);
        return create_continue_node();
    }

    // 注意：赋值语句 (x = 5;) 也是一种语句，我们需要在这里处理
    if (current_token->type == TOKEN_IDENTIFIER || current_token->type == TOKEN_STAR) {
        // 1. 先解析左边的部分 (x 或 *p 或 add())
        // parse_expression 会自动处理优先级，解析出 *p 这个节点
        ASTNode* left = parse_expression();

        // 2. 检查后面是不是赋值号 '='
        if (current_token->type == TOKEN_ASSIGN) {
            // 是赋值语句: x = ... 或 *p = ...
            eat(TOKEN_ASSIGN);
            ASTNode* right = parse_expression();
            eat(TOKEN_SEMICOLON);
            
            // 返回一个二元操作节点 (ASSIGN)，左边是 x 或 *p，右边是值
            return (ASTNode*)create_binary_op_node(left, TOKEN_ASSIGN, right);
        } else {
            // 不是赋值，那可能是一个函数调用语句 add(1); 
            // 或者是单纯的表达式 x; (虽然没啥用但在C里合法)
            eat(TOKEN_SEMICOLON);
            return left;
        }
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

void parse_struct_definition() {
    eat(TOKEN_KEYWORD); // struct
    char* struct_name = current_token->value;
    eat(TOKEN_IDENTIFIER);
    eat(TOKEN_LBRACE); // {
    
    StructDef* s = define_struct(struct_name);
    
    // 解析成员
    while (current_token->type != TOKEN_RBRACE) {
        // 简化：成员只能是 int x; 或 char y; 不支持嵌套 struct
        DataType type = parse_type();
        char* mem_name = current_token->value;
        eat(TOKEN_IDENTIFIER);
        eat(TOKEN_SEMICOLON);
        
        add_struct_member(s, mem_name, type);
    }
    
    eat(TOKEN_RBRACE); // }
    eat(TOKEN_SEMICOLON); // ;
}

// 新函数：用于解析顶层内容（可能是函数，可能是全局变量）
ASTNode* parse_top_level() {
    // --- 新增：检查是不是 struct 定义 ---
    if (current_token->type == TOKEN_KEYWORD && strcmp(current_token->value, "struct") == 0) {
        parse_struct_definition();
        return NULL; // <--- 关键！返回 NULL 表示这只是元数据定义，不是可执行代码
    }

    // 1. 先解析类型
    DataType type = parse_type();

    // 2. 名字
    char* name = current_token->value;
    eat(TOKEN_IDENTIFIER);

    // 3. 关键判断：向前看一个 Token
    if (current_token->type == TOKEN_LPAREN) {
        // --- 情况 A: 是函数声明 ---
        eat(TOKEN_LPAREN);

        int arg_count = 0;
        ASTNode** args = parse_parameter_list(&arg_count);

        eat(TOKEN_RPAREN);

        BlockStatementNode* body = (BlockStatementNode*)parse_block_statement();
        
        // 创建并返回函数节点
        return (ASTNode*)create_function_declaration_node(name, args, arg_count, body);
    } 
    else {
        // --- 变量声明逻辑更新 ---
        int array_size = 0;
        ASTNode* init_expr = NULL;

        // 检查是不是数组声明: int a[10];
        if (current_token->type == TOKEN_LBRACKET) {
            eat(TOKEN_LBRACKET);
            // 这里为了简化，我们只支持数字字面量定义大小
            if (current_token->type != TOKEN_INT) {
                fprintf(stderr, "Error: Array size must be a constant integer.\n");
                exit(1);
            }
            array_size = atoi(current_token->value); // 获取大小
            eat(TOKEN_INT);
            eat(TOKEN_RBRACKET);
            
            // 数组声明后通常直接跟分号
            eat(TOKEN_SEMICOLON);
        } else {
            // 普通变量逻辑: int a = 10;
            if (current_token->type == TOKEN_ASSIGN) {
                eat(TOKEN_ASSIGN);
                init_expr = parse_expression(); 
            }
            eat(TOKEN_SEMICOLON);
        }
        
        // 传入 array_size
        return (ASTNode*)create_var_decl_node(name, init_expr, array_size, type, NULL);
    }
}

// 解析参数列表: (int x, int y)
// 返回 VarDeclNode* 的数组，通过指针参数返回 count
ASTNode** parse_parameter_list(int* count) {
    *count = 0;
    if (current_token->type == TOKEN_RPAREN) {
        return NULL; // 空参数列表
    }

    ASTNode** args = NULL;
    
    // 类似于 do-while 结构，处理 "类型 变量名" + ","
    while (1) {
        // 1. 解析类型 (int)
        DataType type = parse_type();

        // 2. 解析参数名
        char* param_name = current_token->value;
        eat(TOKEN_IDENTIFIER);

        // 3. 创建参数节点 (复用 VarDeclNode，虽然没有初始值，但在 AST 中可以视作声明)
        // 注意：这里 initial_value 传 NULL
        VarDeclNode* param = create_var_decl_node(param_name, NULL, 0, type, NULL);

        // 4. 添加到数组
        (*count)++;
        args = realloc(args, sizeof(ASTNode*) * (*count));
        args[(*count) - 1] = (ASTNode*)param;

        // 5. 如果是逗号，继续；如果是右括号，结束
        if (current_token->type == TOKEN_COMMA) {
            eat(TOKEN_COMMA);
        } else {
            break;
        }
    }
    return args;
}

// 解析 If 语句: "if" "(" <expression> ")" <statement>
ASTNode* parse_if_statement() {
    // TODO:
    // 消费 "if" 关键字
    eat(TOKEN_KEYWORD); // 消费 "if"
    // 消费 "("
    eat(TOKEN_LPAREN);
    // 解析括号内的条件表达式 (调用 parse_expression())
    ASTNode* condition = parse_expression();
    // 消费 ")"
    eat(TOKEN_RPAREN);
    // 解析 if 为真时要执行的语句 (调用 parse_statement())
    ASTNode* body = parse_statement();

    // 检查是否存在 "else" 分支
    ASTNode* else_body = NULL;

    // 此时 current_token 指向 then_body 之后的第一个 token
    // 我们检查它是不是关键字 "else"
    if (current_token->type == TOKEN_KEYWORD && strcmp(current_token->value, "else") == 0) {
        // 既然存在 else，我们就要消费它
        eat(TOKEN_KEYWORD);
        // 然后解析 else 后面的语句
        else_body = parse_statement();
    }

    //    注意：这里允许 if (x>2) y=3; 这种单语句，也允许 if (x>2) { ... } 这种代码块
    //    而 parse_statement() 恰好可以解析这两种情况！
    // 6. 创建并返回一个 IfStatementNode
    return (ASTNode*)create_if_statement_node(condition, body, else_body);
}

// 解析 while 语句
ASTNode* parse_while_statement() {
    eat(TOKEN_KEYWORD); // 消费 "while"
    eat(TOKEN_LPAREN);
    ASTNode* condition = parse_expression();
    eat(TOKEN_RPAREN);
    ASTNode* body = parse_statement();
    return (ASTNode*)create_while_statement_node(condition, body);
}

// 新增 parse_for_statement
ASTNode* parse_for_statement() {
    eat(TOKEN_KEYWORD); // for
    eat(TOKEN_LPAREN);  // (
    
    // 1. 初始化部分
    ASTNode* init = NULL;
    if (current_token->type != TOKEN_SEMICOLON) {
        if (current_token->type == TOKEN_KEYWORD && strcmp(current_token->value, "int") == 0) {
            init = parse_variable_declaration(); 
        } else {
            // 这里为了支持 i=0 这种赋值表达式，我们手动处理一下
            ASTNode* left = parse_expression();
            if (current_token->type == TOKEN_ASSIGN) {
                eat(TOKEN_ASSIGN);
                ASTNode* right = parse_expression();
                init = (ASTNode*)create_binary_op_node(left, TOKEN_ASSIGN, right);
            } else {
                init = left;
            }
            eat(TOKEN_SEMICOLON); 
        }
    } else {
        eat(TOKEN_SEMICOLON);
    }

    // 2. 条件部分
    ASTNode* cond = NULL;
    if (current_token->type != TOKEN_SEMICOLON) {
        cond = parse_expression();
    }
    eat(TOKEN_SEMICOLON);

    // 3. 递增部分 (i = i + 1)
    ASTNode* inc = NULL;
    if (current_token->type != TOKEN_RPAREN) {
        // [关键修改]：手动检查是否是赋值操作
        // 因为 parse_expression 目前不包含赋值逻辑
        
        ASTNode* left = parse_expression(); // 先解析 'i'
        
        if (current_token->type == TOKEN_ASSIGN) {
            // 如果后面跟的是 '='，说明是赋值
            TokenType op = current_token->type; // TOKEN_ASSIGN
            eat(TOKEN_ASSIGN);
            ASTNode* right = parse_expression(); // 解析 'i + 1'
            // 组合成赋值节点
            inc = (ASTNode*)create_binary_op_node(left, op, right);
        } else {
            // 否则就是普通表达式
            inc = left;
        }
    }
    eat(TOKEN_RPAREN); // )

    // 4. 循环体
    ASTNode* body = parse_statement();

    return (ASTNode*)create_for_statement_node(init, cond, inc, body);
}

// -----------
// 语法分析器主入口
// -----------

ASTNode* parse() {
    current_token = get_next_token();
    ProgramNode* prog = create_program_node();

    // 循环直到文件结束
    while (current_token->type != TOKEN_EOF) {
        // [修改后] 调用新的通用解析函数
        ASTNode* node = parse_top_level();
        if (node != NULL) {
            add_declaration_to_program(prog, node);
        }
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
FunctionDeclarationNode* create_function_declaration_node(char* name, struct ASTNode** args, int arg_count, BlockStatementNode* body) {
    FunctionDeclarationNode* node = (FunctionDeclarationNode*)malloc(sizeof(FunctionDeclarationNode));
    if (!node) { exit(1); }
    node->type = NODE_FUNCTION_DECL;
    node->name = name; // 接管 name 指针
    node->body = body; // 接管 body 指针
    node->args = args;       // <--- 新增
    node->arg_count = arg_count; // <--- 新增
    return node;
}

FunctionCallNode* create_function_call_node(char* name, ASTNode** args, int arg_count) {
    FunctionCallNode* node = (FunctionCallNode*)malloc(sizeof(FunctionCallNode));
    if (!node) exit(1);
    node->type = NODE_FUNCTION_CALL;
    node->name = name;
    node->args = args;
    node->arg_count = arg_count;
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
void add_declaration_to_program(ProgramNode* prog, struct ASTNode* decl) {
    prog->count++;
    prog->declarations = realloc(prog->declarations, prog->count * sizeof(ASTNode*));
    if (!prog->declarations) { exit(1); }
    prog->declarations[prog->count - 1] = decl;
}

// 创建一个 “变量声明” 节点
VarDeclNode* create_var_decl_node(char* name, ASTNode* initial_value, int array_size, DataType var_type, char* struct_name){
    VarDeclNode* node = (VarDeclNode*)malloc(sizeof(VarDeclNode));
    if (!node) { exit(1); }
    node->type = NODE_VAR_DECL;
    node->name = name;                      // 接管 name 指针
    node->initial_value = initial_value;    // 接管 body 指针
    node->array_size = array_size;
    node->var_type = var_type;
    node->struct_name = struct_name;
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

// 创建一个 “if” 语句节点
IfStatementNode* create_if_statement_node(ASTNode* condition, ASTNode* body, ASTNode* else_branch){
    IfStatementNode* node = (IfStatementNode*)malloc(sizeof(IfStatementNode));
    if (!node) { exit(1); }
    node->type = NODE_IF_STATEMENT;
    node->body = body;
    node->condition = condition;
    node->else_branch = else_branch;
    return node;
}

// 创建一个 “while” 语句节点
WhileStatementNode* create_while_statement_node(ASTNode* condition, ASTNode* body){
    WhileStatementNode* node = (WhileStatementNode*)malloc(sizeof(WhileStatementNode));
    if (!node) { exit(1); }
    node->type = NODE_WHILE_STATEMENT;
    node->body = body;
    node->condition = condition;
    return node;
}

// 创建一个 一元操作符 节点
UnaryOpNode* create_unary_op_node(TokenType op, ASTNode* operand){
    UnaryOpNode* node = (UnaryOpNode*)malloc(sizeof(UnaryOpNode));
    if (!node) { exit(1); }
    node->type = NODE_UNARY_OP;
    node->op = op;
    node->operand = operand;
    return node;
}

ArrayAccessNode* create_array_access_node(char* name, ASTNode* index) {
    ArrayAccessNode* node = (ArrayAccessNode*)malloc(sizeof(ArrayAccessNode));
    if (!node) { exit(1); }
    node->type = NODE_ARRAY_ACCESS;
    node->array_name = name;
    node->index = index;
    return node;
}

StringLiteralNode* create_string_literal_node(char* value) {
    StringLiteralNode* node = (StringLiteralNode*)malloc(sizeof(StringLiteralNode));
    if (!node) exit(1);
    node->type = NODE_STRING_LITERAL;
    node->value = value;
    node->original_id = -1; // 初始化为 -1，生成代码时再分配
    return node;
}

ForStatementNode* create_for_statement_node(ASTNode* init, ASTNode* cond, ASTNode* inc, ASTNode* body) {
    ForStatementNode* node = malloc(sizeof(ForStatementNode));
    if (!node) exit(1);
    node->type = NODE_FOR_STATEMENT;
    node->init = init;
    node->condition = cond;
    node->increment = inc;
    node->body = body;
    return node;
}

ASTNode* create_break_node() {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (!node) exit(1);
    node->type = NODE_BREAK;
    return node;
}
ASTNode* create_continue_node() {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (!node) exit(1);
    node->type = NODE_CONTINUE;
    return node;
}

StructDef struct_table[MAX_STRUCTS];
int struct_count = 0;

// 定义一个新的结构体
StructDef* define_struct(char* name) {
    if (struct_count >= MAX_STRUCTS) {
        fprintf(stderr, "Error: Too many structs defined.\n");
        exit(1);
    }
    StructDef* s = &struct_table[struct_count++];
    s->name = name;
    s->member_count = 0;
    s->size = 0;
    return s;
}

// 向结构体添加成员
void add_struct_member(StructDef* s, char* member_name, DataType type) {
    if (s->member_count >= MAX_MEMBERS) {
        fprintf(stderr, "Error: Too many members in struct %s.\n", s->name);
        exit(1);
    }
    MemberInfo* m = &s->members[s->member_count++];
    m->name = member_name;
    m->type = type;
    
    // 简化处理：目前所有成员（包括 char）都按 8 字节对齐分配
    // 这样 Codegen 计算地址最简单
    m->offset = s->size; 
    s->size += 8; 
}

// 查找结构体定义
StructDef* find_struct(char* name) {
    for(int i=0; i<struct_count; i++) {
        if(strcmp(struct_table[i].name, name) == 0) {
            return &struct_table[i];
        }
    }
    return NULL;
}

// 查找结构体成员
MemberInfo* find_struct_member(StructDef* s, char* member_name) {
    for(int i=0; i<s->member_count; i++) {
        if(strcmp(s->members[i].name, member_name) == 0) {
            return &s->members[i];
        }
    }
    return NULL;
}

MemberAccessNode* create_member_access_node(char* var_name, char* member_name) {
    MemberAccessNode* node = (MemberAccessNode*)malloc(sizeof(MemberAccessNode));
    if (!node) { exit(1); }
    node->type = NODE_MEMBER_ACCESS;
    node->struct_var_name = var_name;
    node->member_name = member_name;
    return node;
}
