#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <string.h>

#include <bots.h>
#include <player.h>
#include <collision.h>
#include <map.h>
#include <pathfinding.h>
#include <levels.h>
#include "sound.h"

Bot     bots[MAX_BOTS];
Base    gBase;
Spawner sp_bots[GRID_SIZE * GRID_SIZE];
static int  g_hound_saved[GRID_SIZE][GRID_SIZE];
static int  g_hound_blocked = 0;

// ── База ──────────────────────────────────────────────────────────────
void base_init(int fieldX, int fieldY)
{
    gBase.width  = 64.0f;
    gBase.height = 64.0f;
    gBase.x      = fieldX + (GRID_SIZE / 2) * CELL_SIZE + CELL_SIZE / 2.0f;
    gBase.y      = fieldY + (GRID_SIZE - 1) * CELL_SIZE + CELL_SIZE / 2.0f;
    gBase.hp     = 3;
    gBase.alive  = 1;
}

int base_is_destroyed(void) { return !gBase.alive; }

// ── Вспомогалки ───────────────────────────────────────────────────────
static float bot_speed(BotType type)
{
    switch (type) {
    case BOT_HOUND:   return BOT_SPEED_HOUND;
    case BOT_HUNTER:  return BOT_SPEED_HUNTER;
    case BOT_ARMORED: return BOT_SPEED_ARMORED;
    default:          return BOT_SPEED_NORMAL;
    }
}

static int bot_hp(BotType type)
{
    return (type == BOT_ARMORED) ? 3 : 1;
}

static int world_to_grid(float worldPos, int fieldOrigin)
{
    return (int)((worldPos - fieldOrigin) / CELL_SIZE);
}

static float grid_to_world(int cell, int fieldOrigin)
{
    return fieldOrigin + cell * CELL_SIZE + CELL_SIZE / 2.0f;
}

// Возвращает направление выстрела (sdx, sdy) или 0,0 если не выровнен
static int bot_aligned_with_player(Bot* b, float* sdx, float* sdy)
{
    float dx = player.x - b->x;
    float dy = player.y - b->y;

    // Допуск — половина клетки. Бот считается выровненным если
    // перпендикулярное смещение меньше половины CELL_SIZE.
    // Маленький допуск (halfBullet=5px) делал стрельбу практически невозможной.
    float tolerance = CELL_SIZE / 2.0f;

    if (fabsf(dx) > fabsf(dy)) {
        if (fabsf(dy) <= tolerance) {
            *sdx = (dx > 0) ? 1.0f : -1.0f;
            *sdy = 0.0f;
            return 1;
        }
    } else {
        if (fabsf(dx) <= tolerance) {
            *sdx = 0.0f;
            *sdy = (dy > 0) ? 1.0f : -1.0f;
            return 1;
        }
    }
    return 0;
}

// Проверяет прямую видимость по прямой между ботом и игроком
static int bot_has_clear_shot(Bot* b, float sdx, float sdy, int fieldX, int fieldY)
{
    int bi = world_to_grid(b->x, fieldX);
    int bj = world_to_grid(b->y, fieldY);
    int pi = world_to_grid(player.x, fieldX);
    int pj = world_to_grid(player.y, fieldY);

    if (sdx != 0) {
        // Горизонтальный выстрел — проверяем клетки по X
        int minI = (bi < pi) ? bi : pi;
        int maxI = (bi > pi) ? bi : pi;
        for (int i = minI + 1; i < maxI; i++)
            if (map[bj][i] == 2 || map[bj][i] == 3) return 0;
    } else {
        // Вертикальный выстрел — проверяем клетки по Y
        int minJ = (bj < pj) ? bj : pj;
        int maxJ = (bj > pj) ? bj : pj;
        for (int j = minJ + 1; j < maxJ; j++)
            if (map[j][bi] == 2 || map[j][bi] == 3) return 0;
    }
    return 1;
}

