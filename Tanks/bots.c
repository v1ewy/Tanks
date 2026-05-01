#include <math.h>
#include <stdlib.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <bots.h>
#include <player.h>
#include <collision.h>
#include <map.h>

Bot  bots[MAX_BOTS];
Base gBase;
Spawner sp_bots[GRID_SIZE * GRID_SIZE];

// ── База ──────────────────────────────────────────────────────────────
void base_init(int fieldX, int fieldY) {
    gBase.width  = 64.0f;
    gBase.height = 64.0f;
    // Нижний центр карты — предпоследняя строка по центру
    gBase.x      = fieldX + (GRID_SIZE / 2) * CELL_SIZE + CELL_SIZE / 2.0f;
    gBase.y      = fieldY + (GRID_SIZE - 1) * CELL_SIZE + CELL_SIZE / 2.0f;
    gBase.hp     = 3;
    gBase.alive  = 1;
}

int base_is_destroyed(void) {
    return !gBase.alive;
}

// ── Вспомогательное: скорость по типу ────────────────────────────────
static float bot_speed(BotType type)
{
    switch (type) {
    case BOT_HOUND:   return BOT_SPEED_HOUND;
    case BOT_HUNTER:  return BOT_SPEED_HUNTER;
    case BOT_ARMORED: return BOT_SPEED_ARMORED;
    default:          return BOT_SPEED_NORMAL;
    }
}

// ── Вспомогательное: начальный HP по типу ────────────────────────────
static int bot_hp(BotType type)
{
    return (type == BOT_ARMORED) ? 3 : 1;
}

// ── Вспомогательное: цель бота (x, y) ────────────────────────────────
static void bot_target(Bot* b, float* tx, float* ty)
{
    switch (b->type) {
    case BOT_HOUND:
        // Всегда идёт к базе
        *tx = gBase.x;
        *ty = gBase.y;
        break;
    case BOT_HUNTER:
        // Всегда преследует игрока
        *tx = player.x;
        *ty = player.y;
        break;
    case BOT_NORMAL:
    default:
        // 50/50: игрок или база
        if (rand() % 2 == 0) {
            *tx = player.x;
            *ty = player.y;
        } else {
            *tx = gBase.x;
            *ty = gBase.y;
        }
        break;
    }
}

// ── Спавн одного бота ─────────────────────────────────────────────────
static void spawn_bot(int slot, Spawner* spawnPoints,
                      int spawnCount, int* spawnIndex,
                      double currentTime)
{
    bots[slot].x = spawnPoints[*spawnIndex].x;
    bots[slot].y = spawnPoints[*spawnIndex].y;

    // Тип случайный
    BotType type = (BotType)(rand() % 4);
    bots[slot].type  = type;
    bots[slot].speed = bot_speed(type);
    bots[slot].hp    = bot_hp(type);

    // Случайное начальное направление
    float dirs[4][2] = {{1,0},{-1,0},{0,-1},{0,1}};
    int d = rand() % 4;
    bots[slot].dirX = dirs[d][0];
    bots[slot].dirY = dirs[d][1];

    bots[slot].active              = 1;
    bots[slot].invincibleTimer     = INVINCIBLE_DURATION;
    bots[slot].flashTimer          = 0;
    bots[slot].deathTime           = 0.0;
    bots[slot].b_bullet.active     = 0;
    bots[slot].b_bullet.dirX       = bots[slot].dirX;
    bots[slot].b_bullet.dirY       = bots[slot].dirY;
    bots[slot].b_bullet.lastShootTime = currentTime;
    bots[slot].nextRotateTime      = currentTime + BOT_ROTATE_INTERVAL;

    *spawnIndex = (*spawnIndex + 1) % spawnCount;
}

