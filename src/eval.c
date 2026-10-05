#include "include/eval.h"
#include "include/error.h"
#include "include/token.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint64_t op_add(uint64_t l, uint64_t r) { return l + r; }
static uint64_t op_sub(uint64_t l, uint64_t r) { return l - r; }
static uint64_t op_mul(uint64_t l, uint64_t r) { return l * r; }
static uint64_t op_div(uint64_t l, uint64_t r) { return l / r; }
static uint64_t op_mod(uint64_t l, uint64_t r) { return l % r; }
static uint64_t op_and(uint64_t l, uint64_t r) { return l & r; }
static uint64_t op_or(uint64_t l, uint64_t r) { return l | r; }
static uint64_t op_xor(uint64_t l, uint64_t r) { return l ^ r; }
static uint64_t op_less_than(uint64_t l, uint64_t r) { return l < r; }
static uint64_t op_greater_than(uint64_t l, uint64_t r) { return l > r; }
static uint64_t op_lshift(uint64_t l, uint64_t r) { return l << r; }
static uint64_t op_rshift(uint64_t l, uint64_t r) { return l >> r; }

static uint64_t op_not(uint64_t l, uint64_t r) {
    (void)l;
    return ~r;
}

static uint64_t (*const OP_TABLE[NUM_TOKEN_TYPES])(uint64_t, uint64_t) = {
    [TOK_PLUS] = op_add,
    [TOK_MINUS] = op_sub,
    [TOK_MUL] = op_mul,
    [TOK_DIV] = op_div,
    [TOK_MOD] = op_mod,
    [TOK_AND] = op_and,
    [TOK_OR] = op_or,
    [TOK_XOR] = op_xor,
    [TOK_NOT] = op_not,
    [TOK_LESS_THAN] = op_less_than,
    [TOK_GREATER_THAN] = op_greater_than,
    [TOK_LSHIFT] = op_lshift,
    [TOK_RSHIFT] = op_rshift,
};

static uint64_t registers[NUM_REGISTERS] = {0};
static uint64_t error = 0;

uint64_t eval_recurse(struct ast_node *node);

uint64_t eval(struct ast_node *const node, struct calc_state *state) {
    for (int i = 0; i < NUM_REGISTERS; ++i) {
        assert(registers[i] == state->registers[i] && "state is wrong");
    }
    error = 0;
    uint64_t val = eval_recurse(node);
    if (error != 0) {
        return error;
    }
    state->accumulator = val;
    memcpy(state->registers, registers, sizeof(uint64_t) * NUM_REGISTERS);
    return 0;
}

uint64_t eval_recurse(struct ast_node *node) {
    TEST(node);
    if (node->tok.type == TOK_EQUAL) {
        TEST(node->left);
        TEST(node->right);
        uint64_t val = eval_recurse(node->right);
        if (node->left->tok.type == TOK_SLICE) {
            struct ast_node *slice = node->left;
            TEST(slice->left);
            TEST(slice->left->tok.type == TOK_IDENT);
            uint64_t hi = 0;
            uint64_t lo = 0;
            TEST(sscanf(slice->tok.value, "%lu:%lu", &hi, &lo) == 2);
            TEST(hi >= lo);
            uint64_t width = hi - lo + 1;
            uint64_t mask = (width >= 64) ? ~(uint64_t)0 : (((uint64_t)1 << width) - 1);
            size_t reg = *slice->left->tok.value - 'a';
            uint64_t sliced_val = ~(mask << lo) | ((val & mask) << lo);
            registers[reg] = (registers[reg] & ~(mask << lo)) | ((val & mask) << lo);
            return sliced_val;
        }
        TEST(node->left->tok.type == TOK_IDENT);
        registers[*node->left->tok.value - 'a'] = val;
        return val;
    }
    if (node->tok.type == TOK_SLICE) {
        TEST(node->left);
        uint64_t val = eval_recurse(node->left);
        uint64_t hi = 0;
        uint64_t lo = 0;
        TEST(sscanf(node->tok.value, "%lu:%lu", &hi, &lo) == 2);
        TEST(hi >= lo);
        uint64_t width = hi - lo + 1;
        uint64_t mask = (width >= 64) ? ~(uint64_t)0 : (((uint64_t)1 << width) - 1);
        return (val >> lo) & mask;
    }
    if (!node->left && !node->right) {
        if (node->tok.type == TOK_IDENT) {
            assert(*node->tok.value >= 'a' && 'z' >= *node->tok.value);
            return registers[*node->tok.value - 'a'];
        }
        return strtoll(node->tok.value, NULL, 0);
    }
    uint64_t left = 0;
    uint64_t right = 0;
    if (node->left) {
        left = eval_recurse(node->left);
    }
    if (node->right) {
        right = eval_recurse(node->right);
    }
    assert(OP_TABLE[node->tok.type] && "type not in table");
    return OP_TABLE[node->tok.type](left, right);
error:
    error = ENULL;
    return ENULL;
}