// В bots.c, после bot_has_clear_shot:
static int bot_has_clear_shot_to(Bot* b, float tx, float ty,
                                  float sdx, float sdy,
                                  int fieldX, int fieldY)
{
    int bi = world_to_grid(b->x,  fieldX);
    int bj = world_to_grid(b->y,  fieldY);
    int ti = world_to_grid(tx,    fieldX);
    int tj = world_to_grid(ty,    fieldY);

    if (sdx != 0) {
        int minI = (bi < ti) ? bi : ti;
        int maxI = (bi > ti) ? bi : ti;
        for (int i = minI + 1; i < maxI; i++)
            if (map[bj][i] == 2 || map[bj][i] == 3) return 0;
    } else {
        int minJ = (bj < tj) ? bj : tj;
        int maxJ = (bj > tj) ? bj : tj;
        for (int j = minJ + 1; j < maxJ; j++)
            if (map[j][bi] == 2 || map[j][bi] == 3) return 0;
    }
    return 1;
}

static void bot_try_shoot(Bot* b, float tx, float ty, double currentTime)
{
    if (b->b_bullet.active) return;
    if (currentTime - b->b_bullet.lastShootTime < BOT_SHOOT_DELAY) return;

    float dx = tx - b->x;
    float dy = ty - b->y;

    float sdx = 0.0f, sdy = 0.0f;
    if (fabsf(dx) > fabsf(dy)) sdx = (dx > 0) ? 1.0f : -1.0f;
    else                        sdy = (dy > 0) ? 1.0f : -1.0f;

    // Обновляем взгляд бота в сторону выстрела
    b->faceX = sdx;
    b->faceY = sdy;

    b->b_bullet.dirX          = sdx;
    b->b_bullet.dirY          = sdy;
    b->b_bullet.active        = 1;
    b->b_bullet.x             = b->x + sdx * BOT_SIZE / 2.0f;
    b->b_bullet.y             = b->y + sdy * BOT_SIZE / 2.0f;
    b->b_bullet.lastShootTime = currentTime;
    sound_play("bot_shoot");
}

// ── Спавн одного бота из очереди уровня ──────────────────────────────
static void spawn_next_bot(Spawner* spawnPoints, int spawnCount,
                           double currentTime)
{
    if (spawnCount <= 0) return;

    Level* lvl = &levels[currentLevelIndex];
    if (botQueueIndex >= lvl->botCount) return;

    // Считаем активных ботов
    int activeCount = 0;
    for (int i = 0; i < MAX_BOTS; i++)
        if (bots[i].active) activeCount++;
    if (activeCount >= MAX_BOTS) return;

    BotWave* wave = &lvl->botQueue[botQueueIndex];

    // Индекс точки спавна из очереди
    int spIdx = wave->spawnPointIndex;
    if (spIdx >= spawnCount) spIdx = 0;

    // Проверяем что точка спавна свободна
    for (int k = 0; k < MAX_BOTS; k++) {
        if (!bots[k].active) continue;
        float dx = fabsf(bots[k].x - spawnPoints[spIdx].x);
        float dy = fabsf(bots[k].y - spawnPoints[spIdx].y);
        if (dx < BOT_SIZE && dy < BOT_SIZE) return; // занято — ждём
    }

    // Ищем свободный слот
    for (int i = 0; i < MAX_BOTS; i++) {
        if (!bots[i].active &&
            (bots[i].deathTime == 0 ||
             currentTime - bots[i].deathTime >= BOT_SPAWN_INTERVAL))
        {
            bots[i].x               = spawnPoints[spIdx].x;
            bots[i].y               = spawnPoints[spIdx].y;
            bots[i].type            = wave->type;
            bots[i].speed           = bot_speed(wave->type);
            bots[i].hp              = bot_hp(wave->type);
            bots[i].dirX            = 0.0f;
            bots[i].dirY            = 0.0f;
            bots[i].active          = 1;
            bots[i].invincibleTimer = INVINCIBLE_DURATION;
            bots[i].flashTimer      = 0;
            bots[i].deathTime       = 0.0;
            bots[i].stuckTimer      = currentTime;
            bots[i].b_bullet.active = 0;
            bots[i].b_bullet.lastShootTime = currentTime;
            bots[i].targetCellX     = -1;
            bots[i].targetCellY     = -1;
            bots[i].animFrame    = 0;
            bots[i].lastAnimTime = 0.0;
            bots[i].faceX        = 0.0f;
            bots[i].faceY        = -1.0f; // смотрит вниз к полю по умолчанию

            botQueueIndex++;
            return;
        }
    }
}

