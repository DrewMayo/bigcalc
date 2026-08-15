#ifndef STATE_H
#define STATE_H

#include <stdint.h>
#define NUM_REGISTERS 26
#define MAX_HISTORY 1024
#define MAX_CHARS 128

struct calc_state {
    uint64_t accumulator;
    uint64_t registers[NUM_REGISTERS];
    char history[MAX_HISTORY][MAX_CHARS];
    uint16_t hist_cnt;
    uint16_t hist_head;
    uint16_t hist_pos;
    uint8_t width;
    uint8_t mode;
};

#endif
