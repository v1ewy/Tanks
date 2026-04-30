#include <stdio.h>
#include <math.h>
#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <render.h>
#include <map.h>
#include <player.h>
#include <bots.h>
#include <bullet.h>

RenderContext gRender;

// Цвета
static float COL_BLACK[]      = {0.00f, 0.00f, 0.00f, 1.0f};
static float COL_GRAY[]       = {0.50f, 0.50f, 0.50f, 1.0f};
static float COL_WHITE[]      = {1.00f, 1.00f, 1.00f, 1.0f};
static float COL_BLUE[]       = {0.00f, 0.20f, 1.00f, 1.0f};
static float COL_ORANGE[]     = {0.80f, 0.40f, 0.00f, 1.0f};
static float COL_GREEN[]      = {0.00f, 1.00f, 0.00f, 1.0f};
static float COL_DARK_GREEN[] = {0.00f, 0.70f, 0.00f, 0.8f};
static float COL_RED[]        = {1.00f, 0.00f, 0.00f, 1.0f};
static float COL_YELLOW[]     = {1.00f, 1.00f, 0.00f, 1.0f};

void render_init(GLuint shaderProgram, GLuint VAO,
                 GLint modelLoc, GLint colorLoc, GLint projLoc,
                 GLuint shaderProgramText, GLuint VAOText, GLuint VBOText,
                 GLint projLocText, GLint textColorLoc, GLuint fontTexture)
{
    gRender.shaderProgram     = shaderProgram;
    gRender.VAO               = VAO;
    gRender.modelLoc          = modelLoc;
    gRender.colorLoc          = colorLoc;
    gRender.projLoc           = projLoc;
    gRender.shaderProgramText = shaderProgramText;
    gRender.VAOText           = VAOText;
    gRender.VBOText           = VBOText;
    gRender.projLocText       = projLocText;
    gRender.textColorLoc      = textColorLoc;
    gRender.fontTexture       = fontTexture;
}

// ─────────────────────────────────────────────
//  Базовые примитивы
// ─────────────────────────────────────────────

void draw_rect(float x, float y, float w, float h, float* color)
{
    float model[16] = {0};
    model[0]  = w;    model[5]  = h;
    model[10] = 1.0f; model[15] = 1.0f;
    model[12] = x;    model[13] = y;

    glUseProgram(gRender.shaderProgram);
    glUniformMatrix4fv(gRender.modelLoc, 1, GL_FALSE, model);
    glUniform4fv(gRender.colorLoc, 1, color);
    glBindVertexArray(gRender.VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}
// ─────────────────────────────────────────────
//  Карта
// ─────────────────────────────────────────────

void render_map(void)
{
    // Подложка поля
    draw_rect(fieldX + FIELD_SIZE / 2.0f,
              fieldY + FIELD_SIZE / 2.0f,
              FIELD_SIZE, FIELD_SIZE, COL_BLACK);

    for (int j = 0; j < GRID_SIZE; j++) {
        for (int i = 0; i < GRID_SIZE; i++) {
            float cx = fieldX + i * CELL_SIZE + CELL_SIZE / 2.0f;
            float cy = fieldY + j * CELL_SIZE + CELL_SIZE / 2.0f;

            switch (map[j][i]) {
            case 2: // стена
                draw_rect(cx, cy, BLOCK_SIZE, BLOCK_SIZE,
                                     COL_GRAY);
                break;
            case 3: // вода
                draw_rect(cx, cy, BLOCK_SIZE, BLOCK_SIZE,
                                     COL_BLUE);
                break;
            case 5: // дерево (разрушаемое)
                if (woods[j][i].width > 0 && woods[j][i].height > 0) {
                    draw_rect(woods[j][i].x, woods[j][i].y,
                                        woods[j][i].width, woods[j][i].height,
                                        COL_ORANGE);
                }
                break;
            default:
                break;
            }
        }
    }
}

// Листва рисуется ПОВЕРХ всего (после игрока и ботов)
void render_foliage(void)
{
    for (int j = 0; j < GRID_SIZE; j++) {
        for (int i = 0; i < GRID_SIZE; i++) {
            if (map[j][i] == 4) {
                float cx = fieldX + i * CELL_SIZE + CELL_SIZE / 2.0f;
                float cy = fieldY + j * CELL_SIZE + CELL_SIZE / 2.0f;
                draw_rect(cx, cy, BLOCK_SIZE, BLOCK_SIZE,
                                     COL_DARK_GREEN);
            }
        }
    }
}

// ─────────────────────────────────────────────
//  Игрок
// ─────────────────────────────────────────────

void render_player(void)
{
    if (player.dead) return;

    // Мигание при неуязвимости
    if (player.invincibleTimer > 0.0f && fmod(glfwGetTime(), 0.2) > 0.1) goto draw_bullet;

    draw_rect(player.x, player.y,
                         PLAYER_SIZE, PLAYER_SIZE,
                         COL_GREEN);

draw_bullet:
    if (player.p_bullet.active) {
        float w = (player.p_bullet.dirX != 0) ? BULLET_WIDTH  : BULLET_HEIGHT;
        float h = (player.p_bullet.dirX != 0) ? BULLET_HEIGHT : BULLET_WIDTH;
        draw_rect(player.p_bullet.x, player.p_bullet.y,
                             w, h, COL_YELLOW);
    }
}

// ─────────────────────────────────────────────
//  Боты
// ─────────────────────────────────────────────

void render_bots(void)
{
    for (int i = 0; i < MAX_BOTS; i++) {
        if (!bots[i].active) continue;

        if (!(bots[i].invincibleTimer > 0.0f && fmod(glfwGetTime(), 0.2) > 0.1)) {
            draw_rect(bots[i].x, bots[i].y,
                                 BOT_SIZE, BOT_SIZE,
                                 COL_RED);
        }

        if (bots[i].b_bullet.active) {
            float w = (bots[i].b_bullet.dirX != 0) ? BULLET_WIDTH  : BULLET_HEIGHT;
            float h = (bots[i].b_bullet.dirX != 0) ? BULLET_HEIGHT : BULLET_WIDTH;
            draw_rect(bots[i].b_bullet.x, bots[i].b_bullet.y,
                                 w, h, COL_YELLOW);
        }
    }
}

// ─────────────────────────────────────────────
//  HUD
// ─────────────────────────────────────────────

void render_hud(void)
{
    extern void render_text(const char*, float, float, float, float, float, float);
    extern int windowWidth, windowHeight;

    char text[64];

    sprintf(text, "LIVES: %d", player.lives);
    render_text(text, 20, 40, 0.9f, 1.0f, 1.0f, 1.0f);

    int enemies = 0;
    for (int i = 0; i < MAX_BOTS; i++) if (bots[i].active) enemies++;
    sprintf(text, "ENEMIES: %d", enemies);
    render_text(text, 20, 90, 0.9f, 1.0f, 1.0f, 1.0f);
}
