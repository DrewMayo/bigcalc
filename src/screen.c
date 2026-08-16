#include "include/screen.h"
#include "include/error.h"
#include "include/state.h"
#include <assert.h>
#include <ncurses.h>
#include <stdint.h>
#include <string.h>

#define INPUT_HEIGHT 3
#define STATUS_HEIGHT 1
#define DISPLAY_HEIGHT 7
#define HEX_BUFSZ 20
#define BIN_BUFSZ 48
#define INPUT_TEXT "Input: "

WINDOW *display_win;
WINDOW *register_win;
WINDOW *input_win;
WINDOW *history_win;
WINDOW *status_win;

int max_y;
int max_x;

char display_hex_buf[HEX_BUFSZ];
char display_bin_buf[2][BIN_BUFSZ];

void render_display(const struct calc_state *const state);
void render_history(const struct calc_state *const state);
void render_input(const char *const input);
void render_registers(const struct calc_state *const state);
void render_status(const struct calc_state *const state);

void init_tui(void) {
    initscr();
    cbreak();
    noecho();
    getmaxyx(stdscr, max_y, max_x);
    input_win = newwin(INPUT_HEIGHT, 0.70 * max_x,
                       max_y - INPUT_HEIGHT - STATUS_HEIGHT, 0);
    status_win = newwin(STATUS_HEIGHT, max_x, max_y - STATUS_HEIGHT, 0);
    register_win = newwin(max_y - STATUS_HEIGHT, 0.30 * max_x, 0, max_x * 0.70);
    display_win = newwin(DISPLAY_HEIGHT, 0.70 * max_x, 0, 0);
    history_win = newwin(max_y - DISPLAY_HEIGHT - INPUT_HEIGHT - STATUS_HEIGHT,
                         0.70 * max_x, DISPLAY_HEIGHT, 0);
    refresh();
    leaveok(input_win, FALSE);
    leaveok(status_win, TRUE);
    leaveok(register_win, TRUE);
    leaveok(display_win, TRUE);
    leaveok(history_win, TRUE);
    keypad(input_win, TRUE);
}

uint64_t format_hex(uint64_t hex_val, char *buf, size_t bufsz) {
    static const char values[] = "0123456789ABCDEF";
    size_t pos = 0;
    size_t shift_cnt = 0;
    memset(buf, ' ', bufsz);
    do {
        uint8_t num = (hex_val & 0xF000000000000000) >> 60;
        assert(num < 16);
        if (pos < bufsz) {
            buf[pos++] = values[num];
            shift_cnt++;
            if (shift_cnt != 0 && shift_cnt % 4 == 0) {
                buf[pos++] = ' ';
            }
        } else {
            return EFULL;
        }
        hex_val <<= 4;
    } while (hex_val > 0);
    buf[bufsz - 1] = '\0';
    return 0;
}

uint64_t format_bin(uint32_t bin_val, char *buf, size_t bufsz) {
    static const char values[] = "01";
    size_t pos = 0;
    size_t shift_cnt = 0;
    memset(buf, ' ', bufsz);
    while (shift_cnt < 32) {
        uint8_t num = (bin_val & 0x80000000) >> 31;
        buf[pos++] = values[num];
        shift_cnt++;
        if (shift_cnt != 0 && shift_cnt % 4 == 0) {
            buf[pos++] = ' ';
            if (shift_cnt % 16 == 0) {
                buf[pos++] = ' ';
                buf[pos++] = ' ';
                buf[pos++] = ' ';
            }
        }
        bin_val <<= 1;
    }
    buf[bufsz - 1] = '\0';
    return 0;
}

void render_tui(const struct calc_state *const state, const char *const input) {
    render_display(state);
    render_history(state);
    render_registers(state);
    render_status(state);
    render_input(input);
}

