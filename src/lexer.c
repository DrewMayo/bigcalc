#include "include/lexer.h"
#include "include/error.h"
#include "include/token.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define VALID_BIN "01"
#define VALID_OCT "01234567"
#define VALID_DEC "0123456789"
#define VALID_HEX "0123456789abcdefABCDEF"
#define VALID_IDENT "abcdefghijklmnopqrstuvwxyz"
#define VALID_CHAR " &|^~+-*/%=()<>"

uint64_t token_push(struct lexer *lex, struct token_data tok);
uint64_t extract_num(char **, struct token_data *);
uint64_t extract_ident(char **, struct token_data *);
uint64_t extract_fixed_char(char **, struct token_data *);
uint64_t extract_slice(char **input, struct token_data *slice_tok);
void free_tokens(struct lexer *lex);

// this is how we take a string of characters and
// turn it into a set of tokens to then parse at a later date
uint64_t tokenize(char *input, struct lexer *lex) {
    char *c = input;
    uint64_t retval = 0;
    while (*c != '\0') {
        struct token_data tok = {0};
        while (*c == ' ' || *c == '\n' || *c == '\t') {
            c++;
        }
        if (*c == '\0') {
            break;
        }
        // number
        if (strchr(VALID_DEC, *c)) {
            CHECK_ERROR(extract_num(&c, &tok), retval, 0);
            CHECK_ERROR(token_push(lex, tok), retval, 0);
            continue;
        }
        // identifier
        if (strchr(VALID_IDENT, *c)) {
            CHECK_ERROR(extract_ident(&c, &tok), retval, 0);
            CHECK_ERROR(token_push(lex, tok), retval, 0);
            continue;
        }
        if (*c == '[') {
            CHECK_ERROR(extract_slice(&c, &tok), retval, 0);
            CHECK_ERROR(token_push(lex, tok), retval, 0);
            continue;
        }
        // fixed character
        if (strchr(VALID_CHAR, *c)) {
            CHECK_ERROR(extract_fixed_char(&c, &tok), retval, 0);
            CHECK_ERROR(token_push(lex, tok), retval, 0);
            continue;
        }
        retval = EINVALID;
        goto error;
    }
    return 0;
error:
    free_tokens(lex);
    return retval;
}

void free_tokens(struct lexer *lexer) {
    for (size_t i = 0; i < lexer->token_cnt; ++i) {
        lexer->toks[i].type = TOK_INVLAID;
        if (lexer->toks[i].value != NULL) {
            free(lexer->toks[i].value);
        }
    }
    lexer->token_cnt = 0;
}

uint64_t token_push(struct lexer *lexer, struct token_data tok) {
    if (!lexer) {
        return ENULL;
    }
    if (lexer->token_cnt < MAX_TOKENS) {
        lexer->toks[lexer->token_cnt++] = tok;
    } else {
        return EFULL;
    }
    return 0;
}

uint64_t extract_fixed_char(char **input, struct token_data *tok) {
    if (!input || !(*input)) {
        return ENULL;
    }
    if (*(*input) == '\0') {
        return EINVALID;
    }
    switch (**input) {
    case ('&'): {
        tok->type = TOK_AND;
        (*input)++;
        break;
    }
    case ('|'): {
        tok->type = TOK_OR;
        (*input)++;
        break;
    }
    case ('^'): {
        tok->type = TOK_XOR;
        (*input)++;
        break;
    }
    case ('~'): {
        tok->type = TOK_NOT;
        (*input)++;
        break;
    }
    case ('+'): {
        tok->type = TOK_PLUS;
        (*input)++;
        break;
    }
    case ('-'): {
        tok->type = TOK_MINUS;
        (*input)++;
        break;
    }
    case ('*'): {
        tok->type = TOK_MUL;
        (*input)++;
        break;
    }
    case ('/'): {
        tok->type = TOK_DIV;
        (*input)++;
        break;
    }
    case ('%'): {
        tok->type = TOK_MOD;
        (*input)++;
        break;
    }
    case ('='): {
        tok->type = TOK_EQUAL;
        (*input)++;
        break;
    }
    case ('('): {
        tok->type = TOK_LPAREN;
        (*input)++;
        break;
    }
    case (')'): {
        tok->type = TOK_RPAREN;
        (*input)++;
        break;
    }
    case ('{'): {
        tok->type = TOK_LCBRACE;
        (*input)++;
        break;
    }
    case ('}'): {
        tok->type = TOK_RCBRACE;
        (*input)++;
        break;
    }
    case (','): {
        tok->type = TOK_COMMA;
        (*input)++;
        break;
    }
    case ('<'): {
        (*input)++;
        if (**input == '<') {
            tok->type = TOK_LSHIFT;
            (*input)++;
        } else {
            tok->type = TOK_LESS_THAN;
        }
        break;
    }
    case ('>'): {
        (*input)++;
        if (**input == '>') {
            tok->type = TOK_RSHIFT;
            (*input)++;
        } else {
            tok->type = TOK_GREATER_THAN;
        }
        break;
    }
    default: {
        return EINVALID;
    }
    }
    return 0;
}

