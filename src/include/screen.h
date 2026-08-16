#ifndef SCREEN_H
#define SCREEN_H

#include "state.h"
#include <stdint.h>

void init_tui(void);
void render_tui(const struct calc_state *const state, const char *const input);
void render_input(const char *const input);
void render_history(const struct calc_state *const state);
void end_tui(void);
int get_input_char(void);
int get_input_win_x(void);
uint64_t push_hist(struct calc_state *const state, const char *const input);
uint64_t down_hist(struct calc_state *const state, char *input, int *pos);
uint64_t up_hist(struct calc_state *const state, char *input, int *pos);

#endif