static void hound_block_player(int fieldX, int fieldY,
                                int blocked[GRID_SIZE][GRID_SIZE])
{
    int pi = world_to_grid(player.x, fieldX);
    int pj = world_to_grid(player.y, fieldY);
    int radius = 3; // радиус обхода в клетках
    for (int dj = -radius; dj <= radius; dj++) {
        for (int di = -radius; di <= radius; di++) {
            int ni = pi + di, nj = pj + dj;
            if (ni < 0 || ni >= GRID_SIZE || nj < 0 || nj >= GRID_SIZE) continue;
            if (map[nj][ni] == 0 || map[nj][ni] == 4) { // только свободные
                blocked[nj][ni] = map[nj][ni];
                map[nj][ni] = 99; // временная метка "непроходимо"
            }
        }
    }
}

static void hound_unblock_player(int blocked[GRID_SIZE][GRID_SIZE],
                                  int fieldX, int fieldY)
{
    int pi = world_to_grid(player.x, fieldX);
    int pj = world_to_grid(player.y, fieldY);
    int radius = 3;
    for (int dj = -radius; dj <= radius; dj++) {
        for (int di = -radius; di <= radius; di++) {
            int ni = pi + di, nj = pj + dj;
            if (ni < 0 || ni >= GRID_SIZE || nj < 0 || nj >= GRID_SIZE) continue;
            if (blocked[nj][ni] != -1) {
                map[nj][ni] = blocked[nj][ni];
                blocked[nj][ni] = -1;
            }
        }
    }
}

static void hound_apply_block(int fieldX, int fieldY, int botI, int botJ)
{
    int pi = world_to_grid(player.x, fieldX);
    int pj = world_to_grid(player.y, fieldY);
    memset(g_hound_saved, -1, sizeof(g_hound_saved));
    for (int dj = -3; dj <= 3; dj++) {
        for (int di = -3; di <= 3; di++) {
            int ni = pi + di, nj = pj + dj;
            if (ni < 0 || ni >= GRID_SIZE || nj < 0 || nj >= GRID_SIZE) continue;
            if (ni == botI && nj == botJ) continue;
            if (map[nj][ni] == 0 || map[nj][ni] == 4 || map[nj][ni] == 6) {
                g_hound_saved[nj][ni] = map[nj][ni];
                map[nj][ni] = 2;
            }
        }
    }
    g_hound_blocked = 1;
}

static void hound_remove_block(void)
{
    if (!g_hound_blocked) return;
    for (int j = 0; j < GRID_SIZE; j++)
        for (int i = 0; i < GRID_SIZE; i++)
            if (g_hound_saved[j][i] != -1)
                map[j][i] = g_hound_saved[j][i];
    g_hound_blocked = 0;
}

