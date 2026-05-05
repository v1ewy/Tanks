#ifndef BOT_H
#define BOT_H

#include "bullet.h"
#include "player.h"

#define MAX_BOTS           8
#define BOT_SPAWN_INTERVAL 3.0f
#define BOT_ROTATE_INTERVAL 0.5f
#define BOT_SIZE           58
#define BOT_SHOOT_DELAY    1.0f

// Скорости по типу
#define BOT_SPEED_NORMAL   150.0f
#define BOT_SPEED_HOUND    200.0f
#define BOT_SPEED_HUNTER   150.0f
#define BOT_SPEED_ARMORED  120.0f

// Типы ботов
typedef enum {
    BOT_NORMAL  = 0,
    BOT_HOUND   = 1,
    BOT_HUNTER  = 2,
    BOT_ARMORED = 3
} BotType;

// ── union для специфичных данных по типу бота ────────────────────────
// Курсовое требование: union в пользовательских типах данных.
// Обоснование: каждый тип бота хранит разные данные поведения,
// но в памяти они занимают одно и то же место — экономия памяти
// при большом количестве одновременных ботов.
typedef union {
    struct {
        int hitCount;        // сколько раз попали (BOT_ARMORED)
        int armorIntegrity;  // 3=целый, 2=повреждён, 1=критично
    } armored;
    struct {
        int rushingToBase;   // 1 = в режиме прорыва к базе (BOT_HOUND)
        int ignorePlayer;    // 1 = игнорирует игрока
    } hound;
    struct {
        float lastSeenX;     // последняя известная позиция игрока (BOT_HUNTER/NORMAL)
        float lastSeenY;
    } hunter;
} BotTypeData;

// Структура бота
typedef struct {
    Bullet      b_bullet;
    BotTypeData typeData;   // union с данными по типу бота
    double   deathTime;
    float    x, y;
    float    dirX, dirY;
    float    faceX, faceY;
    float    speed;
    int      active;
    float    invincibleTimer;
    double   stuckTimer;
    double   collisionStuckTimer; // таймер застревания при блокировке ботом/игроком
    BotType  type;
    int      hp;
    int      flashTimer;
    int      targetCellX;
    int      targetCellY;
    int      animFrame;
    double   lastAnimTime;
    int      detourAttempt; // счётчик неудачных попыток — триггер объезда
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
int  base_is_destroyed(void);

#endif // BOT_H
