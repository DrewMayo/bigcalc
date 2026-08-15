#ifndef LEXER_H
#define LEXER_H

#include "token.h"

uint64_t tokenize(char *input, struct lexer *lexer);
uint64_t extract_num(char **, struct token_data *);
void free_tokens(struct lexer *lexer);
const char *token_name(enum token type);

#endif
