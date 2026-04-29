#ifndef BOT_H
#define BOT_H

#include "bullet.h"
#include "map.h"

#define MAX_BOTS 4
#define BOT_SPAWN_INTERVAL 4.0f
#define BOT_ROTATE_INTERVAL 1.0f
#define BOT_SIZE 56
#define BOT_SPEED 175.0f
#define BOT_SHOOT_DELAY 2.0f

typedef struct {
    Bullet b_bullet;
    double deathTime;
    double nextRotateTime;
    float  x, y;
    float  dirX, dirY;
    int    active;
    float  invincibleTimer;
} Bot;

extern Bot bots[MAX_BOTS];
extern Spawner sp_bots[];

// spawnPoints — массив точек спавна ботов, spawnCount — их количество
void bots_update(float deltaTime, double currentTime,
                 int fieldX, int fieldY, int fieldSize,
                 Spawner* spawnPoints, int spawnCount);

void bots_draw(void (*draw_rect)(float, float, float, float, float*));

#endif // BOT_H
