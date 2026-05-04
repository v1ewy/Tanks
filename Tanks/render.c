#include <render.h>
#include <texture.h>
#include <map.h>
#include <player.h>
#include <bots.h>
#include <math.h>
#include <stdio.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <levels.h>

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
    model[0]=h; model[5]=w; model[10]=1.0f; model[15]=1.0f;
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
                    render_rotated_uv(cx, cy, BLOCK_SIZE, BLOCK_SIZE,
                                       gTextures.wall, 1.0f, 1.0f, 0.0f, 0.0f, 3.14159f);
                    break;
                case 3: // вода
                    render_rotated_uv(cx, cy, BLOCK_SIZE, BLOCK_SIZE,
                                       gTextures.water, 1.0f, 1.0f, 0.0f, 0.0f, 3.14159f);
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
            }
        }
    }
}

void render_base(void) {
    if (!gBase.alive) return;

    float r = (gBase.hp == 3) ? 0.0f : (gBase.hp == 2) ? 1.0f : 1.0f;
    float g = (gBase.hp == 3) ? 1.0f : (gBase.hp == 2) ? 1.0f : 0.0f;
    float b = 0.0f, a = 1.0f;
    float color[4] = {r, g, b, a};

    render_rotated_uv(gBase.x, gBase.y, BLOCK_SIZE, BLOCK_SIZE,
                      gTextures.base, 1.0f, 1.0f, 0.0f, 0.0f, 3.14159f);
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
                render_rotated_uv(cx, cy, BLOCK_SIZE, BLOCK_SIZE,
                                   gTextures.foliage, 1.0f, 1.0f, 0.0f, 0.0f, 3.14159f);
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

    // Мигание при неуязвимости — пропускаем отрисовку
    if (player.invincibleTimer > 0.0f && fmod(now, 0.2) > 0.1)
        goto draw_bullet; // ← пропускаем тело, но рисуем пулю

    {
        float u0 = (float) gPlayerAnimFrame      / 4.0f;
        float u1 = (float)(gPlayerAnimFrame + 1) / 4.0f;

        float angle = 0.0f;
        if      (gPlayerDirX >  0) angle =  3.14159f / 2.0f;
        else if (gPlayerDirX <  0) angle = -3.14159f / 2.0f;
        else if (gPlayerDirY <  0) angle =  0.0f;
        else if (gPlayerDirY >  0) angle =  3.14159f;

        render_rotated_uv(player.x, player.y,
                          PLAYER_SIZE, PLAYER_SIZE,
                          gTextures.player,
                          u0, 0.0f, u1, 1.0f,
                          angle);
    }

draw_bullet:
    if (player.p_bullet.active) {
        float w = (player.p_bullet.dirX != 0) ? BULLET_WIDTH  : BULLET_HEIGHT;
        float h = (player.p_bullet.dirX != 0) ? BULLET_HEIGHT : BULLET_WIDTH;
        render_bullet(player.p_bullet.x, player.p_bullet.y,
                      w, h,
                      player.p_bullet.dirX, player.p_bullet.dirY);
    }
}

// ── Боты ──────────────────────────────────────
static GLuint bot_texture(BotType type)
{
    switch (type) {
    case BOT_HOUND:   return gTextures.bot_hound;
    case BOT_HUNTER:  return gTextures.bot_hunter;
    case BOT_ARMORED: return gTextures.bot_armored;
    default:          return gTextures.bot_normal;
    }
}

void render_bots(void)
{
    double now = glfwGetTime();

    for (int i = 0; i < MAX_BOTS; i++) {
        if (!bots[i].active) continue;
        int blink = (bots[i].invincibleTimer > 0.0f && fmod(now, 0.2) > 0.1)
                 || (bots[i].flashTimer > 0 && bots[i].flashTimer % 2 == 0);

        if (!blink) {
            int moving = (bots[i].dirX != 0.0f || bots[i].dirY != 0.0f);
            if (moving && now - bots[i].lastAnimTime >= 0.15) {
                bots[i].animFrame    = (bots[i].animFrame + 1) % 4;
                bots[i].lastAnimTime = now;
            }
            if (!moving) bots[i].animFrame = 0;

            float u0 = (float) bots[i].animFrame      / 4.0f;
            float u1 = (float)(bots[i].animFrame + 1) / 4.0f;

            // Угол поворота по направлению движения
            float angle = 0.0f;
            if      (bots[i].faceX >  0) angle =  3.14159f / 2.0f;
            else if (bots[i].faceX <  0) angle = -3.14159f / 2.0f;
            else if (bots[i].faceY >  0) angle =  3.14159f;
            else                          angle =  0.0f;

            render_rotated_uv(bots[i].x, bots[i].y,
                              BOT_SIZE, BOT_SIZE,
                              bot_texture(bots[i].type),
                              u0, 0.0f, u1, 1.0f,
                              angle);
        }

        // Пуля бота
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
    Level* lvl = &levels[currentLevelIndex];

    extern void render_text(const char*, float, float, float, float, float, float);
    extern int windowWidth, windowHeight;

    char text[64];
    sprintf(text, "LIVES: %d", player.lives);
    render_text(text, 20, 40, 0.9f, 1.0f, 1.0f, 1.0f);

    sprintf(text, "ENEMIES: %d", lvl->botCurrent);
    render_text(text, 20, 90, 0.9f, 1.0f, 1.0f, 1.0f);
    
    sprintf(text, "HP BASE:%d", gBase.hp);
    render_text(text, 20, 140, 0.9f, 1.0f, 1.0f, 1.0f);
}
