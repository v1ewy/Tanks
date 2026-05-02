#include <math.h>
#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <collision.h>
#include <player.h>
#include <bots.h>
#include <map.h>

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
static int _is_base(int x, int y) {
    if (x < 0 || x >= GRID_SIZE || y < 0 || y >= GRID_SIZE) return 0;
    return map[y][x] == 7;
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

            case COLLISION_PLAYER:
                if (_is_wall(i,j) || _is_water(i,j)) return 1;
                if (gBase.alive) {
                    float dx = fabsf(cx - gBase.x);
                    float dy = fabsf(cy - gBase.y);
                    if (dx < (w + gBase.width)  / 2.0f &&
                        dy < (h + gBase.height) / 2.0f)
                        return 1;
                }
                if (_is_wood(i,j)) {
                    float dx = fabsf(cx - woods[j][i].x);
                    float dy = fabsf(cy - woods[j][i].y);
                    if (dx < (w + woods[j][i].width)  / 2.0f &&
                        dy < (h + woods[j][i].height) / 2.0f) return 1;
                } else {
                    for (int k = 0; k < MAX_BOTS; k++) {
                        if (bots[k].active && bots[k].invincibleTimer <= 0.0f) {
                            float dx = fabsf(cx - bots[k].x);
                            float dy = fabsf(cy - bots[k].y);
                            if (dx < (w + BOT_SIZE) / 2.0f &&
                                dy < (h + BOT_SIZE) / 2.0f) return 1;
                        }
                    }
                }
                break;

            case COLLISION_PLAYER_BULLET:
                if (_is_wall(i,j)) return 1;
                if (_is_wood(i,j)) {
                    float dx = fabsf(cx - woods[j][i].x);
                    float dy = fabsf(cy - woods[j][i].y);
                    if (dx < (w + woods[j][i].width)  / 2.0f &&
                        dy < (h + woods[j][i].height) / 2.0f) {
                        if (bulletDirX != 0) {
                            woods[j][i].width -= 16;
                            woods[j][i].x += bulletDirX * 8.0f;
                            if (bulletDirX > 0)
                                woods[j][i].uv_u += 16.0f / 64.0f;
                        } else {
                            woods[j][i].height -= 16;
                            woods[j][i].y += bulletDirY * 8.0f;
                            if (bulletDirY < 0)
                                woods[j][i].uv_v += 16.0f / 64.0f;
                        }
                        if (woods[j][i].width <= 0 || woods[j][i].height <= 0)
                            map[j][i] = 0;
                        return 1;
                    }
                } else {
                    for (int k = 0; k < MAX_BOTS; k++) {
                        if (bots[k].active) {
                            if (bots[k].b_bullet.active) {
                                float bdx = fabsf(cx - bots[k].b_bullet.x);
                                float bdy = fabsf(cy - bots[k].b_bullet.y);
                                float bw = (bots[k].b_bullet.dirX != 0) ? BULLET_WIDTH : BULLET_HEIGHT;
                                float bh = (bots[k].b_bullet.dirX != 0) ? BULLET_HEIGHT : BULLET_WIDTH;
                                if (bdx < (w + bw) / 2.0f && bdy < (h + bh) / 2.0f) {
                                    bots[k].b_bullet.active = 0;
                                    return 1;
                                }
                            }
                            float dx = fabsf(cx - bots[k].x);
                            float dy = fabsf(cy - bots[k].y);
                            if (dx < (w + BOT_SIZE) / 2.0f && dy < (h + BOT_SIZE) / 2.0f) {
                                if (bots[k].invincibleTimer <= 0.0f) {
                                    bots[k].hp--;
                                    if (bots[k].hp <= 0) {
                                        bots[k].active    = 0;
                                        bots[k].deathTime = glfwGetTime();
                                    }
                                    return 1;
                                }
                            }
                        }
                    }
                    if (gBase.alive) {
                        float dx = fabsf(cx - gBase.x);
                        float dy = fabsf(cy - gBase.y);
                        if (dx < (w + gBase.width)  / 2.0f &&
                            dy < (h + gBase.height) / 2.0f)
                            return 1;
                    }
                }
                break;

            case COLLISION_BOT_BULLET:
                if (_is_wall(i,j)) return 1;
                if (_is_base(i,j)) return 1;
                if (_is_wood(i,j)) {
                    float dx = fabsf(cx - woods[j][i].x);
                    float dy = fabsf(cy - woods[j][i].y);
                    if (dx < (w + woods[j][i].width)  / 2.0f &&
                        dy < (h + woods[j][i].height) / 2.0f) {
                        if (bulletDirX != 0) {
                            woods[j][i].width -= 16;
                            woods[j][i].x += bulletDirX * 8.0f;
                        } else {
                            woods[j][i].height -= 16;
                            woods[j][i].y += bulletDirY * 8.0f;
                        }
                        if (woods[j][i].width <= 0 || woods[j][i].height <= 0)
                            map[j][i] = 0;
                        return 1;
                    }
                }
                else if (player.p_bullet.active) {
                        float pdx = fabsf(cx - player.p_bullet.x);
                        float pdy = fabsf(cy - player.p_bullet.y);
                        float pw = (player.p_bullet.dirX != 0) ? BULLET_WIDTH : BULLET_HEIGHT;
                        float ph = (player.p_bullet.dirX != 0) ? BULLET_HEIGHT : BULLET_WIDTH;
                        if (pdx < (w + pw) / 2.0f && pdy < (h + ph) / 2.0f) {
                            player.p_bullet.active = 0;
                            return 1;
                        }
                } else {
                    float dx = fabsf(cx - player.x);
                    float dy = fabsf(cy - player.y);
                    if (dx < (w + PLAYER_SIZE) / 2.0f &&
                        dy < (h + PLAYER_SIZE) / 2.0f) {
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
                    if (gBase.alive) {
                        float dx = fabsf(cx - gBase.x);
                        float dy = fabsf(cy - gBase.y);
                        if (dx < (w + gBase.width)  / 2.0f &&
                            dy < (h + gBase.height) / 2.0f) {
                            gBase.hp--;
                            if (gBase.hp <= 0) gBase.alive = 0;
                            return 1;
                        }
                    }
                }
                break;

            case COLLISION_BOT:
                if (_is_wall(i,j) || _is_water(i,j)) return 1;
                if (gBase.alive) {
                    float dx = fabsf(cx - gBase.x);
                    float dy = fabsf(cy - gBase.y);
                    if (dx < (w + gBase.width)  / 2.0f &&
                        dy < (h + gBase.height) / 2.0f)
                        return 1;
                }
                if (_is_wood(i,j)) {
                    float dx = fabsf(cx - woods[j][i].x);
                    float dy = fabsf(cy - woods[j][i].y);
                    if (dx < (w + woods[j][i].width)  / 2.0f &&
                        dy < (h + woods[j][i].height) / 2.0f) return 1;
                } else {
                    int self = (int)bulletDirX;
                    for (int k = 0; k < MAX_BOTS; k++) {
                        if (bots[k].active && k != self && bots[k].invincibleTimer <= 0.0f) {
                            float dx = fabsf(cx - bots[k].x);
                            float dy = fabsf(cy - bots[k].y);
                            if (dx < (w + BOT_SIZE) / 2.0f &&
                                dy < (h + BOT_SIZE) / 2.0f) return 1;
                        }
                    }
                    if (player.invincibleTimer <= 0.0f) {
                        float dxP = fabsf(cx - player.x);
                        float dyP = fabsf(cy - player.y);
                        if (dxP < (w + PLAYER_SIZE) / 2.0f &&
                            dyP < (h + PLAYER_SIZE) / 2.0f) return 1;
                    }
                }
                break;
            }
        }
    }
    return 0;
}
