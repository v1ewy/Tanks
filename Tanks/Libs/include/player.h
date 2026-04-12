#ifndef PLAYER_H
#define PLAYER_H

#include "bullet.h"

// Константы игрока
#define PLAYER_SIZE 58
#define PLAYER_SPEED 175.0f
#define PLAYER_SHOOT_DELAY 0.4f
#define INVINCIBLE_DURATION 2.0f

// Структура игрока
typedef struct {
    Bullet p_bullet;
    float x, y;
    float dirX, dirY;
    float invincibleTimer;
    int dead;
    int lives;
} Player;

typedef struct {
    float x, y;
} Spawner;

// Глобальные переменные
extern Player player;
extern Spawner sp_player;

#endif // PLAYER_H
