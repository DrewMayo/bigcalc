#ifndef TOKEN_H
#define TOKEN_H

// type of tokens
// that are possible
// numbers that have explicit numbers have been chosen for easy comparisons
// and usecases
#include <stdint.h>

#define MAX_TOKENS 128

enum token {
    TOK_INVLAID = 0,
    TOK_BIN_NUM = 1,
    TOK_OCT_NUM = 2,
    TOK_DEC_NUM = 3,
    TOK_HEX_NUM = 4,
    TOK_IDENT = 5,
    TOK_AND,
    TOK_OR,
    TOK_XOR,
    TOK_NOT,
    TOK_PLUS,
    TOK_MINUS,
    TOK_MUL,
    TOK_DIV,
    TOK_MOD,
    TOK_EQUAL,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_LCBRACE,
    TOK_RCBRACE,
    TOK_COMMA,
    TOK_LESS_THAN,
    TOK_GREATER_THAN,
    TOK_LSHIFT,
    TOK_RSHIFT,
    TOK_SLICE,
    // MUST BE THE LAST TOKEN
    NUM_TOKEN_TYPES
};

struct token_data {
    enum token type;
    char *value;
};

struct lexer {
    struct token_data toks[MAX_TOKENS];
    uint64_t token_cnt;
};
#endif
