#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <bots.h>
#include <player.h>
#include <collision.h>
#include <map.h>
#include <pathfinding.h>
#include <levels.h>

Bot     bots[MAX_BOTS];
Base    gBase;
Spawner sp_bots[GRID_SIZE * GRID_SIZE];

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

// Прямая видимость игрока по горизонтали или вертикали
static int bot_has_line_of_sight(Bot* b, int fieldX, int fieldY)
{
    int bi = world_to_grid(b->x, fieldX);
    int bj = world_to_grid(b->y, fieldY);
    int pi = world_to_grid(player.x, fieldX);
    int pj = world_to_grid(player.y, fieldY);

    if (bi != pi && bj != pj) return 0;

    if (bi == pi) {
        int minJ = (bj < pj) ? bj : pj;
        int maxJ = (bj > pj) ? bj : pj;
        for (int j = minJ + 1; j < maxJ; j++)
            if (map[j][bi] == 2 || map[j][bi] == 3) return 0;
        return 1;
    } else {
        int minI = (bi < pi) ? bi : pi;
        int maxI = (bi > pi) ? bi : pi;
        for (int i = minI + 1; i < maxI; i++)
            if (map[bj][i] == 2 || map[bj][i] == 3) return 0;
        return 1;
    }
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

    b->b_bullet.dirX          = sdx;
    b->b_bullet.dirY          = sdy;
    b->b_bullet.active        = 1;
    b->b_bullet.x             = b->x + sdx * BOT_SIZE / 2.0f;
    b->b_bullet.y             = b->y + sdy * BOT_SIZE / 2.0f;
    b->b_bullet.lastShootTime = currentTime;
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

            botQueueIndex++;
            return;
        }
    }
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

        // ── Стрельба по типу ──────────────────────────────────────────
        switch (bots[i].type) {
        case BOT_HOUND:
            // Стреляет по базе если на одной оси
            if (gBase.alive) {
                float dx = fabsf(bots[i].x - gBase.x);
                float dy = fabsf(bots[i].y - gBase.y);
                if (dx < CELL_SIZE / 2.0f || dy < CELL_SIZE / 2.0f)
                    bot_try_shoot(&bots[i], gBase.x, gBase.y, currentTime);
            }
            break;
        case BOT_HUNTER:
            // Стреляет по игроку если видит
            if (bot_has_line_of_sight(&bots[i], fieldX, fieldY))
                bot_try_shoot(&bots[i], player.x, player.y, currentTime);
            break;
        default:
            // Едут к базе, но стреляют по игроку если видят
            if (bot_has_line_of_sight(&bots[i], fieldX, fieldY))
                bot_try_shoot(&bots[i], player.x, player.y, currentTime);
            break;
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
        int hasTarget = (bots[i].targetCellX >= 0 &&
                         bots[i].targetCellY >= 0);

        float tCX = hasTarget
                    ? grid_to_world(bots[i].targetCellX, fieldX)
                    : bots[i].x;
        float tCY = hasTarget
                    ? grid_to_world(bots[i].targetCellY, fieldY)
                    : bots[i].y;

        float distToTarget = sqrtf(
            (bots[i].x - tCX) * (bots[i].x - tCX) +
            (bots[i].y - tCY) * (bots[i].y - tCY));
        int reachedTarget = hasTarget && (distToTarget < 2.0f);

        if (!hasTarget || reachedTarget) {
            // Фиксируем позицию точно по центру клетки
            bots[i].x = grid_to_world(bi, fieldX);
            bots[i].y = grid_to_world(bj, fieldY);

            // Временно очищаем целевую клетку чтобы A* мог её достичь
            int savedTile = map[goalJ][goalI];
            if (savedTile != 0) map[goalJ][goalI] = 0;
            PFPoint next = pathfind_next_step(bi, bj, goalI, goalJ, 1);
            map[goalJ][goalI] = savedTile;

            if (next.x != -1) {
                bots[i].targetCellX = next.x;
                bots[i].targetCellY = next.y;
                bots[i].dirX = (float)(next.x - bi);
                bots[i].dirY = (float)(next.y - bj);

                // Фиксируем перпендикулярную ось
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
