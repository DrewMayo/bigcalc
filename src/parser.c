#include "include/parser.h"
#include "include/error.h"
#include "include/lexer.h"
#include "include/token.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define UNARY_BP 100
#define SLICE_BP 200

// takes a lexer and parses it testing it for correctness
struct parser {
    struct lexer *lex;
    uint64_t pos;
};

static struct ast_node *parse_number(struct parser *par);
static struct ast_node *parse_binary(struct parser *par, struct ast_node *left);
static struct ast_node *parse_assign(struct parser *par, struct ast_node *left);
static struct ast_node *parse_slice(struct parser *par, struct ast_node *left);
static struct ast_node *parse_unary(struct parser *par);
static struct ast_node *parse_group(struct parser *par);
static struct ast_node *parse_expr(struct parser *par, uint64_t min_bp);

// function typedefs so we don't have to look at weird things
typedef struct ast_node *(*const prefix_fn)(struct parser *);
typedef struct ast_node *(*const infix_fn)(struct parser *, struct ast_node *);

// function table for prefix (nud)
static prefix_fn PREFIX_TABLE[NUM_TOKEN_TYPES] = {
    [TOK_BIN_NUM] = parse_number,
    [TOK_OCT_NUM] = parse_number,
    [TOK_DEC_NUM] = parse_number,
    [TOK_HEX_NUM] = parse_number,
    [TOK_IDENT] = parse_number,
    [TOK_NOT] = parse_unary,
    [TOK_LPAREN] = parse_group,
};

// function table for infix (led)
static infix_fn INFIX_TABLE[NUM_TOKEN_TYPES] = {
    [TOK_EQUAL] = parse_assign,
    [TOK_AND] = parse_binary,
    [TOK_OR] = parse_binary,
    [TOK_XOR] = parse_binary,
    [TOK_PLUS] = parse_binary,
    [TOK_MINUS] = parse_binary,
    [TOK_MUL] = parse_binary,
    [TOK_DIV] = parse_binary,
    [TOK_LSHIFT] = parse_binary,
    [TOK_RSHIFT] = parse_binary,
    [TOK_LESS_THAN] = parse_binary,
    [TOK_GREATER_THAN] = parse_binary,
    [TOK_SLICE] = parse_slice,
};

enum assoc {
    ASSOC_LEFT,
    ASSOC_RIGHT
};

struct op_info {
    uint64_t bp;
    enum assoc assoc;
};
// binding power (higher means tighter bindings)
static const struct op_info BP_TABLE[NUM_TOKEN_TYPES] = {
    [TOK_EQUAL] = {1, ASSOC_RIGHT}, [TOK_OR] = {2, ASSOC_LEFT}, [TOK_XOR] = {3, ASSOC_LEFT}, [TOK_AND] = {4, ASSOC_LEFT}, [TOK_LESS_THAN] = {5, ASSOC_LEFT}, [TOK_GREATER_THAN] = {5, ASSOC_LEFT}, [TOK_LSHIFT] = {6, ASSOC_LEFT}, [TOK_RSHIFT] = {6, ASSOC_LEFT}, [TOK_PLUS] = {7, ASSOC_LEFT}, [TOK_MINUS] = {7, ASSOC_LEFT}, [TOK_MUL] = {8, ASSOC_LEFT}, [TOK_DIV] = {8, ASSOC_LEFT}, [TOK_MOD] = {8, ASSOC_LEFT}, [TOK_SLICE] = {SLICE_BP, ASSOC_LEFT}};

static uint64_t next_min_bp(enum token type) {
    const struct op_info *info = &BP_TABLE[type];
    return info->assoc == ASSOC_LEFT ? info->bp + 1 : info->bp;
}

// canonical peeking from pratt parsing
struct token_data peek(struct parser *par) {
    if (par->pos >= par->lex->token_cnt) {
        return (struct token_data){0};
    }
    return par->lex->toks[par->pos];
}

// advance the pratt parser
struct token_data advance(struct parser *par) {
    struct token_data tok = peek(par);
    if (tok.type != TOK_INVLAID) {
        ++par->pos;
    }
    return tok;
}

