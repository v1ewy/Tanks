#ifndef LEVELS_H
#define LEVELS_H

#include <map.h>
#include <bots.h>

#define TOTAL_LEVELS 3
#define MAX_LEVEL_BOTS 20  // максимум ботов на уровень

typedef struct {
    BotType type;
    int     spawnPointIndex; // индекс точки спавна (0,1,... по порядку из карты)
} BotWave;

typedef struct {
    int      map[GRID_SIZE][GRID_SIZE];
    int      player_lives;
    const char* name;
    BotWave  botQueue[MAX_LEVEL_BOTS]; // очередь ботов
    int      botCount;
    int botCurrent;
} Level;

extern Level levels[TOTAL_LEVELS];
extern int botQueueIndex;
extern int currentLevelIndex;

void levels_init(void);
void load_level(int index);

// Возвращает 1 если все боты уровня уже заспавнены и мертвы
int level_is_complete(void);

#endif // LEVELS_H
