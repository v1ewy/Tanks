#ifndef PLAYER_H
#define PLAYER_H

#include "bullet.h"
#include "map.h"

#define PLAYER_SIZE 56
#define PLAYER_SPEED 175.0f
#define PLAYER_SHOOT_DELAY 0.4f
#define INVINCIBLE_DURATION 1.0f

typedef struct {
    Bullet p_bullet;
    float  x, y;
    int    dead;
    int    lives;
    float  invincibleTimer;
} Player;

extern Player  player;
extern Spawner sp_player;

void player_update(void* window, float deltaTime, double currentTime,
                   int fieldX, int fieldY, int fieldSize);

#endif 