uint64_t extract_slice(char **input, struct token_data *slice_tok) {
    if (!input || !(*input)) {
        return ENULL;
    }
    if (*(*input) == '\0') {
        return EINVALID;
    }
    char *c = *input;
    char *start = *input;
    char *end = start;
    if (*c != '[') {
        return EINVALID;
    }
    c++;
    start = c;
    while (*c != ']') {
        if (*c == '\0') {
            return EINVALID;
        }
        c++;
    }
    end = c;
    c++;
    uint64_t bufsz = end - start;
    assert(end >= start);
    assert(bufsz != 0);
    assert(bufsz > 0);
    slice_tok->type = TOK_SLICE;
    slice_tok->value = (char *)malloc(bufsz + 1);
    memcpy(slice_tok->value, start, bufsz);
    slice_tok->value[bufsz] = '\0';
    *input = c;
    return 0;
}

// finds the number from the input. char **num is malloced.
// input is advanced
uint64_t extract_num(char **input, struct token_data *num_tok) {
    if (!input || !(*input)) {
        return ENULL;
    }
    if (*(*input) == '\0') {
        return EINVALID;
    }
    char *c = *input;
    char *start = *input;
    char *end = start;
    char *char_set = VALID_DEC;
    enum token type = TOK_DEC_NUM;
    // find the charset we need for the type of number we are lexing
    if (*c == '0') {
        if (*(c + 1) == 'b') {
            c += 2;
            char_set = VALID_BIN;
            type = TOK_BIN_NUM;
        } else if (*(c + 1) == 'x' || *(c + 1) == 'X') {
            c += 2;
            char_set = VALID_HEX;
            type = TOK_HEX_NUM;
        } else {
            char_set = VALID_OCT;
            type = TOK_OCT_NUM;
        }
    }
    while (1) {
        if (*c == '\0' || !strchr(char_set, *c)) {
            if (strchr(VALID_CHAR, *c)) {
                end = c;
                break;
            } else {
                return EINVALID;
            }
        }
        c++;
    }
    uint64_t bufsz = end - start;
    assert(end >= start);
    assert(bufsz != 0);
    assert(bufsz > 0);
    // handle the 0x or 0b case
    if (bufsz == 2 && (type == TOK_HEX_NUM || type == TOK_BIN_NUM)) {
        return EINVALID;
    }
    num_tok->type = type;
    num_tok->value = (char *)malloc(bufsz + 1);
    memcpy(num_tok->value, start, bufsz);
    num_tok->value[bufsz] = '\0';
    *input = c;
    return 0;
}

uint64_t extract_ident(char **input, struct token_data *ident_tok) {
    if (!input || !(*input)) {
        return ENULL;
    }
    if (*(*input) == '\0') {
        return EINVALID;
    }
    char *plus_one = *input + 1;
    if (*plus_one != '\0' && strchr(VALID_IDENT, *plus_one)) {
        return EINVALID;
    }
    ident_tok->type = TOK_IDENT;
    ident_tok->value = calloc(2, sizeof(char));
    ident_tok->value[0] = **input;
    ident_tok->value[1] = '\0';
    (*input)++;
    return 0;
}

const char *token_name(enum token type) {
    static const char *const names[] = {
        [TOK_INVLAID] = "(invalid)",
        [TOK_DEC_NUM] = "DEC_NUM",
        [TOK_HEX_NUM] = "HEX_NUM",
        [TOK_OCT_NUM] = "OCT_NUM",
        [TOK_BIN_NUM] = "BIN_NUM",
        [TOK_AND] = "AND",
        [TOK_OR] = "OR",
        [TOK_XOR] = "XOR",
        [TOK_NOT] = "NOT",
        [TOK_PLUS] = "PLUS",
        [TOK_MINUS] = "MINUS",
        [TOK_MUL] = "MUL",
        [TOK_DIV] = "DIV",
        [TOK_MOD] = "MOD",
        [TOK_EQUAL] = "EQUAL",
        [TOK_LPAREN] = "LPAREN",
        [TOK_RPAREN] = "RPAREN",
        [TOK_LCBRACE] = "LCBRACE",
        [TOK_RCBRACE] = "RCBRACE",
        [TOK_COMMA] = "COMMA",
        [TOK_IDENT] = "IDENT",
    };
    return names[type];
}
