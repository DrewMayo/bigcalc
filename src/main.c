#include "include/eval.h"
#include "include/parser.h"
#include "include/screen.h"
#include <ncurses.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    init_tui();
    char input[MAX_CHARS] = {0};
    struct calc_state state = {0};
    state.width = 64;
    render_tui(&state, input);
    int ch;
    int pos = 0;
    // main loop for handling and dispatching inputs
    while ((ch = get_input_char()) != KEY_F(1)) {
        if (ch == '\n') {
            struct ast_node *tree = NULL;
            if (parse(input, &tree) == 0 && eval(tree, &state) == 0) {
                free_ast(tree);
                push_hist(&state, input);
            }
            memset(input, 0, MAX_CHARS);
            render_tui(&state, input);
            pos = 0;
        } else if (ch == KEY_BACKSPACE || ch == 127) {
            if (pos > 0) {
                input[--pos] = '\0';
                render_input(input);
            }
        } else if (pos < MAX_CHARS - 1 && pos < get_input_win_x() - 1 &&
                   ch >= 32 && ch < 127) {
            input[pos++] = (char)ch;
            input[pos] = '\0';
            render_input(input);
        } else if (ch == KEY_RESIZE) {
            end_tui();
            init_tui();
            render_tui(&state, input);
        } else if (ch == KEY_UP) {
        } else if (ch == KEY_DOWN) {
        }
    }
    end_tui();
    exit(EXIT_SUCCESS);
}
