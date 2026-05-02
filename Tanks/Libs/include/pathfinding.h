#ifndef PATHFINDING_H
#define PATHFINDING_H

#include <map.h>

#define PF_MAX_NODES (GRID_SIZE * GRID_SIZE)

typedef struct {
    int x, y;
} PFPoint;

PFPoint pathfind_next_step(int fromX, int fromY,
                            int toX,   int toY,
                            int canBreakWood);

#endif // PATHFINDING_H
