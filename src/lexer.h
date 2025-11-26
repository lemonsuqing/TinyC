#ifndef LEXER_H
#define LEXER_H

// --- 从 main.c 移动过来的 ---

typedef enum {
    TOKEN_INT,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_EOF,
    TOKEN_UNKNOWN,
    TOKEN_IDENTIFIER,   // 标识符, e.g., main, my_variable
    TOKEN_KEYWORD,      // 关键字, e.g., int, return
    TOKEN_LPAREN,       // (
    TOKEN_RPAREN,       // )
    TOKEN_SEMICOLON,    // ;
    TOKEN_ASSIGN,       // = :赋值
    TOKEN_PLUS,         // +
    TOKEN_MINUS,        // -
    TOKEN_GT,           // > (大于)
    TOKEN_STAR,         // *
    TOKEN_SLASH,        // /
    TOKEN_EQ,           // ==
    TOKEN_NEQ,          // !=
    TOKEN_LT,           // < (小于)
    TOKEN_LE,           // <=
    TOKEN_GE,           // >=
    TOKEN_BANG,         // ! (逻辑非)
} TokenType;

typedef struct {
    TokenType type;
    char* value;
} Token;

// --- 函数声明 ---
// 初始化词法分析器
void lexer_init(char* source);
// 获取下一个 token
Token* get_next_token();

#endif // LEXER_H