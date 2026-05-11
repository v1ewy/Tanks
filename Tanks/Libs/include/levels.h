#ifndef LEVELS_H
#define LEVELS_H

#include <map.h>
#include <bots.h>

#define TOTAL_LEVELS 3
#define MAX_LEVEL_BOTS 20 

typedef struct {
    BotType type;
    int     spawnPointIndex; // индекс точки спавна (по порядку из карты)
} BotWave;

typedef struct {
    int      map[GRID_SIZE][GRID_SIZE];
    int      player_lives;
    const char* name;
    BotWave  botQueue[MAX_LEVEL_BOTS]; 
    int      botCount;
    int botCurrent;
} Level;

extern Level levels[TOTAL_LEVELS];
extern int botQueueIndex;
extern int currentLevelIndex;

void levels_init(void);
void load_level(int index);


int level_is_complete(void);

#endif 
