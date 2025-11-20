// main.c

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

// ---------------------------------
// 词法分析器 (Lexical Analyzer) 部分
// ---------------------------------

// 1. 定义 Token 类型
typedef enum {
    TOKEN_INT,      // 整数字面量
    TOKEN_LBRACE,   // {
    TOKEN_RBRACE,   // }
    TOKEN_EOF,      // 文件结束符 (End of File)
    TOKEN_UNKNOWN   // 未知类型
} TokenType;

// 2. 定义 Token 结构体
typedef struct {
    TokenType type; // Token 的类型
    char* value;    // Token 的原始值 (例如 "123", "{")
} Token;

// 全局变量，用于指向当前正在分析的源代码位置
char* source_code;
int current_pos = 0;

// 函数：创建一个新的 Token
Token* create_token(TokenType type, char* value) {
    Token* token = (Token*)malloc(sizeof(Token));
    token->type = type;
    token->value = value;
    return token;
}

// 函数：获取下一个 Token
// 这是你需要实现的核心部分！
Token* get_next_token() {
    // 当我们读到源代码的末尾时，就返回 EOF Token
    if (source_code[current_pos] == '\0') {
        return create_token(TOKEN_EOF, "");
    }

    // TODO: 跳过所有的空白字符 (空格, 换行, 制表符等)
    // 你可以使用 isspace() 函数
    // while (...) { ... }
    while(isspace(source_code[current_pos])){
        current_pos++;
    }

    // TODO: 识别左大括号 '{'
    if (source_code[current_pos] == '{') {
        current_pos++;
        return create_token(TOKEN_LBRACE, "{");
    }

    // TODO: 识别右大括号 '}'
    // if (...) { ... }
    if (source_code[current_pos] == '}') {
        current_pos++;
        return create_token(TOKEN_RBRACE, "}");
    }


    // TODO: 识别一个或多个连续的数字，并将其作为一个整数 Token
    // 你可以使用 isdigit() 函数
    if (isdigit(source_code[current_pos])) {
    //     ...
    //     // 你需要记录数字的起始位置，然后找到结束位置
    //     // 截取这部分字符串，并创建一个 TOKEN_INT 类型的 Token
        int start = current_pos;
        while (isdigit(source_code[current_pos])){
            current_pos++;
        }
        int len = current_pos - start;
        char* num_str = (char*)malloc(len + 1);
        strncpy(num_str, source_code + start, len);
        num_str[len] = '\0'; // 手动添加字符串结束符
        return create_token(TOKEN_INT, num_str);
    }

    // 如果以上都不是，那么就是一个我们无法识别的 Token
    current_pos++;
    return create_token(TOKEN_UNKNOWN, "");
}


// ---------------------------------
// 主函数 (main) 部分
// ---------------------------------
int main() {
    // 我们要分析的源代码
    source_code = "{ 123 }";
    printf("正在分析: %s\n", source_code);

    // 不断调用 get_next_token() 直到我们拿到 EOF Token
    while (1) {
        Token* token = get_next_token();
        
        printf("Token 类型: %d, 值: '%s'\n", token->type, token->value);

        // 检查是否到达文件末尾
        if (token->type == TOKEN_EOF) {
            free(token); // 释放最后一个 EOF token
            break;       // 然后安全退出循环
        }

        // 核心修正：处理内存泄漏
        // 如果 Token 是一个整数字面量，它的 value 是通过 malloc 创建的，需要单独释放
        if (token->type == TOKEN_INT) {
            free(token->value);
        }

        // 最后释放 Token 结构体本身
        // 对于 LBRACE 和 RBRACE，它们的 value 指向字符串字面量，不需要也不能 free
        free(token);
    }

    return 0;
}