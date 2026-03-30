#ifndef BOT_H
#define BOT_H

#include "bullet.h"

// Константы ботов
#define MAX_BOTS 4
#define BOT_SPAWN_INTERVAL 4.0f
#define BOT_ROTATE_INTERVAL 1.0f
#define BOT_SIZE 58
#define BOT_SPEED 175.0f
#define BOT_SHOOT_DELAY 2.0f

// Структура бота
typedef struct {
    Bullet b_bullet;           // пуля бота
    double deathTime;          // время смерти (для спавна)
    double nextRotateTime;     // время следующего поворота
    float x, y;
    float dirX, dirY;
    int active;
    float invincibleTimer;
} Bot;

// Глобальные переменные
extern Bot bots[MAX_BOTS];
extern Spawner sp_bots[];

#endif // BOT_H
