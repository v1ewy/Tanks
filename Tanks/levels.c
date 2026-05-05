#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <string.h>
#include <map.h>
#include <levels.h>
#include <player.h>
#include <bots.h>
#include <score.h>

extern int fieldX, fieldY;

Level levels[TOTAL_LEVELS];

int botQueueIndex     = 0;
int currentLevelIndex = 0;

void levels_init(void)
{
    // ── Уровень 1: Открытое поле ──────────────────────────────
    int l1[GRID_SIZE][GRID_SIZE] = {
        {0,6,2,0,0,0,6,0,0,0,2,6,0},
        {5,5,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0},
        {4,4,4,4,0,3,3,3,0,4,4,4,4},
        {0,5,5,5,5,5,5,5,5,5,5,0,0},
        {2,2,5,0,4,4,4,4,4,0,5,2,2},
        {0,2,5,5,4,4,4,4,4,5,5,2,0},
        {0,0,0,0,5,5,5,5,5,0,0,0,0},
        {0,0,0,5,0,3,0,3,0,5,0,0,0},
        {4,0,0,5,5,3,3,3,5,5,0,0,4},
        {4,5,5,5,2,2,2,2,2,5,5,5,4},
        {4,0,0,0,0,5,5,5,0,0,0,0,4},
        {4,4,0,0,1,5,0,5,0,0,0,4,4},
    };
    memcpy(levels[0].map, l1, sizeof(l1));
    levels[0].player_lives = 3;
    levels[0].name         = "OPEN FIELD";
    levels[0].botCount     = 12;
    levels[0].botCurrent   = 12;
    BotWave w1[] = {
        {BOT_NORMAL,  0},
        {BOT_NORMAL,  1},
        {BOT_NORMAL,  2},
        {BOT_NORMAL,  1},
        {BOT_NORMAL,  0},
        {BOT_NORMAL,  1},
        {BOT_NORMAL,  2},
        {BOT_NORMAL,  1},
        {BOT_HUNTER,  0},
        {BOT_HUNTER,  1},
        {BOT_HUNTER,  2},
        {BOT_HUNTER,  1},
    };
    memcpy(levels[0].botQueue, w1, sizeof(w1));

    // ── Уровень 2: Городской бой ──────────────────────────────
    int l2[GRID_SIZE][GRID_SIZE] = {
        {6,0,0,2,0,0,0,0,0,2,0,0,6},
        {0,2,0,2,0,2,2,2,0,2,0,2,0},
        {0,2,0,0,0,2,0,2,0,0,0,2,0},
        {0,2,0,2,0,0,0,0,0,2,0,2,0},
        {0,0,0,2,2,0,3,0,2,2,0,0,0},
        {2,2,0,0,0,0,3,0,0,0,0,2,2},
        {0,0,0,2,0,0,3,0,0,2,0,0,0},
        {0,2,0,2,0,4,4,4,0,2,0,2,0},
        {0,2,0,0,0,4,0,4,0,0,0,2,0},
        {0,2,5,2,0,0,0,0,0,2,5,2,0},
        {0,0,0,2,0,2,0,2,0,2,0,0,0},
        {6,0,0,0,0,5,5,5,0,0,0,0,6},
        {0,0,0,0,1,5,0,5,0,0,0,0,0},
    };
    memcpy(levels[1].map, l2, sizeof(l2));
    levels[1].player_lives = 3;
    levels[1].name         = "URBAN COMBAT";
    levels[1].botCount     = 10;
    levels[1].botCurrent   = 10;
    BotWave w2[] = {
        {BOT_NORMAL,  0},
        {BOT_NORMAL,  1},
        {BOT_HUNTER,  0},
        {BOT_HOUND,   1},
        {BOT_NORMAL,  0},
        {BOT_HUNTER,  1},
        {BOT_HOUND,   0},
        {BOT_ARMORED, 1},
        {BOT_HUNTER,  0},
        {BOT_ARMORED, 1},
    };
    memcpy(levels[1].botQueue, w2, sizeof(w2));

    // ── Уровень 3: Крепость ───────────────────────────────────
    int l3[GRID_SIZE][GRID_SIZE] = {
        {6,2,2,2,2,2,2,2,2,2,2,2,6},
        {2,0,0,0,0,5,0,5,0,0,0,0,2},
        {2,0,2,2,0,0,0,0,0,2,2,0,2},
        {2,0,2,0,0,3,3,3,0,0,2,0,2},
        {2,0,0,0,3,0,0,0,3,0,0,0,2},
        {2,5,0,3,0,0,4,0,0,3,0,5,2},
        {2,0,0,3,0,4,4,4,0,3,0,0,2},
        {2,0,0,0,3,0,0,0,3,0,0,0,2},
        {2,0,2,0,0,3,3,3,0,0,2,0,2},
        {2,0,2,2,0,0,0,0,0,2,2,0,2},
        {2,0,0,0,0,5,0,5,0,0,0,0,2},
        {2,0,0,0,0,0,0,0,0,0,0,0,2},
        {6,2,2,2,2,2,1,2,2,2,2,2,6},
    };
    memcpy(levels[2].map, l3, sizeof(l3));
    levels[2].player_lives = 5;
    levels[2].name         = "FORTRESS";
    levels[2].botCount     = 14;
    levels[2].botCurrent   = 14;
    BotWave w3[] = {
        {BOT_HOUND,   0},
        {BOT_HOUND,   1},
        {BOT_HUNTER,  0},
        {BOT_HUNTER,  1},
        {BOT_ARMORED, 0},
        {BOT_NORMAL,  1},
        {BOT_HOUND,   0},
        {BOT_HUNTER,  1},
        {BOT_ARMORED, 0},
        {BOT_ARMORED, 1},
        {BOT_HOUND,   0},
        {BOT_HUNTER,  1},
        {BOT_ARMORED, 0},
        {BOT_ARMORED, 1},
    };
    memcpy(levels[2].botQueue, w3, sizeof(w3));
}

