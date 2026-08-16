#ifndef LEXER_H
#define LEXER_H

#include "token.h"

uint64_t tokenize(char *input, struct lexer *lexer);
void free_tokens(struct lexer *lexer);
const char *token_name(enum token type);

#endif
