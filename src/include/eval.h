#ifndef EVAL_H
#define EVAL_H

#include "ast.h"
#include "state.h"

uint64_t eval(struct ast_node *node, struct calc_state *state);

#endif
