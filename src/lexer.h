#ifndef LEXER_H
#define LEXER_H

// --- 从 main.c 移动过来的 ---

typedef enum {
    TOKEN_INT,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_EOF,
    TOKEN_UNKNOWN
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