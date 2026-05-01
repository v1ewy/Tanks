#include <render.h>
#include <texture.h>
#include <map.h>
#include <player.h>
#include <bots.h>
#include <math.h>
#include <stdio.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

RenderContext gRender;

void render_init(GLuint shaderProgram, GLuint VAO,
                 GLint modelLoc, GLint colorLoc, GLint projLoc,
                 GLuint texShaderProgram, GLint texModelLoc, GLint texProjLoc,
                 GLuint shaderProgramText, GLuint VAOText, GLuint VBOText,
                 GLint projLocText, GLint textColorLoc, GLuint fontTexture)
{
    gRender.shaderProgram    = shaderProgram;
    gRender.VAO              = VAO;
    gRender.modelLoc         = modelLoc;
    gRender.colorLoc         = colorLoc;
    gRender.projLoc          = projLoc;
    gRender.texShaderProgram = texShaderProgram;
    gRender.texModelLoc      = texModelLoc;
    gRender.texProjLoc       = texProjLoc;
    gRender.shaderProgramText = shaderProgramText;
    gRender.VAOText          = VAOText;
    gRender.VBOText          = VBOText;
    gRender.projLocText      = projLocText;
    gRender.textColorLoc     = textColorLoc;
    gRender.fontTexture      = fontTexture;
}