// build the ast tree for the pratt parser
uint64_t parse(char *input, struct ast_node **tree) {
    struct lexer lex = {0};
    uint64_t retval = 0;
    CHECK_ERROR(tokenize(input, &lex), retval, 0);
    if (lex.token_cnt == 0) {
        return ENULL;
    }
    struct parser par = {&lex, 0};
    *tree = parse_expr(&par, 0);
    if (*tree == NULL) {
        return ENULL;
    }
    free_tokens(&lex);
    return 0;
error:
    return retval;
}

// nud
static struct ast_node *parse_number(struct parser *par) {
    struct token_data tok = advance(par);
    struct ast_node *node = calloc(1, sizeof(struct ast_node));
    TEST(node);
    node->tok = tok;
    node->tok.value = strdup(tok.value);
    return node;
error:
    return NULL;
}

// led
static struct ast_node *parse_unary(struct parser *par) {
    struct token_data op = advance(par);
    assert(op.type < NUM_TOKEN_TYPES && "token type out of enum range");
    struct ast_node *node = calloc(1, sizeof(struct ast_node));
    TEST(node);
    node->tok = op;
    node->right = parse_expr(par, UNARY_BP);
    TEST(node->right);
    return node;
error:
    free(node);
    return NULL;
}

static struct ast_node *parse_group(struct parser *par) {
    advance(par);
    struct ast_node *inner = parse_expr(par, 0);
    TEST(inner);
    struct token_data close = advance(par);
    TEST(close.type == TOK_RPAREN);
    return inner;
error:
    free_ast(inner);
    return NULL;
}

static struct ast_node *parse_binary(struct parser *par,
                                     struct ast_node *left) {
    struct token_data op = advance(par);
    assert(op.type < NUM_TOKEN_TYPES && "token type out of enum range");
    struct ast_node *node = calloc(1, sizeof(struct ast_node));
    TEST(node);
    node->tok = op;
    node->left = left;
    node->right = parse_expr(par, next_min_bp(op.type));
    TEST(node->right);
    return node;
error:
    free(node);
    return NULL;
}

static struct ast_node *parse_assign(struct parser *par,
                                     struct ast_node *left) {
    struct ast_node *node = NULL;
    struct token_data op = advance(par);
    assert(op.type < NUM_TOKEN_TYPES && "token type out of enum range");
    TEST(left);
    TEST(left->tok.type == TOK_IDENT || left->tok.type == TOK_SLICE);
    if (left->tok.type == TOK_SLICE) {
        TEST(left->left);
        TEST(left->left->tok.type == TOK_IDENT);
    }
    node = calloc(1, sizeof(struct ast_node));
    TEST(node);
    node->tok = op;
    node->left = left;
    node->right = parse_expr(par, next_min_bp(op.type));
    TEST(node->right);
    return node;
error:
    free(node);
    return NULL;
}

static struct ast_node *parse_slice(struct parser *par, struct ast_node *left) {
    struct ast_node *node = NULL;
    struct token_data op = advance(par);
    assert(op.type < NUM_TOKEN_TYPES && "token type out of enum range");
    TEST(left);
    node = calloc(1, sizeof(struct ast_node));
    TEST(node);
    node->tok = op;
    node->tok.value = strdup(op.value);
    node->left = left;
    return node;
error:
    free(node);
    return NULL;
}

static struct ast_node *parse_expr(struct parser *par, uint64_t min_bp) {
    struct token_data tok = peek(par);
    prefix_fn left_fn = PREFIX_TABLE[tok.type];
    assert(tok.type < NUM_TOKEN_TYPES && "token type out of enum range");
    TEST(left_fn);
    struct ast_node *left = left_fn(par);
    TEST(left);
    while (1) {
        struct token_data op = peek(par);
        assert(op.type < NUM_TOKEN_TYPES && "token type out of enum range");
        uint64_t bp = BP_TABLE[op.type].bp;
        if (bp == 0 || bp < min_bp) {
            break;
        }

        if (!INFIX_TABLE[op.type]) {
            break;
        }

        left = INFIX_TABLE[op.type](par, left);
        TEST(left);
    }
    return left;
error:
    return NULL;
}

void free_ast(struct ast_node *node) {
    if (!node) {
        return;
    }
    free_ast(node->left);
    free_ast(node->right);
    free(node->tok.value);
    free(node);
}
