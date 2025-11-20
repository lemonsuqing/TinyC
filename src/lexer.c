#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "lexer.h" // 引入我们自己的头文件

// --- 从 main.c 移动过来的 ---

// 全局变量，用于指向当前正在分析的源代码位置
static char* source_code; // 使用 static 使其成为文件内私有变量
static int current_pos = 0;

Token* create_token(TokenType type, char* value) {
    Token* token = (Token*)malloc(sizeof(Token));
    token->type = type;
    token->value = value;
    return token;
}

// 初始化词法分析器
void lexer_init(char* source) {
    source_code = source;
    current_pos = 0;
}

Token* get_next_token() {
    // 这部分代码和你写的一样，直接从你之前的 main.c 复制过来即可
    if (source_code[current_pos] == '\0') {
        return create_token(TOKEN_EOF, "");
    }
    while(isspace(source_code[current_pos])){
        current_pos++;
    }
    if (source_code[current_pos] == '\0') {
        return create_token(TOKEN_EOF, "");
    }
    if (source_code[current_pos] == '{') {
        current_pos++;
        return create_token(TOKEN_LBRACE, "{");
    }
    if (source_code[current_pos] == '}') {
        current_pos++;
        return create_token(TOKEN_RBRACE, "}");
    }
    if (isdigit(source_code[current_pos])) {
        int start = current_pos;
        while (isdigit(source_code[current_pos])){
            current_pos++;
        }
        int len = current_pos - start;
        char* num_str = (char*)malloc(len + 1);
        strncpy(num_str, source_code + start, len);
        num_str[len] = '\0';
        return create_token(TOKEN_INT, num_str);
    }
    current_pos++;
    return create_token(TOKEN_UNKNOWN, "");
}