void render_display(const struct calc_state *const state) {
    werase(display_win);
    format_hex(state->accumulator, display_hex_buf, HEX_BUFSZ);
    format_bin((state->accumulator >> 32) & 0xFFFFFFFF, display_bin_buf[0],
               BIN_BUFSZ);
    format_bin(state->accumulator & 0xFFFFFFFF, display_bin_buf[1], BIN_BUFSZ);
    mvwprintw(display_win, 1, 1, "    Dec: %lu", state->accumulator);
    mvwprintw(display_win, 2, 1, "    Hex: %s    Oct: %lo", display_hex_buf,
              state->accumulator);
    mvwprintw(display_win, 4, 1, "[63:32]: %s", display_bin_buf[0]);
    mvwprintw(display_win, 5, 1, "[31:00]: %s", display_bin_buf[1]);
    box(display_win, 0, 0);
    wrefresh(display_win);
}

void render_history(const struct calc_state *const state) {
    const uint64_t hist_height =
        max_y - DISPLAY_HEIGHT - INPUT_HEIGHT - STATUS_HEIGHT - 2;
    const uint64_t num_render =
        hist_height > state->hist_cnt ? state->hist_cnt : hist_height;
    werase(history_win);
    uint64_t idx = 0;
    for (uint64_t i = 0; num_render != 0 && i < num_render; i++) {
        // do not render the past the end of the buffer
        if (state->hist_pos + i >= MAX_HISTORY) {
            break;
        }
        idx = (state->hist_cnt - state->hist_pos - 1 - i) % MAX_HISTORY;
        mvwprintw(history_win, hist_height - i, 1, "%s",
                  state->history[idx]);
    }
    box(history_win, 0, 0);
    wrefresh(history_win);
}

uint64_t push_hist(struct calc_state *const state, const char *const input) {
    if (!state || !input) {
        return ENULL;
    }
    const size_t size = strlen(input);
    memcpy(&state->history[state->hist_cnt++ % MAX_HISTORY], input, size);
    if (state->hist_cnt >= MAX_HISTORY) {
        state->hist_head = (state->hist_head + 1) % MAX_HISTORY;
    }
    state->hist_pos = 0;
    return 0;
}

uint64_t down_hist(struct calc_state *const state, char *input, int *pos) {
    if (!state || !input) {
        return ENULL;
    }
    if (state->hist_pos != 0) {
        --state->hist_pos;
    }
    const uint64_t idx = (state->hist_cnt - state->hist_pos) % MAX_HISTORY;
    memcpy(input, state->history[idx], MAX_CHARS);
    *pos = strlen(input);
    return 0;
}

uint64_t up_hist(struct calc_state *const state, char *input, int *pos) {
    if (!state || !input) {
        return ENULL;
    }
    if (state->hist_pos < MAX_HISTORY && state->hist_pos < state->hist_cnt) {
        state->hist_pos++;
    }
    const uint64_t idx = (state->hist_cnt - state->hist_pos) % MAX_HISTORY;
    memcpy(input, state->history[idx], MAX_CHARS);
    *pos = strlen(input);
    return 0;
}

void render_registers(const struct calc_state *const state) {
    werase(register_win);
    for (int i = 0; i < NUM_REGISTERS; i++) {
        const char reg_char = 'a' + i;
        mvwprintw(register_win, 1 + i, 1, "%c: %lu", reg_char,
                  state->registers[i]);
    }
    box(register_win, 0, 0);
    wrefresh(register_win);
}

void render_input(const char *const input) {
    werase(input_win);
    mvwprintw(input_win, 1, 1, "%s%s", INPUT_TEXT, input);
    box(input_win, 0, 0);
    wrefresh(input_win);
}

void render_status(const struct calc_state *const state) {
    werase(status_win);
    wprintw(status_win, "Width: %d Bit\t Mode: 2s complement", state->width);
    wrefresh(status_win);
}

int get_input_win_x(void) { return max_x * 0.70 - strlen(INPUT_TEXT) - 2; }

void end_tui(void) { endwin(); }
int get_input_char(void) { return wgetch(input_win); }