void load_level(int index)
{
    currentLevelIndex = index;
    botQueueIndex     = 0;

    memcpy(map, levels[index].map, sizeof(int) * GRID_SIZE * GRID_SIZE);

    // ── Спавн игрока ──────────────────────────────────────────
    int found = 0;
    for (int j = 0; j < GRID_SIZE && !found; j++) {
        for (int i = 0; i < GRID_SIZE && !found; i++) {
            if (map[j][i] == 1) {
                sp_player.x = fieldX + i * CELL_SIZE + CELL_SIZE / 2.0f;
                sp_player.y = fieldY + j * CELL_SIZE + CELL_SIZE / 2.0f;
                found = 1;
            }
        }
    }
    if (!found) {
        sp_player.x = fieldX + FIELD_SIZE / 2.0f;
        sp_player.y = fieldY + FIELD_SIZE / 2.0f;
    }

    player.x               = sp_player.x;
    player.y               = sp_player.y;
    player.dead            = 0;
    player.lives           = levels[index].player_lives;
    player.invincibleTimer = INVINCIBLE_DURATION;
    player.p_bullet.active = 0;
    player.p_bullet.dirX   = 0.0f;
    player.p_bullet.dirY   = -1.0f;
    player.p_bullet.lastShootTime = 0.0;

    // ── Спавны ботов из карты ─────────────────────────────────
    int spawn_count = 0;
    for (int j = 0; j < GRID_SIZE; j++) {
        for (int i = 0; i < GRID_SIZE; i++) {
            if (map[j][i] == 6) {
                sp_bots[spawn_count].x =
                    fieldX + i * CELL_SIZE + CELL_SIZE / 2.0f;
                sp_bots[spawn_count].y =
                    fieldY + j * CELL_SIZE + CELL_SIZE / 2.0f;
                spawn_count++;
            }
        }
    }

    // ── Сброс ботов ───────────────────────────────────────────
    for (int i = 0; i < MAX_BOTS; i++) {
        bots[i].active              = 0;
        bots[i].deathTime           = 0;
        bots[i].b_bullet.active     = 0;
        bots[i].targetCellX         = -1;
        bots[i].targetCellY         = -1;
        bots[i].hp                  = 1;
        bots[i].invincibleTimer     = 0.0f;
        bots[i].flashTimer          = 0;
        bots[i].collisionStuckTimer = 0.0; // инициализация нового поля
        bots[i].detourAttempt       = 0;   // инициализация нового поля
    }

    // ── Инициализация деревьев ────────────────────────────────
    for (int j = 0; j < GRID_SIZE; j++) {
        for (int i = 0; i < GRID_SIZE; i++) {
            if (map[j][i] == 5) {
                woods[j][i].width  = CELL_SIZE;
                woods[j][i].height = CELL_SIZE;
                woods[j][i].uv_u   = 0.0f;
                woods[j][i].uv_v   = 0.0f;
                woods[j][i].x = fieldX + i * CELL_SIZE + CELL_SIZE / 2.0f;
                woods[j][i].y = fieldY + j * CELL_SIZE + CELL_SIZE / 2.0f;
            }
        }
    }

    // ── База ─────────────────────────────────────────────────
    base_init(fieldX, fieldY);

    // ── Очки и таймер ────────────────────────────────────────
    score_reset();
    gLevelStartTime = glfwGetTime();
}

int level_is_complete(void)
{
    if (botQueueIndex == 0) return 0;
    if (botQueueIndex < levels[currentLevelIndex].botCount) return 0;
    for (int i = 0; i < MAX_BOTS; i++)
        if (bots[i].active) return 0;
    return 1;
}
