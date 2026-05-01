#include <math.h>
#include <stdlib.h>
#include <stdio.h>
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

// ── Вспомогательная логика ИИ ─────────────────────────────────────────

static int reservation[GRID_SIZE][GRID_SIZE];

static float bot_speed(BotType type) {
    switch (type) {
        case BOT_HOUND:   return BOT_SPEED_HOUND;   // Желтые (быстрые)
        case BOT_HUNTER:  return BOT_SPEED_HUNTER;  // Охотники
        case BOT_ARMORED: return BOT_SPEED_ARMORED;
        default:          return BOT_SPEED_NORMAL;
    }
}

// Оценка стоимости клетки
static int get_tile_cost(int x, int y, int selfIdx) {
    if (x < 0 || x >= GRID_SIZE || y < 0 || y >= GRID_SIZE) return 1000;
    
    // Если клетка забронирована другим ботом — она очень дорогая
    if (reservation[y][x] != -1 && reservation[y][x] != selfIdx) return 50;

    int tile = map[y][x];
    if (tile == 0 || tile == 4) return 1;    // Пусто/Трава
    if (tile == 5) return 10;               // Дерево
    if (tile == 2 || tile == 3) return 1000; // Стена/Вода[cite: 12]
    return 1;
}

static void bot_choose_best_direction(Bot* b, int selfIdx, float tx, float ty, int fX, int fY) {
    int curI = (int)((b->x - fX) / CELL_SIZE);
    int curJ = (int)((b->y - fY) / CELL_SIZE);

    float bestScore = 1e10f;
    float bestDX = 0, bestDY = 0;
    float dirs[4][2] = {{0, 1}, {0, -1}, {1, 0}, {-1, 0}};

    for (int i = 0; i < 4; i++) {
        int nextI = curI + (int)dirs[i][0];
        int nextJ = curJ + (int)dirs[i][1];

        int cost = get_tile_cost(nextI, nextJ, selfIdx);
        if (cost >= 1000) continue;

        // Расстояние от центра целевой клетки до цели
        float targetCellX = nextI * CELL_SIZE + fX + CELL_SIZE/2;
        float targetCellY = nextJ * CELL_SIZE + fY + CELL_SIZE/2;
        float dist = sqrtf(powf(targetCellX - tx, 2) + powf(targetCellY - ty, 2));
        
        float score = dist + (cost * CELL_SIZE);

        if (score < bestScore) {
            bestScore = score;
            bestDX = dirs[i][0];
            bestDY = dirs[i][1];
        }
    }

    // Если нашли путь, бронируем клетку
    if (bestDX != 0 || bestDY != 0) {
        b->dirX = bestDX;
        b->dirY = bestDY;
        int nextI = curI + (int)bestDX;
        int nextJ = curJ + (int)bestDY;
        if (nextI >= 0 && nextI < GRID_SIZE && nextJ >= 0 && nextJ < GRID_SIZE) {
            reservation[nextJ][nextI] = selfIdx;
        }
    }
}

void bots_update(float deltaTime, double currentTime,
                 int fieldX, int fieldY, int fieldSize,
                 Spawner* spawnPoints, int spawnCount)
{
    // Очистка брони перед циклом
    for(int y=0; y<GRID_SIZE; y++) for(int x=0; x<GRID_SIZE; x++) reservation[y][x] = -1;

    // Спавн (без изменений)[cite: 12]
    static double lastSpawn = 0;
    if (currentTime - lastSpawn > BOT_SPAWN_INTERVAL) {
        for (int i = 0; i < MAX_BOTS; i++) {
            if (!bots[i].active) {
                int sIdx = rand() % spawnCount;
                bots[i].active = 1;
                bots[i].x = spawnPoints[sIdx].x;
                bots[i].y = spawnPoints[sIdx].y;
                bots[i].type = (BotType)(rand() % 4);
                bots[i].speed = bot_speed(bots[i].type);
                bots[i].hp = (bots[i].type == BOT_ARMORED) ? 3 : 1;
                bots[i].dirX = 0; bots[i].dirY = 1;
                bots[i].nextRotateTime = 0;
                lastSpawn = currentTime;
                break;
            }
        }
    }

    for (int i = 0; i < MAX_BOTS; i++) {
        if (!bots[i].active) continue;

        float tx, ty;
        // Охотники (Hunter) едут к игроку, остальные — к базе[cite: 12]
        if (bots[i].type == BOT_HUNTER) { tx = player.x; ty = player.y; }
        else { tx = gBase.x; ty = gBase.y; }

        // 1. Логика выбора клетки
        int curI = (int)((bots[i].x - fieldX) / CELL_SIZE);
        int curJ = (int)((bots[i].y - fieldY) / CELL_SIZE);
        float centerX = curI * CELL_SIZE + fieldX + CELL_SIZE/2;
        float centerY = curJ * CELL_SIZE + fieldY + CELL_SIZE/2;

        // Пересчитываем путь, если бот в центре клетки или стоит
        float dToCenter = sqrtf(powf(bots[i].x - centerX, 2) + powf(bots[i].y - centerY, 2));
        if (dToCenter < 5.0f || (bots[i].dirX == 0 && bots[i].dirY == 0)) {
            bot_choose_best_direction(&bots[i], i, tx, ty, fieldX, fieldY);
        }

        // 2. Плавное выравнивание по оси
        if (bots[i].dirY != 0) {
            float dx = centerX - bots[i].x;
            if (fabsf(dx) > 0.5f) bots[i].x += (dx > 0 ? 1 : -1) * bots[i].speed * 0.5f * deltaTime;
        } else if (bots[i].dirX != 0) {
            float dy = centerY - bots[i].y;
            if (fabsf(dy) > 0.5f) bots[i].y += (dy > 0 ? 1 : -1) * bots[i].speed * 0.5f * deltaTime;
        }

        // 3. Стрельба по преградам[cite: 12]
        int aheadI = curI + (int)bots[i].dirX;
        int aheadJ = curJ + (int)bots[i].dirY;
        if (aheadI >= 0 && aheadI < GRID_SIZE && aheadJ >= 0 && aheadJ < GRID_SIZE) {
            if (map[aheadJ][aheadI] == 5 && !bots[i].b_bullet.active) {
                if (currentTime - bots[i].b_bullet.lastShootTime >= BOT_SHOOT_DELAY) {
                    bots[i].b_bullet.active = 1;
                    bots[i].b_bullet.x = bots[i].x; bots[i].b_bullet.y = bots[i].y;
                    bots[i].b_bullet.dirX = bots[i].dirX; bots[i].b_bullet.dirY = bots[i].dirY;
                    bots[i].b_bullet.lastShootTime = currentTime;
                }
            }
        }

        // 4. Движение
        float nx = bots[i].x + bots[i].dirX * bots[i].speed * deltaTime;
        float ny = bots[i].y + bots[i].dirY * bots[i].speed * deltaTime;

        // Проверка коллизий с запасом[cite: 12]
        if (!check_rect_collision_with_map(COLLISION_BOT, nx, ny, BOT_SIZE - 6, BOT_SIZE - 6, (float)i, 0)) {
            bots[i].x = nx;
            bots[i].y = ny;
        } else {
            // Если уперлись в другого бота или игрока — обнуляем направление, чтобы пересчитать на след. кадре
            bots[i].dirX = 0; bots[i].dirY = 0;
        }

        bullet_update(&bots[i].b_bullet, COLLISION_BOT_BULLET, deltaTime, fieldX, fieldY, fieldSize);
    }
}
