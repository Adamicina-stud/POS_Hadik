#include "client_input.h"
#include <ncurses.h>

direction_t input_key_to_dir(int ch) {
    // WASD
    if (ch == 'w' || ch == 'W') return DIR_UP;
    if (ch == 's' || ch == 'S') return DIR_DOWN;
    if (ch == 'a' || ch == 'A') return DIR_LEFT;
    if (ch == 'd' || ch == 'D') return DIR_RIGHT;

    // šípky (ncurses)
    if (ch == KEY_UP) return DIR_UP;
    if (ch == KEY_DOWN) return DIR_DOWN;
    if (ch == KEY_LEFT) return DIR_LEFT;
    if (ch == KEY_RIGHT) return DIR_RIGHT;

    return DIR_NONE;
}

char input_dir_to_char(direction_t d) {
    switch (d) {
        case DIR_UP: return 'U';
        case DIR_DOWN: return 'D';
        case DIR_LEFT: return 'L';
        case DIR_RIGHT: return 'R';
        default: return 'U';
    }
}