// ── Основное обновление ───────────────────────────────────────────────
void bots_update(float deltaTime, double currentTime,
                 int fieldX, int fieldY, int fieldSize,
                 Spawner* spawnPoints, int spawnCount)
{
    if (!gBase.alive) return;

    // ── Спавн из очереди ──────────────────────────────────────────────
    static double lastSpawn = 0.0;
    if (currentTime - lastSpawn >= BOT_SPAWN_INTERVAL) {
        spawn_next_bot(spawnPoints, spawnCount, currentTime);
        lastSpawn = currentTime;
    }

    float halfB = BOT_SIZE / 2.0f;
    float minXb = fieldX + halfB;
    float maxXb = fieldX + fieldSize - halfB;
    float minYb = fieldY + halfB;
    float maxYb = fieldY + fieldSize - halfB;

    for (int i = 0; i < MAX_BOTS; i++) {
        if (!bots[i].active) continue;

        // ── Таймеры ───────────────────────────────────────────────────
        if (bots[i].invincibleTimer > 0.0f) {
            bots[i].invincibleTimer -= deltaTime;
            if (bots[i].invincibleTimer < 0.0f) bots[i].invincibleTimer = 0.0f;
        }
        if (bots[i].flashTimer > 0) bots[i].flashTimer--;

        // ── Текущая клетка ────────────────────────────────────────────
        int bi = world_to_grid(bots[i].x, fieldX);
        int bj = world_to_grid(bots[i].y, fieldY);

        // ── Цель по типу ──────────────────────────────────────────────
        int goalI, goalJ;
        switch (bots[i].type) {
        case BOT_HOUND:
            goalI = world_to_grid(gBase.x, fieldX);
            goalJ = world_to_grid(gBase.y, fieldY);
            break;
        case BOT_HUNTER:
            goalI = world_to_grid(player.x, fieldX);
            goalJ = world_to_grid(player.y, fieldY);
            break;
        default: // BOT_NORMAL, BOT_ARMORED — едут к базе
            goalI = world_to_grid(gBase.x, fieldX);
            goalJ = world_to_grid(gBase.y, fieldY);
            break;
        }

        // ── Стрельба по типу ──────────────────────────────────────────────
        // ── Стрельба ──────────────────────────────────────────────────────────
        float shotDirX = 0.0f, shotDirY = 0.0f;

        // 1. Стрельба по игроку (все типы кроме чистого гончего)
        if (bots[i].type != BOT_HOUND) {
            if (bot_aligned_with_player(&bots[i], &shotDirX, &shotDirY) &&
                bot_has_clear_shot(&bots[i], shotDirX, shotDirY, fieldX, fieldY))
            {
                bot_try_shoot(&bots[i], player.x, player.y, currentTime);
            }
        }

        // 2. Стрельба по базе (все типы)
        if (!bots[i].b_bullet.active && gBase.alive) {
            float dx = gBase.x - bots[i].x;
            float dy = gBase.y - bots[i].y;
            float bsdx = 0.0f, bsdy = 0.0f;

            // Выравниваем как и с игроком
            if (fabsf(dx) > fabsf(dy)) {
                if (fabsf(dy) <= BULLET_HEIGHT / 2.0f)
                    bsdx = (dx > 0) ? 1.0f : -1.0f;
            } else {
                if (fabsf(dx) <= BULLET_HEIGHT / 2.0f)
                    bsdy = (dy > 0) ? 1.0f : -1.0f;
            }

            if ((bsdx != 0.0f || bsdy != 0.0f) &&
                bot_has_clear_shot_to(&bots[i], gBase.x, gBase.y,
                                      bsdx, bsdy, fieldX, fieldY))
            {
                bot_try_shoot(&bots[i], gBase.x, gBase.y, currentTime);
            }
        }

        // ── Таймер застревания ────────────────────────────────────────
        if (bots[i].dirX == 0.0f && bots[i].dirY == 0.0f) {
            if (currentTime - bots[i].stuckTimer > 1.0) {
                bots[i].targetCellX = -1;
                bots[i].targetCellY = -1;
                bots[i].stuckTimer  = currentTime;
            }
        } else {
            bots[i].stuckTimer = currentTime;
        }

        // ── Навигация A* ──────────────────────────────────────────────
        // ── Навигация A* (пересчёт только по таймеру) ─────────────────────────
        int hasTarget = (bots[i].targetCellX >= 0 && bots[i].targetCellY >= 0);

        float tCX = hasTarget ? grid_to_world(bots[i].targetCellX, fieldX) : bots[i].x;
        float tCY = hasTarget ? grid_to_world(bots[i].targetCellY, fieldY) : bots[i].y;

        float distToTarget = sqrtf(
            (bots[i].x - tCX) * (bots[i].x - tCX) +
            (bots[i].y - tCY) * (bots[i].y - tCY));
        int reachedTarget = hasTarget && (distToTarget < 2.0f);

        // Пересчёт пути сразу при достижении клетки или при отсутствии цели.
        // Паузы между клетками убраны — они и создавали рывки.
        // Антипаровозик реализован через случайное смещение целевой клетки.
        if (!hasTarget || reachedTarget) {
            if (reachedTarget) {
                // Фиксируем позицию строго по центру клетки
                bots[i].x = grid_to_world(bi, fieldX);
                bots[i].y = grid_to_world(bj, fieldY);
            }

            // Антипаровозик: Normal/Armored с шансом 25% идут к соседней
            // клетке от цели — это разбивает колонну без остановок.
            int rGoalI = goalI, rGoalJ = goalJ;
            if (bots[i].type == BOT_NORMAL || bots[i].type == BOT_ARMORED) {
                if (rand() % 4 == 0) {
                    int offsets[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};
                    int r = rand() % 4;
                    int ni = goalI + offsets[r][0];
                    int nj = goalJ + offsets[r][1];
                    if (ni >= 0 && ni < GRID_SIZE &&
                        nj >= 0 && nj < GRID_SIZE &&
                        (map[nj][ni] == 0 || map[nj][ni] == 4 || map[nj][ni] == 5))
                    {
                        rGoalI = ni;
                        rGoalJ = nj;
                    }
                }
            }

            // Для Hound — блокируем зону вокруг игрока
            if (bots[i].type == BOT_HOUND)
                hound_apply_block(fieldX, fieldY, bi, bj);

            int savedTile = map[rGoalJ][rGoalI];
            if (savedTile != 0) map[rGoalJ][rGoalI] = 0;

            PFPoint next = pathfind_next_step(bi, bj, rGoalI, rGoalJ, 1);

            map[rGoalJ][rGoalI] = savedTile;

            if (bots[i].type == BOT_HOUND)
                hound_remove_block();

            if (next.x != -1) {
                bots[i].targetCellX = next.x;
                bots[i].targetCellY = next.y;
                bots[i].dirX = (float)(next.x - bi);
                bots[i].dirY = (float)(next.y - bj);

                // Фиксируем перпендикулярную ось чтобы не было сноса
                if (bots[i].dirX != 0.0f)
                    bots[i].y = grid_to_world(bj, fieldY);
                if (bots[i].dirY != 0.0f)
                    bots[i].x = grid_to_world(bi, fieldX);

                // Следующая клетка — дерево: стреляем и ждём
                if (map[next.y][next.x] == 5) {
                    bot_try_shoot(&bots[i],
                                  grid_to_world(next.x, fieldX),
                                  grid_to_world(next.y, fieldY),
                                  currentTime);
                    bots[i].dirX        = 0.0f;
                    bots[i].dirY        = 0.0f;
                    bots[i].targetCellX = bi;
                    bots[i].targetCellY = bj;
                }
            } else {
                bots[i].dirX        = 0.0f;
                bots[i].dirY        = 0.0f;
                bots[i].targetCellX = bi;
                bots[i].targetCellY = bj;
            }
        }

        // Запоминаем направление взгляда пока бот движется.
        // faceX/faceY не обнуляются при остановке — рендер использует их.
        if (bots[i].dirX != 0.0f || bots[i].dirY != 0.0f) {
            bots[i].faceX = bots[i].dirX;
            bots[i].faceY = bots[i].dirY;
        }

        // ── Движение к целевой клетке ─────────────────────────────────
        if (bots[i].dirX != 0.0f || bots[i].dirY != 0.0f) {
            float newX = bots[i].x + bots[i].dirX * bots[i].speed * deltaTime;
            float newY = bots[i].y + bots[i].dirY * bots[i].speed * deltaTime;

            // Не перелетаем центр целевой клетки
            float tcX = grid_to_world(bots[i].targetCellX, fieldX);
            float tcY = grid_to_world(bots[i].targetCellY, fieldY);
            if (bots[i].dirX > 0.0f && newX > tcX) newX = tcX;
            if (bots[i].dirX < 0.0f && newX < tcX) newX = tcX;
            if (bots[i].dirY > 0.0f && newY > tcY) newY = tcY;
            if (bots[i].dirY < 0.0f && newY < tcY) newY = tcY;

            newX = fmaxf(minXb, fminf(maxXb, newX));
            newY = fmaxf(minYb, fminf(maxYb, newY));

            if (!check_rect_collision_with_map(COLLISION_BOT,
                    newX, newY, BOT_SIZE, BOT_SIZE, (float)i, 0)) {
                bots[i].x = newX;
                bots[i].y = newY;
            }
            // При коллизии просто стоим — stuckTimer пересчитает путь
        }

        // ── Обновление пули ───────────────────────────────────────────
        bullet_update(&bots[i].b_bullet, COLLISION_BOT_BULLET,
                      deltaTime, fieldX, fieldY, fieldSize);
    }
}