// ── Основное обновление ───────────────────────────────────────────────
void bots_update(float deltaTime, double currentTime,
                 int fieldX, int fieldY, int fieldSize,
                 Spawner* spawnPoints, int spawnCount)
{
    static int    spawnIndex = 0;
    static double nextSpawn  = 0.0;

    if (!gBase.alive) return; // база уничтожена — боты уже победили

    // ── Спавн ────────────────────────────────────────────────────────
    int activeCount = 0;
    for (int i = 0; i < MAX_BOTS; i++)
        if (bots[i].active) activeCount++;

    if (activeCount < MAX_BOTS && spawnCount > 0
        && currentTime >= nextSpawn)
    {
        for (int i = 0; i < MAX_BOTS; i++) {
            if (!bots[i].active &&
                (bots[i].deathTime == 0 ||
                 currentTime - bots[i].deathTime >= BOT_SPAWN_INTERVAL))
            {
                spawn_bot(i, spawnPoints, spawnCount,
                          &spawnIndex, currentTime);
                nextSpawn = currentTime + BOT_SPAWN_INTERVAL;
                break;
            }
        }
    }

    float halfB = BOT_SIZE / 2.0f;
    float minXb = fieldX + halfB, maxXb = fieldX + fieldSize - halfB;
    float minYb = fieldY + halfB, maxYb = fieldY + fieldSize - halfB;

    for (int i = 0; i < MAX_BOTS; i++) {
        if (!bots[i].active) continue;

        // ── Поворот / навигация ───────────────────────────────────────
        if (currentTime >= bots[i].nextRotateTime) {
            float tx, ty;
            bot_target(&bots[i], &tx, &ty);

            float dx = tx - bots[i].x;
            float dy = ty - bots[i].y;

            float desiredDirX = 0.0f, desiredDirY = 0.0f;
            if (fabsf(dx) > fabsf(dy))
                desiredDirX = (dx > 0) ? 1.0f : -1.0f;
            else
                desiredDirY = (dy > 0) ? 1.0f : -1.0f;

            float candidates[4][2] = {
                { desiredDirX,  desiredDirY},
                { desiredDirY,  desiredDirX},
                {-desiredDirY, -desiredDirX},
                {-desiredDirX, -desiredDirY}
            };

            for (int d = 0; d < 4; d++) {
                float tx2 = bots[i].x + candidates[d][0] * bots[i].speed * deltaTime;
                float ty2 = bots[i].y + candidates[d][1] * bots[i].speed * deltaTime;
                if (tx2 < minXb || tx2 > maxXb ||
                    ty2 < minYb || ty2 > maxYb) continue;
                if (!check_rect_collision_with_map(COLLISION_BOT,
                        tx2, ty2, BOT_SIZE, BOT_SIZE, (float)i, 0)) {
                    bots[i].dirX = candidates[d][0];
                    bots[i].dirY = candidates[d][1];
                    break;
                }
            }
            bots[i].nextRotateTime = currentTime + BOT_ROTATE_INTERVAL;
        }

        // ── Движение ──────────────────────────────────────────────────
        float newX = bots[i].x + bots[i].dirX * bots[i].speed * deltaTime;
        float newY = bots[i].y + bots[i].dirY * bots[i].speed * deltaTime;
        newX = fmaxf(minXb, fminf(maxXb, newX));
        newY = fmaxf(minYb, fminf(maxYb, newY));

        if (!check_rect_collision_with_map(COLLISION_BOT,
                newX, newY, BOT_SIZE, BOT_SIZE, (float)i, 0)) {
            bots[i].x = newX;
            bots[i].y = newY;
            if (!bots[i].b_bullet.active) {
                bots[i].b_bullet.dirX = bots[i].dirX;
                bots[i].b_bullet.dirY = bots[i].dirY;
            }
        }

        // ── Неуязвимость ─────────────────────────────────────────────
        if (bots[i].invincibleTimer > 0.0f) {
            bots[i].invincibleTimer -= deltaTime;
            if (bots[i].invincibleTimer < 0.0f)
                bots[i].invincibleTimer = 0.0f;
        }

        if (bots[i].flashTimer > 0)
            bots[i].flashTimer--;

        // ── Стрельба ─────────────────────────────────────────────────
        if (!bots[i].b_bullet.active &&
            currentTime - bots[i].b_bullet.lastShootTime >= BOT_SHOOT_DELAY)
        {
            // Гончая и охотник целятся точнее — направляем пулю к цели
            if (bots[i].type == BOT_HOUND ||
                bots[i].type == BOT_HUNTER)
            {
                float tx, ty;
                bot_target(&bots[i], &tx, &ty);
                float dx = tx - bots[i].x;
                float dy = ty - bots[i].y;
                if (fabsf(dx) > fabsf(dy)) {
                    bots[i].b_bullet.dirX = (dx > 0) ? 1.0f : -1.0f;
                    bots[i].b_bullet.dirY = 0.0f;
                } else {
                    bots[i].b_bullet.dirX = 0.0f;
                    bots[i].b_bullet.dirY = (dy > 0) ? 1.0f : -1.0f;
                }
            }

            bots[i].b_bullet.active = 1;
            if (bots[i].b_bullet.dirX != 0) {
                bots[i].b_bullet.x =
                    bots[i].x + bots[i].b_bullet.dirX * BOT_SIZE / 2.0f;
                bots[i].b_bullet.y = bots[i].y;
            } else {
                bots[i].b_bullet.x = bots[i].x;
                bots[i].b_bullet.y =
                    bots[i].y + bots[i].b_bullet.dirY * BOT_SIZE / 2.0f;
            }
            bots[i].b_bullet.lastShootTime = currentTime;
        }

        // ── Обновление пули ───────────────────────────────────────────
        bullet_update(&bots[i].b_bullet, COLLISION_BOT_BULLET,
                      deltaTime, fieldX, fieldY, fieldSize);

    
    }
}
