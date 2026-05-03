#include <math.h>
#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <collision.h>
#include <player.h>
#include <bots.h>
#include <map.h>
#include <score.h>

static int _is_wall(int x, int y) {
    if (x < 0 || x >= GRID_SIZE || y < 0 || y >= GRID_SIZE) return 1;
    return map[y][x] == 2;
}
static int _is_water(int x, int y) {
    if (x < 0 || x >= GRID_SIZE || y < 0 || y >= GRID_SIZE) return 0;
    return map[y][x] == 3;
}
static int _is_wood(int x, int y) {
    if (x < 0 || x >= GRID_SIZE || y < 0 || y >= GRID_SIZE) return 0;
    return map[y][x] == 5;
}

// Проверка перекрытия двух AABB
static int aabb_overlap(float ax, float ay, float aw, float ah,
                        float bx, float by, float bw, float bh)
{
    return fabsf(ax - bx) < (aw + bw) / 2.0f &&
           fabsf(ay - by) < (ah + bh) / 2.0f;
}

int check_rect_collision_with_map(int who, float cx, float cy,
                                  float w, float h,
                                  float bulletDirX, float bulletDirY)
{
    float halfW = w / 2.0f, halfH = h / 2.0f;
    int left   = (int)((cx - halfW - fieldX) / CELL_SIZE);
    int right  = (int)((cx + halfW - fieldX) / CELL_SIZE);
    int top    = (int)((cy - halfH - fieldY) / CELL_SIZE);
    int bottom = (int)((cy + halfH - fieldY) / CELL_SIZE);

    for (int j = top; j <= bottom; j++) {
        for (int i = left; i <= right; i++) {
            switch (who) {

            // ── Игрок ────────────────────────────────────────────────
            case COLLISION_PLAYER:
                if (_is_wall(i,j) || _is_water(i,j)) return 1;
                if (_is_wood(i,j)) {
                    if (aabb_overlap(cx, cy, w, h,
                                     woods[j][i].x, woods[j][i].y,
                                     woods[j][i].width, woods[j][i].height))
                        return 1;
                }
                // База блокирует игрока
                if (gBase.alive &&
                    aabb_overlap(cx, cy, w, h,
                                 gBase.x, gBase.y,
                                 gBase.width, gBase.height))
                    return 1;
                // Боты блокируют игрока
                for (int k = 0; k < MAX_BOTS; k++) {
                    if (bots[k].active &&
                        aabb_overlap(cx, cy, w, h,
                                     bots[k].x, bots[k].y,
                                     BOT_SIZE, BOT_SIZE))
                        return 1;
                }
                break;

            // ── Пуля игрока ───────────────────────────────────────────
            case COLLISION_PLAYER_BULLET:
                if (_is_wall(i,j)) return 1;
                if (_is_wood(i,j)) {
                    if (aabb_overlap(cx, cy, w, h,
                                     woods[j][i].x, woods[j][i].y,
                                     woods[j][i].width, woods[j][i].height)) {
                        if (bulletDirX != 0) {
                            woods[j][i].width -= 16;
                            woods[j][i].x     += bulletDirX * 8.0f;
                            if (bulletDirX > 0)
                                woods[j][i].uv_u += 16.0f / 64.0f;
                        } else {
                            woods[j][i].height -= 16;
                            woods[j][i].y      += bulletDirY * 8.0f;
                            if (bulletDirY < 0)
                                woods[j][i].uv_v += 16.0f / 64.0f;
                        }
                        if (woods[j][i].width <= 0 || woods[j][i].height <= 0)
                            map[j][i] = 0;
                        return 1;
                    }
                    break;
                }
                // Пуля игрока уничтожает пулю бота
                for (int k = 0; k < MAX_BOTS; k++) {
                    if (!bots[k].active) continue;
                    if (bots[k].b_bullet.active) {
                        float bw = (bots[k].b_bullet.dirX != 0)
                                   ? BULLET_WIDTH : BULLET_HEIGHT;
                        float bh = (bots[k].b_bullet.dirX != 0)
                                   ? BULLET_HEIGHT : BULLET_WIDTH;
                        if (aabb_overlap(cx, cy, w, h,
                                         bots[k].b_bullet.x,
                                         bots[k].b_bullet.y, bw, bh)) {
                            bots[k].b_bullet.active = 0;
                            return 1;
                        }
                    }
                    // Попадание в бота
                    if (aabb_overlap(cx, cy, w, h,
                                     bots[k].x, bots[k].y,
                                     BOT_SIZE, BOT_SIZE)) {
                        if (bots[k].invincibleTimer <= 0.0f) {
                            bots[k].hp--;
                            bots[k].flashTimer = 10;
                            if (bots[k].hp <= 0) {
                                score_add_kill(bots[k].type);
                                bots[k].active    = 0;
                                bots[k].deathTime = glfwGetTime();
                            } else {
                                bots[k].invincibleTimer = 0.5f;
                            }
                            return 1;
                        }
                    }
                }
                // Пуля игрока не бьёт по своей базе
                break;

            // ── Пуля бота ────────────────────────────────────────────
            case COLLISION_BOT_BULLET:
                if (_is_wall(i,j)) return 1;
                if (_is_wood(i,j)) {
                    if (aabb_overlap(cx, cy, w, h,
                                     woods[j][i].x, woods[j][i].y,
                                     woods[j][i].width, woods[j][i].height)) {
                        if (bulletDirX != 0) {
                            woods[j][i].width -= 16;
                            woods[j][i].x     += bulletDirX * 8.0f;
                        } else {
                            woods[j][i].height -= 16;
                            woods[j][i].y      += bulletDirY * 8.0f;
                        }
                        if (woods[j][i].width <= 0 || woods[j][i].height <= 0)
                            map[j][i] = 0;
                        return 1;
                    }
                    break;
                }
                // Пуля бота уничтожает пулю игрока
                if (player.p_bullet.active) {
                    float pw = (player.p_bullet.dirX != 0)
                               ? BULLET_WIDTH : BULLET_HEIGHT;
                    float ph = (player.p_bullet.dirX != 0)
                               ? BULLET_HEIGHT : BULLET_WIDTH;
                    if (aabb_overlap(cx, cy, w, h,
                                     player.p_bullet.x,
                                     player.p_bullet.y, pw, ph)) {
                        player.p_bullet.active = 0;
                        return 1;
                    }
                }
                // Попадание в игрока
                if (aabb_overlap(cx, cy, w, h,
                                 player.x, player.y,
                                 PLAYER_SIZE, PLAYER_SIZE)) {
                    if (player.invincibleTimer <= 0.0f) {
                        player.lives--;
                        if (player.lives <= 0) {
                            player.dead = 1;
                        } else {
                            player.x = sp_player.x;
                            player.y = sp_player.y;
                            player.invincibleTimer = INVINCIBLE_DURATION;
                        }
                    }
                    return 1;
                }
                // Попадание в базу
                if (gBase.alive &&
                    aabb_overlap(cx, cy, w, h,
                                 gBase.x, gBase.y,
                                 gBase.width, gBase.height)) {
                    gBase.hp--;
                    if (gBase.hp <= 0) gBase.alive = 0;
                    return 1;
                }
                break;

            // ── Бот (движение) ────────────────────────────────────────
            case COLLISION_BOT: {
                if (_is_wall(i,j) || _is_water(i,j)) return 1;
                if (_is_wood(i,j)) {
                    if (aabb_overlap(cx, cy, w, h,
                                     woods[j][i].x, woods[j][i].y,
                                     woods[j][i].width, woods[j][i].height))
                        return 1;
                }
                // База блокирует ботов
                if (gBase.alive &&
                    aabb_overlap(cx, cy, w, h,
                                 gBase.x, gBase.y,
                                 gBase.width, gBase.height))
                    return 1;
                // Бот-бот: не входим в других ботов
                int self = (int)bulletDirX;
                for (int k = 0; k < MAX_BOTS; k++) {
                    if (!bots[k].active || k == self) continue;
                    if (aabb_overlap(cx, cy, w, h,
                                     bots[k].x, bots[k].y,
                                     BOT_SIZE, BOT_SIZE))
                        return 1;
                }
                // Бот не входит в игрока
                if (aabb_overlap(cx, cy, w, h,
                                 player.x, player.y,
                                 PLAYER_SIZE, PLAYER_SIZE))
                    return 1;
                break;
            }
            }
        }
    }
    return 0;
}
