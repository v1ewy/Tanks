#ifndef SCORE_H
#define SCORE_H

#include <bots.h>

// Очки за убийство по типу бота
#define SCORE_NORMAL  100
#define SCORE_HUNTER  300
#define SCORE_HOUND   600
#define SCORE_ARMORED 300

// Бонус за скорость по уровню (если уложился в лимит)
#define SCORE_TIME_BONUS_L1 500  // < 30 сек
#define SCORE_TIME_BONUS_L2 400  // < 60 сек
#define SCORE_TIME_BONUS_L3 300  // < 90 сек

#define SCORE_TIME_LIMIT_L1 30.0
#define SCORE_TIME_LIMIT_L2 60.0
#define SCORE_TIME_LIMIT_L3 90.0

extern int   gScore;
extern double gLevelStartTime;
extern int gKills[4];

void  score_reset(void);
void  score_add_kill(BotType type);
int   score_time_bonus(int levelIndex, double elapsed);

#endif // SCORE_H//

