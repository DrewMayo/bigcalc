#ifndef AST_H
#define AST_H
#include "token.h"

struct ast_node {
    struct token_data tok;
    struct ast_node *left;
    struct ast_node *right;
};
#endif
