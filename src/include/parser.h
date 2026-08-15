#ifndef PARSER_H
#define PARSER_H
#include "ast.h"

uint64_t parse(char *, struct ast_node **);
void free_ast(struct ast_node *);

#endif
