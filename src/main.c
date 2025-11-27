#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "codegen.h"

// -----------
// 调试与清理函数
// -----------
void print_ast(ASTNode* node, int indent) {
    if (node == NULL) return;

    // 打印缩进
    for (int i = 0; i < indent; i++) printf("  ");

    switch (node->type) {
        case NODE_PROGRAM: {
            printf("Program:\n");
            ProgramNode* prog = (ProgramNode*)node;
            for (int i = 0; i < prog->count; i++) {
                print_ast((ASTNode*)prog->declarations[i], indent + 1);
            }
            break;
        }
        case NODE_FUNCTION_DECL: {
            FunctionDeclarationNode* func = (FunctionDeclarationNode*)node;
            printf("FunctionDeclaration: int %s()\n", func->name);
            // 打印参数 (如果有)
            if (func->arg_count > 0) {
                for (int i = 0; i < indent + 1; i++) printf("  ");
                printf("Args:\n");
                for (int i = 0; i < func->arg_count; i++) {
                    print_ast(func->args[i], indent + 2);
                }
            }
            print_ast((ASTNode*)func->body, indent + 1);
            break;
        }
        case NODE_BLOCK_STATEMENT: {
            printf("BlockStatement:\n");
            BlockStatementNode* block = (BlockStatementNode*)node;
            for (int i = 0; i < block->count; i++) {
                print_ast(block->statements[i], indent + 1);
            }
            break;
        }
        case NODE_VAR_DECL: {
            VarDeclNode* var = (VarDeclNode*)node;
            if (var->array_size > 0) {
                printf("VarDecl: int %s[%d] (Array)\n", var->name, var->array_size);
            } else {
                printf("VarDecl: int %s\n", var->name);
            }
            if (var->initial_value) {
                print_ast(var->initial_value, indent + 1);
            }
            break;
        }
        case NODE_RETURN_STATEMENT: {
            printf("ReturnStatement:\n");
            ReturnStatementNode* ret = (ReturnStatementNode*)node;
            print_ast(ret->argument, indent + 1);
            break;
        }
        case NODE_IF_STATEMENT: {
            printf("IfStatement:\n");
            IfStatementNode* if_stmt = (IfStatementNode*)node;
            print_ast(if_stmt->condition, indent + 1);
            print_ast(if_stmt->body, indent + 1);
            if (if_stmt->else_branch) {
                for (int i = 0; i < indent; i++) printf("  ");
                printf("Else:\n");
                print_ast(if_stmt->else_branch, indent + 1);
            }
            break;
        }
        case NODE_WHILE_STATEMENT: {
            printf("WhileStatement:\n");
            WhileStatementNode* while_stmt = (WhileStatementNode*)node;
            print_ast(while_stmt->condition, indent + 1);
            print_ast(while_stmt->body, indent + 1);
            break;
        }
        case NODE_BINARY_OP: {
            BinaryOpNode* bin = (BinaryOpNode*)node;
            // 简单打印操作符的枚举值，你也可以写个 switch 转成字符显示
            printf("BinaryOp (Token type: %d):\n", bin->op);
            print_ast(bin->left, indent + 1);
            print_ast(bin->right, indent + 1);
            break;
        }
        case NODE_UNARY_OP: {
            UnaryOpNode* unary = (UnaryOpNode*)node;
            printf("UnaryOp (Token type: %d):\n", unary->op);
            print_ast(unary->operand, indent + 1);
            break;
        }
        case NODE_FUNCTION_CALL: {
            FunctionCallNode* call = (FunctionCallNode*)node;
            printf("FunctionCall: %s(...)\n", call->name);
            for (int i = 0; i < call->arg_count; i++) {
                print_ast(call->args[i], indent + 1);
            }
            break;
        }
        case NODE_ARRAY_ACCESS: {
            ArrayAccessNode* arr = (ArrayAccessNode*)node;
            printf("ArrayAccess: %s[...]\n", arr->array_name);
            print_ast(arr->index, indent + 1);
            break;
        }
        case NODE_IDENTIFIER: {
            IdentifierNode* ident = (IdentifierNode*)node;
            printf("Identifier: %s\n", ident->name);
            break;
        }
        case NODE_NUMERIC_LITERAL: {
            NumericLiteralNode* num_node = (NumericLiteralNode*)node;
            printf("NumericLiteral: %s\n", num_node->value);
            break;
        }
        case NODE_STRING_LITERAL: {
            StringLiteralNode* str_node = (StringLiteralNode*)node;
            printf("StringLiteral: \"%s\"\n", str_node->value);
            break;
        }
        default:
            printf("Unknown Node (Type: %d)\n", node->type);
    }
}

