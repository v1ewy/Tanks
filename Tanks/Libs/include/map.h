#ifndef MAP_H
#define MAP_H

#define GRID_SIZE 13
#define CELL_SIZE 64
#define BLOCK_SIZE 64
#define FIELD_SIZE 832

typedef struct {
    int   width, height;
    float x, y;
} Wood;

typedef struct { float x, y; } Spawner;

extern int  map[GRID_SIZE][GRID_SIZE];
extern Wood woods[GRID_SIZE][GRID_SIZE];
extern int  fieldX, fieldY;

#endif // MAP_H
