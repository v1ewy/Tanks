#include <string.h>
#include <map.h>
#include <levels.h>
#include <player.h>
#include <bots.h>

extern int fieldX, fieldY;

Level levels[TOTAL_LEVELS];

void levels_init(void)
{
    // ── Уровень 1: Открытое поле ──────────────────────────────
    // Спавны игрока=1, ботов=6, стена=2, вода=3, листва=4, дерево=5
    int l1[GRID_SIZE][GRID_SIZE] = {
        {0,0,6,0,0,0,0,0,0,0,6,0,0},
        {0,0,0,2,0,0,0,0,0,2,0,0,0},
        {0,0,0,2,0,0,0,0,0,2,0,0,0},
        {0,0,0,2,0,0,3,0,0,2,0,0,0},
        {0,0,0,2,0,0,3,3,0,2,0,0,0},
        {0,0,0,2,0,0,0,3,0,2,0,0,0},
        {0,0,0,2,0,3,3,3,0,2,0,0,0},
        {0,0,0,2,0,0,0,0,0,2,0,0,0},
        {5,5,5,2,0,0,0,0,0,2,5,5,5},
        {0,0,0,0,0,0,0,0,0,0,0,0,0},
        {4,5,4,5,4,5,4,5,4,5,4,5,4},
        {0,3,3,3,0,0,0,0,0,3,3,3,0},
        {0,0,0,0,0,0,1,7,0,0,0,0,0},
    };
    memcpy(levels[0].map, l1, sizeof(l1));
    levels[0].player_lives = 3;
    levels[0].name = "OPEN FIELD";

    // ── Уровень 2: Городской бой ──────────────────────────────
    // Много стен, узкие коридоры, больше ботов-спавнов
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
        {6,0,0,0,0,0,0,0,0,0,0,0,6},
        {0,0,0,0,0,0,1,0,0,0,0,0,0},
    };
    memcpy(levels[1].map, l2, sizeof(l2));
    levels[1].player_lives = 3;
    levels[1].name = "URBAN COMBAT";

    // ── Уровень 3: Крепость ───────────────────────────────────
    // Закрытая карта, боты спавнятся по углам, много препятствий
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
    levels[2].player_lives = 5; // больше жизней — карта сложнее
    levels[2].name = "FORTRESS";
}

void load_level(int index)
{
    memcpy(map, levels[index].map, sizeof(int) * GRID_SIZE * GRID_SIZE);

    // ── Спавн игрока ──
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

    player.x              = sp_player.x;
    player.y              = sp_player.y;
    player.dead           = 0;
    player.lives          = levels[index].player_lives;
    player.invincibleTimer = 0.0f;
    player.p_bullet.active = 0;
    player.p_bullet.dirX  = 0.0f;
    player.p_bullet.dirY  = -1.0f;
    player.p_bullet.lastShootTime = 0.0;

    // ── Спавны ботов ──
    int spawn_count = 0;
    for (int j = 0; j < GRID_SIZE; j++) {
        for (int i = 0; i < GRID_SIZE; i++) {
            if (map[j][i] == 6) {
                sp_bots[spawn_count].x = fieldX + i * CELL_SIZE + CELL_SIZE / 2.0f;
                sp_bots[spawn_count].y = fieldY + j * CELL_SIZE + CELL_SIZE / 2.0f;
                spawn_count++;
            }
        }
    }

    // ── Сброс ботов ──
    for (int i = 0; i < MAX_BOTS; i++) {
        bots[i].active         = 0;
        bots[i].deathTime      = 0;
        bots[i].b_bullet.active = 0;
    }

    // ── Инициализация деревьев ──
    for (int j = 0; j < GRID_SIZE; j++) {
        for (int i = 0; i < GRID_SIZE; i++) {
            if (map[j][i] == 5) {
                woods[j][i].width      = CELL_SIZE;
                woods[j][i].height     = CELL_SIZE;
                woods[j][i].uv_u = 0.0f;
                woods[j][i].uv_v = 0.0f;
                woods[j][i].x = fieldX + i * CELL_SIZE + CELL_SIZE / 2.0f;
                woods[j][i].y = fieldY + j * CELL_SIZE + CELL_SIZE / 2.0f;
            }
        }
    }
    base_init(fieldX, fieldY);
}