// 这是一个完整且正确的 free_ast 函数
void free_ast(ASTNode* node) {
    if (node == NULL) return;

    switch (node->type) {
        case NODE_PROGRAM: {
            ProgramNode* prog = (ProgramNode*)node;
            for (int i = 0; i < prog->count; i++) {
                free_ast((ASTNode*)prog->declarations[i]);
            }
            free(prog->declarations);
            break;
        }
        case NODE_BLOCK_STATEMENT: {
            BlockStatementNode* block = (BlockStatementNode*)node;
            for (int i = 0; i < block->count; i++) {
                free_ast(block->statements[i]);
            }
            free(block->statements);
            break;
        }
        case NODE_VAR_DECL: { // <--- 新增 case
            VarDeclNode* var_decl = (VarDeclNode*)node;
            free(var_decl->name); // 释放变量名
            free_ast(var_decl->initial_value); // 递归释放初始值
            break;
        }
        case NODE_RETURN_STATEMENT: {
            ReturnStatementNode* ret = (ReturnStatementNode*)node;
            free_ast(ret->argument);
            break;
        }
        case NODE_IDENTIFIER: { // <--- 新增 case
            IdentifierNode* ident = (IdentifierNode*)node;
            free(ident->name); // 释放标识符名
            break;
        }
        case NODE_NUMERIC_LITERAL: {
            NumericLiteralNode* num_node = (NumericLiteralNode*)node;
            free(num_node->value);
            break;
        }
        case NODE_BINARY_OP: {
            BinaryOpNode* bin = (BinaryOpNode*)node;
            free_ast(bin->left);
            free_ast(bin->right);
            break;
        }
        case NODE_IF_STATEMENT: { // 假设你有这个 case，如果没有请加上
            IfStatementNode* if_stmt = (IfStatementNode*)node;
            free_ast(if_stmt->condition);
            free_ast(if_stmt->body);
            if (if_stmt->else_branch != NULL) { // 递归释放 else
                free_ast(if_stmt->else_branch);
            }
            break;
        }
        case NODE_WHILE_STATEMENT: {
            WhileStatementNode* while_stmt = (WhileStatementNode*)node;
            free_ast(while_stmt->condition);
            free_ast(while_stmt->body);
            break;
        }
        case NODE_UNARY_OP: {
            UnaryOpNode* unary = (UnaryOpNode*)node;
            free_ast(unary->operand);
            break;
        }
        case NODE_FUNCTION_DECL: {
            FunctionDeclarationNode* func = (FunctionDeclarationNode*)node;
            free(func->name);
            
            // --- 新增：释放参数列表 ---
            // 1. 遍历释放每一个参数节点 (VarDeclNode)
            for (int i = 0; i < func->arg_count; i++) {
                free_ast(func->args[i]);
            }
            // 2. 释放存放指针的数组本身
            if (func->args != NULL) {
                free(func->args);
            }
            
            free_ast((ASTNode*)func->body);
            break;
        }
        case NODE_ARRAY_ACCESS: {
            ArrayAccessNode* arr_access = (ArrayAccessNode*)node;
            free(arr_access->array_name);  // 释放数组名（动态分配的字符串）
            free_ast(arr_access->index);   // 递归释放索引表达式（可能是数字、标识符等节点）
            break;
        }
        default:
            break;
    }
    free(node);
}

// -----------
// 主函数
// -----------
int main() {
    char* source_code = 
        "// 这是整个文件的头部注释（行首注释）\n" // OK
        "int main() { "
        "  int a = 0; // 定义变量a并初始化（行尾注释）\n" // <--- 加上 \n
        "  int b = 1; // 定义变量b，值为1 // 注释内再写//也不影响\n" // <--- 加上 \n
        "  // 这是单独一行的注释，下面的if语句正常执行\n" // <--- 加上 \n
        "  if (a == 0 || b == 1) {" 
        "     a = 10; // 满足条件，a赋值为10\n" // <--- 加上 \n
        "  } "
        "  if (a == 10 && b == 0) { " 
        "     a = 20; "
        "  } "
        "  printf(\"Hello TinyC! Number: %d\\n\", a); // 输出a的值\n" // <--- 加上 \n
        "  return 0; // 函数返回0 // 注释结尾\n" // <--- 关键！必须加上 \n
        "}"; // 现在 } 位于新的一行，安全了
    // printf("--- 正在分析 ---\n%s\n\n", source_code);
    
    lexer_init(source_code);

    ASTNode* root = parse();
    // printf("语法分析完成！\n\n");

    // printf("--- 生成的 AST 树 ---\n");
    // print_ast(root, 0);

    // printf("--- Generating Assembly Code ---\n");
    codegen(root);

    free_ast(root);
    // printf("内存已释放。\n");

    return 0;
}