float w[4] = {1.0f, 1.0f, 1.0f, 0.75f};
// ── Цветной прямоугольник ──────────────────────
void draw_rect(float x, float y, float w, float h, float* color)
{
    float model[16] = {0};
    model[0]=w; model[5]=h; model[10]=1.0f; model[15]=1.0f;
    model[12]=x; model[13]=y;

    glUseProgram(gRender.shaderProgram);
    glUniformMatrix4fv(gRender.modelLoc, 1, GL_FALSE, model);
    glUniform4fv(gRender.colorLoc, 1, color);
    glBindVertexArray(gRender.VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

// ── Текстурированный прямоугольник ────────────
void draw_textured_rect(float x, float y, float w, float h, GLuint textureID)
{
    float model[16] = {0};
    model[0]=w; model[5]=h; model[10]=1.0f; model[15]=1.0f;
    model[12]=x; model[13]=y;

    glUseProgram(gRender.texShaderProgram);
    glUniformMatrix4fv(gRender.texModelLoc, 1, GL_FALSE, model);

    extern float projectionMatrix[16];
    glUniformMatrix4fv(gRender.texProjLoc, 1, GL_FALSE, projectionMatrix);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glUniform1i(glGetUniformLocation(gRender.texShaderProgram, "uTexture"), 0);

    glBindVertexArray(gRender.VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

float gPlayerAnimTimer = 0.0f;
int   gPlayerAnimFrame = 0;

void render_rotated_uv(float x, float y, float w, float h,
                       GLuint textureID,
                       float u0, float v0, float u1, float v1,
                       float angle)
{
    float cosA = cosf(angle);
    float sinA = sinf(angle);
    float hw = w / 2.0f, hh = h / 2.0f;

    // Локальные углы прямоугольника
    float corners[4][2] = {
        {-hw, -hh},
        { hw, -hh},
        { hw,  hh},
        {-hw,  hh}
    };

    float uvs[4][2] = {
        {u0, v1},
        {u1, v1},
        {u1, v0},
        {u0, v0}
    };

    float verts[16]; // 4 вершины * (x,y,u,v)
    for (int k = 0; k < 4; k++) {
        float rx = corners[k][0] * cosA - corners[k][1] * sinA;
        float ry = corners[k][0] * sinA + corners[k][1] * cosA;
        verts[k*4+0] = x + rx;
        verts[k*4+1] = y + ry;
        verts[k*4+2] = uvs[k][0];
        verts[k*4+3] = uvs[k][1];
    }

    unsigned int idx[] = { 0,1,2, 0,2,3 };

    GLuint vao, vbo, ebo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
                          4*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
                          4*sizeof(float), (void*)(2*sizeof(float)));
    glEnableVertexAttribArray(1);

    glUseProgram(gRender.texShaderProgram);
    extern float projectionMatrix[16];
    float model[16] = {0};
    model[0]=1.0f; model[5]=1.0f; model[10]=1.0f; model[15]=1.0f;
    glUniformMatrix4fv(gRender.texModelLoc, 1, GL_FALSE, model);
    glUniformMatrix4fv(gRender.texProjLoc,  1, GL_FALSE, projectionMatrix);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glUniform1i(glGetUniformLocation(gRender.texShaderProgram, "uTexture"), 0);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    glBindVertexArray(0);
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
}

// ── Карта ─────────────────────────────────────
void render_map(void) {
    float black[4] = {0,0,0,1};
    draw_rect(fieldX + FIELD_SIZE/2.0f, fieldY + FIELD_SIZE/2.0f,
              FIELD_SIZE, FIELD_SIZE, black);

    for (int j = 0; j < GRID_SIZE; j++) {
        for (int i = 0; i < GRID_SIZE; i++) {
            float cx = fieldX + i * CELL_SIZE + CELL_SIZE / 2.0f;
            float cy = fieldY + j * CELL_SIZE + CELL_SIZE / 2.0f;

            switch (map[j][i]) {
                case 2: // стена
                    draw_textured_rect(cx, cy, BLOCK_SIZE, BLOCK_SIZE,
                                       gTextures.wall);
                    break;
                case 3: // вода
                    draw_textured_rect(cx, cy, BLOCK_SIZE, BLOCK_SIZE,
                                       gTextures.water);
                    break;
                case 5:
                    if (woods[j][i].width > 0 && woods[j][i].height > 0) {
                        Wood* w = &woods[j][i];
                        float invTexSize = 1.0f / 64.0f;
                        float u0 = w->uv_u;
                        float v0 = w->uv_v;
                        float u1 = u0 + w->width  * invTexSize;
                        float v1 = v0 + w->height * invTexSize;

                        render_rotated_uv(w->x, w->y,
                                          w->width, w->height,
                                          gTextures.wood,
                                          u0, v0, u1, v1, 0);
                    }
                    break;
                case 7:
                    draw_rect(cx, cy, BLOCK_SIZE, BLOCK_SIZE, w);
                    break;
            }
        }
    }
}

// render.c — отдельная функция для пули с поворотом
void render_bullet(float x, float y, float w, float h,
                   float dirX, float dirY) {
    float angle = 0.0f;
    if      (dirX >  0) angle =  3.14159f;               // вправо — зеркало влево = 0°
    else if (dirX <  0) angle =  0.0f;            // влево  — исходная текстура
    else if (dirY <  0) angle =  3.14159f / 2.0f;     // вверх
    else if (dirY >  0) angle = -3.14159f / 2.0f;     // вниз

    // Всегда используем BULLET_WIDTH x BULLET_HEIGHT — поворот сделает остальное
    render_rotated_uv(x, y,
                      BULLET_WIDTH, BULLET_HEIGHT,
                      gTextures.bullet,
                      0.0f, 0.0f, 1.0f, 1.0f,
                      angle);
}

// ── Листва (поверх всего) ─────────────────────
void render_foliage(void) {
    for (int j = 0; j < GRID_SIZE; j++) {
        for (int i = 0; i < GRID_SIZE; i++) {
            if (map[j][i] == 4) {
                float cx = fieldX + i * CELL_SIZE + CELL_SIZE / 2.0f;
                float cy = fieldY + j * CELL_SIZE + CELL_SIZE / 2.0f;
                // Листва пока цветом (текстуры нет)
                float dg[4] = {0.0f, 0.55f, 0.0f, 0.75f};
                draw_rect(cx, cy, BLOCK_SIZE, BLOCK_SIZE, dg);
            }
        }
    }
}

// ── Игрок ─────────────────────────────────────
void render_player(void)
{
    if (player.dead) return;

    static double lastAnimTime = 0.0;
    double now = glfwGetTime();

    if (gPlayerMoving) {
        if (now - lastAnimTime >= 0.15) {
            gPlayerAnimFrame = (gPlayerAnimFrame + 1) % 4;
            lastAnimTime = now;
        }
    } else {
        gPlayerAnimFrame = 0;
    }

    if (player.invincibleTimer > 0.0f && fmod(now, 0.2) > 0.1)
        goto draw_player_bullet;

    {
        float u0 = (float) gPlayerAnimFrame      / 4.0f;
        float u1 = (float)(gPlayerAnimFrame + 1) / 4.0f;

        // Угол поворота по направлению движения
        // Спрайт по умолчанию смотрит ВПРАВО
        float angle = 0.0f;
        if      (gPlayerDirX >  0) angle = 3.14159f / 2.0f;               // вправо
        else if (gPlayerDirX <  0) angle =  -3.14159f / 2.0f;            // влево
        else if (gPlayerDirY <  0) angle = 0.0f;     // вверх
        else if (gPlayerDirY >  0) angle =  3.14159f;     // вниз

        render_rotated_uv(player.x, player.y,
                          PLAYER_SIZE, PLAYER_SIZE,
                          gTextures.player,
                          u0, 0.0f, u1, 1.0f,
                          angle);
    }

draw_player_bullet:
    if (player.p_bullet.active) {
        float w = (player.p_bullet.dirX != 0) ? BULLET_WIDTH  : BULLET_HEIGHT;
        float h = (player.p_bullet.dirX != 0) ? BULLET_HEIGHT : BULLET_WIDTH;
        render_bullet(player.p_bullet.x, player.p_bullet.y,
                      w, h,
                      player.p_bullet.dirX, player.p_bullet.dirY);
    }
}

// ── Боты ──────────────────────────────────────
// Цвета по типу бота
static float* bot_color(BotType type)
{
    static float red[]    = {1.0f, 0.0f, 0.0f, 1.0f}; // обычный
    static float yellow[] = {1.0f, 1.0f, 0.0f, 1.0f}; // гончая
    static float orange[] = {1.0f, 0.5f, 0.0f, 1.0f}; // охотник
    static float silver[] = {0.7f, 0.7f, 0.7f, 1.0f}; // бронированный
    switch (type) {
    case BOT_HOUND:   return yellow;
    case BOT_HUNTER:  return orange;
    case BOT_ARMORED: return silver;
    default:          return red;
    }
}

void render_base(void) {
    if (!gBase.alive) return;

    // Цвет меняется по HP: зелёный → жёлтый → красный
    float r = (gBase.hp == 3) ? 0.0f : (gBase.hp == 2) ? 1.0f : 1.0f;
    float g = (gBase.hp == 3) ? 1.0f : (gBase.hp == 2) ? 1.0f : 0.0f;
    float b = 0.0f, a = 1.0f;
    float color[4] = {r, g, b, a};

    draw_rect(gBase.x, gBase.y, gBase.width, gBase.height, color);

    extern void render_text(const char*, float, float, float, float, float, float);
    char txt[8];
    sprintf(txt, "HP:%d", gBase.hp);
    render_text(txt, gBase.x - 25, gBase.y + 8, 0.6f, 1.0f, 1.0f, 1.0f);
}

void render_bots(void) {
    for (int i = 0; i < MAX_BOTS; i++) {
        if (!bots[i].active) continue;

        int blink = (bots[i].invincibleTimer > 0.0f && fmod(glfwGetTime(), 0.2) > 0.1)
                 || (bots[i].flashTimer > 0 && bots[i].flashTimer % 2 == 0);
        if (!blink) {
            float* color = bot_color(bots[i].type);

            if (bots[i].type == BOT_ARMORED) {
                float dark[4] = {0.3f, 0.3f, 0.3f, 1.0f};
                draw_rect(bots[i].x, bots[i].y - BOT_SIZE/2.0f - 6,
                          BOT_SIZE, 5, dark);
                float hpColor[4] = {0.0f, 1.0f, 0.0f, 1.0f};
                float hpW = BOT_SIZE * (bots[i].hp / 3.0f);
                draw_rect(bots[i].x - (BOT_SIZE - hpW) / 2.0f,
                          bots[i].y - BOT_SIZE/2.0f - 6,
                          hpW, 5, hpColor);
            }

            color[3] = 0.4f;
            draw_rect(bots[i].x, bots[i].y, BOT_SIZE, BOT_SIZE, color);
            color[3] = 1.0f;
        }

        if (bots[i].b_bullet.active) {
            render_bullet(bots[i].b_bullet.x, bots[i].b_bullet.y,
                          BULLET_WIDTH, BULLET_HEIGHT,
                          bots[i].b_bullet.dirX, bots[i].b_bullet.dirY);
        }
    }
}

// ── HUD ───────────────────────────────────────
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
