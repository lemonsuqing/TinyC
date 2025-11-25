#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "lexer.h"

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

    // 1. 识别单字符 Token: '(', ')', ';'
    if (source_code[current_pos] == '('){
        current_pos++;
        return create_token(TOKEN_LPAREN, "(");
    }

    if (source_code[current_pos] == ')'){
        current_pos++;
        return create_token(TOKEN_RPAREN, ")");
    }

    if (source_code[current_pos] == ';'){
        current_pos++;
        return create_token(TOKEN_SEMICOLON, ";");
    }

    if (source_code[current_pos] == '=') {
        current_pos++;
        return create_token(TOKEN_ASSIGN, "=");
    }

    if (source_code[current_pos] == '+') {
        current_pos++;
        return create_token(TOKEN_PLUS, "+");
    }
    if (source_code[current_pos] == '-') {
        current_pos++;
        return create_token(TOKEN_MINUS, "-");
    }


    // 2. 识别标识符和关键字
    // C 语言的标识符以字母或下划线开头
    if (isalpha(source_code[current_pos]) || source_code[current_pos] == '_') {
        // TODO:
        // a. 记录起始位置
        int start = current_pos;
        // b. 循环读取所有连续的字母、数字、下划线 (isalnum() 会很有用)
        while (isalnum(source_code[current_pos]) || source_code[current_pos] == '_') {
            current_pos++;
        }
        // c. 截取这个单词的子字符串 (像处理数字时一样，记得 malloc)
        int len = current_pos - start;
        char* str = (char*)malloc(len+1);   // 给单词字符串创建空间
        strncpy(str, source_code + start, len);
        str[len] = '\0';
        // d. 检查这个字符串是不是关键字 ("int", "return")
        //    - 如果是，返回一个 TOKEN_KEYWORD 类型的 Token
        //    - 如果不是，返回一个 TOKEN_IDENTIFIER 类型的 Token
        if (strcmp(str, "int") == 0 || strcmp(str, "return") == 0) {
            // 是关键字：返回 TOKEN_KEYWORD 类型，值为关键字字符串
            return create_token(TOKEN_KEYWORD, str);
        } else {
            // 不是关键字：返回 TOKEN_IDENTIFIER 类型，值为标识符字符串
            return create_token(TOKEN_IDENTIFIER, str);
        }
    }
    current_pos++;
    return create_token(TOKEN_UNKNOWN, "");
}