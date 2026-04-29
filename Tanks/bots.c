#include <math.h>
#include <stdlib.h>
#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <bots.h>
#include <player.h>
#include <collision.h>

Bot bots[MAX_BOTS];
Spawner sp_bots[GRID_SIZE * GRID_SIZE];

void bots_update(float deltaTime, double currentTime,
                 int fieldX, int fieldY, int fieldSize,
                 Spawner* spawnPoints, int spawnCount)
{
    static double nextSpawn  = 0;
    static int    spawnIndex = 0;

    // --- Спавн ---
    int activeCount = 0;
    for (int i = 0; i < MAX_BOTS; i++)
        if (bots[i].active) activeCount++;

    if (activeCount < MAX_BOTS && spawnCount > 0 && currentTime >= nextSpawn) {
        for (int i = 0; i < MAX_BOTS; i++) {
            if (!bots[i].active &&
                (bots[i].deathTime == 0 ||
                 currentTime - bots[i].deathTime >= BOT_SPAWN_INTERVAL))
            {
                bots[i].x = spawnPoints[spawnIndex].x;
                bots[i].y = spawnPoints[spawnIndex].y;

                int dir = rand() % 4;
                float dirs[4][2] = {{1,0},{-1,0},{0,-1},{0,1}};
                bots[i].dirX = dirs[dir][0];
                bots[i].dirY = dirs[dir][1];

                bots[i].active = 1;
                bots[i].b_bullet.active = 0;
                bots[i].b_bullet.dirX = bots[i].dirX;
                bots[i].b_bullet.dirY = bots[i].dirY;
                bots[i].b_bullet.lastShootTime = currentTime;
                bots[i].nextRotateTime = currentTime + BOT_ROTATE_INTERVAL;
                bots[i].deathTime = 0.0;
                bots[i].invincibleTimer = INVINCIBLE_DURATION;

                spawnIndex = (spawnIndex + 1) % spawnCount;
                nextSpawn  = currentTime + BOT_SPAWN_INTERVAL;
                break;
            }
        }
    }

    float halfB  = BOT_SIZE / 2.0f;
    float minXb  = fieldX + halfB, maxXb = fieldX + fieldSize - halfB;
    float minYb  = fieldY + halfB, maxYb = fieldY + fieldSize - halfB;

    for (int i = 0; i < MAX_BOTS; i++) {
        if (!bots[i].active) continue;

        // --- Поворот / навигация к игроку ---
        if (currentTime >= bots[i].nextRotateTime) {
            float dx = player.x - bots[i].x;
            float dy = player.y - bots[i].y;

            float desiredDirX = 0.0f, desiredDirY = 0.0f;
            if (fabsf(dx) > fabsf(dy)) desiredDirX = (dx > 0) ? 1.0f : -1.0f;
            else                        desiredDirY = (dy > 0) ? 1.0f : -1.0f;

            float candidates[4][2] = {
                { desiredDirX,  desiredDirY},
                { desiredDirY,  desiredDirX},
                {-desiredDirY, -desiredDirX},
                {-desiredDirX, -desiredDirY}
            };

            for (int d = 0; d < 4; d++) {
                float tx = bots[i].x + candidates[d][0] * BOT_SPEED * deltaTime;
                float ty = bots[i].y + candidates[d][1] * BOT_SPEED * deltaTime;
                if (tx < minXb || tx > maxXb || ty < minYb || ty > maxYb) continue;
                if (!check_rect_collision_with_map(COLLISION_BOT,
                        tx, ty, BOT_SIZE, BOT_SIZE, (float)i, 0)) {
                    bots[i].dirX = candidates[d][0];
                    bots[i].dirY = candidates[d][1];
                    break;
                }
            }
            bots[i].nextRotateTime = currentTime + BOT_ROTATE_INTERVAL;
        }

        // --- Движение ---
        float newX = fmaxf(minXb, fminf(maxXb, bots[i].x + bots[i].dirX * BOT_SPEED * deltaTime));
        float newY = fmaxf(minYb, fminf(maxYb, bots[i].y + bots[i].dirY * BOT_SPEED * deltaTime));

        if (!check_rect_collision_with_map(COLLISION_BOT,
                newX, newY, BOT_SIZE, BOT_SIZE, (float)i, 0)) {
            bots[i].x = newX;
            bots[i].y = newY;
            if (!bots[i].b_bullet.active) {
                bots[i].b_bullet.dirX = bots[i].dirX;
                bots[i].b_bullet.dirY = bots[i].dirY;
            }
        }

        // --- Неуязвимость ---
        if (bots[i].invincibleTimer > 0.0f) {
            bots[i].invincibleTimer -= deltaTime;
            if (bots[i].invincibleTimer < 0.0f) bots[i].invincibleTimer = 0.0f;
        }

        // --- Стрельба ---
        if (!bots[i].b_bullet.active &&
            currentTime - bots[i].b_bullet.lastShootTime >= BOT_SHOOT_DELAY)
        {
            bots[i].b_bullet.active = 1;
            if (bots[i].b_bullet.dirX != 0) {
                bots[i].b_bullet.x = bots[i].x + bots[i].b_bullet.dirX * BOT_SIZE / 2.0f;
                bots[i].b_bullet.y = bots[i].y;
            } else {
                bots[i].b_bullet.x = bots[i].x;
                bots[i].b_bullet.y = bots[i].y + bots[i].b_bullet.dirY * BOT_SIZE / 2.0f;
            }
            bots[i].b_bullet.lastShootTime = currentTime;
        }

        // --- Обновление пули ---
        bullet_update(&bots[i].b_bullet, COLLISION_BOT_BULLET,
                      deltaTime, fieldX, fieldY, fieldSize);
    }
}

void bots_draw(void (*draw_rect)(float, float, float, float, float*))
{
    float red[4]   = { 1.0f, 0.0f, 0.0f, 1.0f };
    float white[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

    for (int i = 0; i < MAX_BOTS; i++) {
        if (!bots[i].active) continue;

        if (bots[i].invincibleTimer > 0.0f && fmod(glfwGetTime(), 0.2) > 0.1) continue;
        draw_rect(bots[i].x, bots[i].y, BOT_SIZE, BOT_SIZE, red);

        if (bots[i].b_bullet.active) {
            float w = (bots[i].b_bullet.dirX != 0) ? BULLET_WIDTH  : BULLET_HEIGHT;
            float h = (bots[i].b_bullet.dirX != 0) ? BULLET_HEIGHT : BULLET_WIDTH;
            draw_rect(bots[i].b_bullet.x, bots[i].b_bullet.y, w, h, white);
        }
    }
}
