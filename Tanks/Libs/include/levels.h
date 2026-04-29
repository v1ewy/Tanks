#ifndef LEVELS_H
#define LEVELS_H

#include <map.h>

#define TOTAL_LEVELS 3

typedef struct {
    int map[GRID_SIZE][GRID_SIZE];
    int player_lives;
    const char* name;
} Level;

extern Level levels[TOTAL_LEVELS];

void levels_init(void);
void load_level(int index);

#endif // LEVELS_H
