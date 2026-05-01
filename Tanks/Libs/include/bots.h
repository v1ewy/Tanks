#ifndef BOT_H
#define BOT_H

#include "bullet.h"
#include "player.h"

#define MAX_BOTS           8
#define BOT_SPAWN_INTERVAL 4.0f
#define BOT_ROTATE_INTERVAL 0.5f
#define BOT_SIZE           58
#define BOT_SHOOT_DELAY    2.0f

// Скорости по типу
#define BOT_SPEED_NORMAL   175.0f
#define BOT_SPEED_HOUND    280.0f
#define BOT_SPEED_HUNTER   175.0f
#define BOT_SPEED_ARMORED  150.0f

// Типы ботов
typedef enum {
    BOT_NORMAL  = 0,  // средняя скорость, цель: игрок или база
    BOT_HOUND   = 1,  // быстрый, цель: база
    BOT_HUNTER  = 2,  // средняя скорость, цель: игрок
    BOT_ARMORED = 3   // медленный, 3 хита
} BotType;

// Структура бота
typedef struct {
    Bullet   b_bullet;
    double   deathTime;
    double   nextRotateTime;
    float    x, y;
    float    dirX, dirY;
    float    speed;
    int      active;
    float    invincibleTimer;
    BotType  type;
    int      hp;          // хиты (у бронированного = 3)
    int      flashTimer;  // мигание при попадании
} Bot;

// База
typedef struct {
    float x, y;
    float width, height;
    int   hp;
    int   alive;
} Base;

extern Bot  bots[MAX_BOTS];
extern Base gBase;
extern Spawner sp_bots[];

void bots_update(float deltaTime, double currentTime,
                 int fieldX, int fieldY, int fieldSize,
                 Spawner* spawnPoints, int spawnCount);

void base_init(int fieldX, int fieldY);

// Возвращает 1 если боты победили (база уничтожена)
int  base_is_destroyed(void);

#endif // BOT_H
