#include <math.h>
#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <player.h>
#include <collision.h>

Player  player;
Spawner sp_player;
int gPlayerMoving = 0;
float gPlayerDirX = 0.0f;
float gPlayerDirY = -1.0f;

void player_update(void* window, float deltaTime, double currentTime,
                   int fieldX, int fieldY, int fieldSize)
{
    GLFWwindow* win = (GLFWwindow*)window;

    if (!player.dead) {
        // --- Движение ---
        float dx = 0.0f, dy = 0.0f;
        if (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS) dx -= 1.0f;
        else if (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS) dx += 1.0f;
        else if (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS) dy -= 1.0f;
        else if (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS) dy += 1.0f;

        if (dx != 0.0f || dy != 0.0f) {
            gPlayerMoving = 1;
            gPlayerDirX = dx;
            gPlayerDirY = dy;
            // Запоминаем направление для пули
            if (!player.p_bullet.active) {
                if (dx != 0.0f) {
                    player.p_bullet.dirX = (dx > 0) ? 1.0f : -1.0f;
                    player.p_bullet.dirY = 0.0f;
                } else {
                    player.p_bullet.dirX = 0.0f;
                    player.p_bullet.dirY = (dy > 0) ? 1.0f : -1.0f;
                }
            }

            float newX = player.x + dx * PLAYER_SPEED * deltaTime;
            if (!check_rect_collision_with_map(COLLISION_PLAYER,
                    newX, player.y, PLAYER_SIZE, PLAYER_SIZE, 0, 0))
                player.x = newX;

            float newY = player.y + dy * PLAYER_SPEED * deltaTime;
            if (!check_rect_collision_with_map(COLLISION_PLAYER,
                    player.x, newY, PLAYER_SIZE, PLAYER_SIZE, 0, 0))
                player.y = newY;
        }

        // --- Ограничение полем ---
        float half = PLAYER_SIZE / 2.0f;
        if (player.x < fieldX + half) player.x = fieldX + half;
        if (player.x > fieldX + fieldSize - half) player.x = fieldX + fieldSize - half;
        if (player.y < fieldY + half) player.y = fieldY + half;
        if (player.y > fieldY + fieldSize - half) player.y = fieldY + fieldSize - half;

        // --- Стрельба ---
        if (glfwGetKey(win, GLFW_KEY_SPACE) == GLFW_PRESS) {
            if (!player.p_bullet.active &&
                currentTime - player.p_bullet.lastShootTime >= PLAYER_SHOOT_DELAY)
            {
                player.p_bullet.active = 1;
                if (player.p_bullet.dirX != 0) {
                    player.p_bullet.x = player.x + player.p_bullet.dirX * PLAYER_SIZE / 2.0f;
                    player.p_bullet.y = player.y;
                } else {
                    player.p_bullet.x = player.x;
                    player.p_bullet.y = player.y + player.p_bullet.dirY * PLAYER_SIZE / 2.0f;
                }
                player.p_bullet.lastShootTime = currentTime;
            }
        }
    }

    // --- Неуязвимость ---
    if (player.invincibleTimer > 0.0f) {
        player.invincibleTimer -= deltaTime;
        if (player.invincibleTimer < 0.0f) player.invincibleTimer = 0.0f;
    }

    // --- Обновление пули ---
    bullet_update(&player.p_bullet, COLLISION_PLAYER_BULLET,
                  deltaTime, fieldX, fieldY, fieldSize);
}

void player_draw(void (*draw_rect)(float, float, float, float, float*))
{
    if (player.dead) return;

    // Мигание при неуязвимости
    if (player.invincibleTimer > 0.0f && fmod(glfwGetTime(), 0.2) > 0.1) return;

    float color[4] = { 0.0f, 1.0f, 0.0f, 1.0f }; // зелёный
    draw_rect(player.x, player.y, PLAYER_SIZE, PLAYER_SIZE, color);

    // Пуля
    if (player.p_bullet.active) {
        float w = (player.p_bullet.dirX != 0) ? BULLET_WIDTH  : BULLET_HEIGHT;
        float h = (player.p_bullet.dirX != 0) ? BULLET_HEIGHT : BULLET_WIDTH;
        float wc[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        draw_rect(player.p_bullet.x, player.p_bullet.y, w, h, wc);
    }
}
