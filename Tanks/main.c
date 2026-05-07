#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include <bullet.h>
#include <player.h>
#include <bots.h>
#include <map.h>
#include <collision.h>
#include <render.h>
#include <menu.h>
#include <levels.h>
#include <texture.h>
#include <score.h>
#include "sound.h"
#include "leaderboard.h"

// ─────────────────────────────────────────────
//  Константы окна и поля
// ─────────────────────────────────────────────
#define WINDOW_WIDTH_INIT  1920
#define WINDOW_HEIGHT_INIT 1080
#define BORDER_WIDTH       16
#define OUTER_SIZE         (FIELD_SIZE + 2 * BORDER_WIDTH)

// ─────────────────────────────────────────────
//  Состояния игры
// ─────────────────────────────────────────────
typedef enum {
    GAME_STATE_NICK_INPUT,    // ввод ника при старте
    GAME_STATE_MENU,
    GAME_STATE_LEADERBOARD,   // таблица рекордов
    GAME_STATE_LEVEL_SELECT,
    GAME_STATE_PLAYING,
    GAME_STATE_PAUSED,        // пауза
    GAME_STATE_GAME_OVER,
    GAME_STATE_VICTORY
} GameState;

// ─────────────────────────────────────────────
//  Глобальные переменные
// ─────────────────────────────────────────────
int windowWidth  = WINDOW_WIDTH_INIT;
int windowHeight = WINDOW_HEIGHT_INIT;
int fieldX = 0, fieldY = 0;
int outerX = 0, outerY = 0;

int  map[GRID_SIZE][GRID_SIZE];
Wood woods[GRID_SIZE][GRID_SIZE];

GameState gameState      = GAME_STATE_NICK_INPUT;
int selectedMenuItem     = 0;
int selectedLevel        = 0;
int total_levels         = TOTAL_LEVELS;

double gameOverTimer        = 0.0;
int    gameOverMessageIndex = 0;
int    gTimeBonusAwarded    = 0;
double victoryTimer         = 0.0;

//рекорд уже добавлен для текущей сессии победы/поражения
static int gScoreSaved = 0;

const char* gameOverMessages[] = {
    "CAPTAIN KILLED",
    "CREW CONCUSSED",
    "TANK ON FIRE",
    "AMMO EXPLOSION",
    "CRITICAL DAMAGE"
};
int gameOverMessagesCount = 5;

// Текстовый рендер
stbtt_bakedchar cdata[96];
GLuint fontTexture;
GLuint shaderProgramText;
GLint  projLocText;
GLint  textColorLoc;
GLuint VAOText;
GLuint VBOText;
float  projectionMatrix[16];

// ─────────────────────────────────────────────
//  Прототипы
// ─────────────────────────────────────────────
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void update_projection_and_field(GLuint projLoc, int width, int height);
GLuint load_shader(const char* source, GLenum type);
GLuint create_program(const char* vertexSource, const char* fragmentSource);
void ortho_projection(float left, float right, float bottom, float top, float* matrix);
void init_font(void);
void render_text(const char* text, float x, float y, float scale,
                 float r, float g, float b);
void render_controls_hud(void);

// ─────────────────────────────────────────────
//  Callback ввода символов (для ника)
// ─────────────────────────────────────────────
static void char_callback(GLFWwindow* window, unsigned int codepoint)
{
    (void)window;
    if (gameState != GAME_STATE_NICK_INPUT) return;
    int len = (int)strlen(gPlayerNick);
    if (len >= NICK_MAX_LEN - 1) return;
    // Только латиница, цифры и дефис
    if ((codepoint >= 'A' && codepoint <= 'Z') ||
        (codepoint >= 'a' && codepoint <= 'z') ||
        (codepoint >= '0' && codepoint <= '9') ||
        codepoint == '-' || codepoint == '_')
    {
        // Приводим к верхнему регистру
        if (codepoint >= 'a' && codepoint <= 'z')
            codepoint -= 32;
        gPlayerNick[len]     = (char)codepoint;
        gPlayerNick[len + 1] = '\0';
    }
}

// ─────────────────────────────────────────────
//  Шейдеры
// ─────────────────────────────────────────────
const char* vertexShaderSource =
    "#version 330 core\n"
    "layout (location = 0) in vec2 aPos;\n"
    "uniform mat4 uProjection;\n"
    "uniform mat4 uModel;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = uProjection * uModel * vec4(aPos, 0.0, 1.0);\n"
    "}\n";

const char* fragmentShaderSource =
    "#version 330 core\n"
    "out vec4 FragColor;\n"
    "uniform vec4 uColor;\n"
    "void main()\n"
    "{\n"
    "   FragColor = uColor;\n"
    "}\n";

const char* texVertexShaderSource =
    "#version 330 core\n"
    "layout (location = 0) in vec2 aPos;\n"
    "layout (location = 1) in vec2 aTexCoord;\n"
    "uniform mat4 uProjection;\n"
    "uniform mat4 uModel;\n"
    "out vec2 TexCoord;\n"
    "void main()\n"
    "{\n"
    "    gl_Position = uProjection * uModel * vec4(aPos, 0.0, 1.0);\n"
    "    TexCoord = aTexCoord;\n"
    "}\n";

const char* texFragmentShaderSource =
    "#version 330 core\n"
    "in vec2 TexCoord;\n"
    "out vec4 FragColor;\n"
    "uniform sampler2D uTexture;\n"
    "void main()\n"
    "{\n"
    "    FragColor = texture(uTexture, TexCoord);\n"
    "}\n";

const char* textVertexShaderSource =
    "#version 330 core\n"
    "layout (location = 0) in vec4 vertex;\n"
    "out vec2 TexCoords;\n"
    "uniform mat4 uProjection;\n"
    "void main()\n"
    "{\n"
    "    gl_Position = uProjection * vec4(vertex.xy, 0.0, 1.0);\n"
    "    TexCoords = vertex.zw;\n"
    "}\n";

const char* textFragmentShaderSource =
    "#version 330 core\n"
    "in vec2 TexCoords;\n"
    "out vec4 color;\n"
    "uniform sampler2D text;\n"
    "uniform vec3 textColor;\n"
    "void main()\n"
    "{\n"
    "    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);\n"
    "    color = vec4(textColor, 1.0) * sampled;\n"
    "}\n";

// ─────────────────────────────────────────────
//  Работа с шейдерами
// ─────────────────────────────────────────────
GLuint load_shader(const char* source, GLenum type)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        fprintf(stderr, "Ошибка компиляции шейдера: %s\n", infoLog);
        exit(1);
    }
    return shader;
}

GLuint create_program(const char* vertexSource, const char* fragmentSource)
{
    GLuint vertex   = load_shader(vertexSource,   GL_VERTEX_SHADER);
    GLuint fragment = load_shader(fragmentSource, GL_FRAGMENT_SHADER);
    GLuint program  = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    int success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        fprintf(stderr, "Ошибка линковки программы: %s\n", infoLog);
        exit(1);
    }
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    return program;
}

// ─────────────────────────────────────────────
//  Матрица проекции
// ─────────────────────────────────────────────
void ortho_projection(float left, float right, float bottom, float top, float* matrix)
{
    for (int i = 0; i < 16; i++) matrix[i] = 0.0f;
    matrix[0]  =  2.0f / (right - left);
    matrix[5]  =  2.0f / (top - bottom);
    matrix[10] = -1.0f;
    matrix[12] = -(right + left) / (right - left);
    matrix[13] = -(top + bottom) / (top - bottom);
    matrix[15] =  1.0f;
}

// ─────────────────────────────────────────────
//  Инициализация шрифта
// ─────────────────────────────────────────────
void init_font(void)
{
    FILE* fontFile = fopen("Assets/Fonts/arial.ttf", "rb");
    if (!fontFile) {
        fprintf(stderr, "Предупреждение: Шрифт не найден.\n");
        return;
    }

    fseek(fontFile, 0, SEEK_END);
    long fontSize = ftell(fontFile);
    fseek(fontFile, 0, SEEK_SET);
    unsigned char* fontBuffer = (unsigned char*)malloc(fontSize);
    fread(fontBuffer, 1, fontSize, fontFile);
    fclose(fontFile);

    unsigned char* tempBitmap = (unsigned char*)calloc(512 * 512, 1);
    stbtt_BakeFontBitmap(fontBuffer, 0, 48.0f, tempBitmap, 512, 512, 32, 96, cdata);

    glGenTextures(1, &fontTexture);
    glBindTexture(GL_TEXTURE_2D, fontTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 512, 512, 0,
                 GL_RED, GL_UNSIGNED_BYTE, tempBitmap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    free(tempBitmap);
    free(fontBuffer);
}

// ─────────────────────────────────────────────
//  Отрисовка текста
// ─────────────────────────────────────────────
void render_text(const char* text, float x, float y, float scale,
                 float r, float g, float b)
{
    if (!shaderProgramText || !fontTexture) return;

    glUseProgram(shaderProgramText);
    glUniformMatrix4fv(projLocText, 1, GL_FALSE, projectionMatrix);
    glUniform3f(textColorLoc, r, g, b);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fontTexture);
    glUniform1i(glGetUniformLocation(shaderProgramText, "text"), 0);
    glBindVertexArray(VAOText);

    #define MAX_CHARS        256
    #define VERTICES_PER_CHAR  6
    #define FLOATS_PER_VERTEX  4

    float vertices[MAX_CHARS * VERTICES_PER_CHAR][FLOATS_PER_VERTEX];
    int   vertexCount = 0;
    float currentX = x, currentY = y;
    float scaleX = scale, scaleY = scale;

    for (const char* p = text;
         *p && vertexCount < MAX_CHARS * VERTICES_PER_CHAR; p++)
    {
        unsigned char ch = (unsigned char)*p;
        if (ch >= 32 && ch < 128) {
            stbtt_aligned_quad q;
            float cx2 = currentX, cy2 = currentY;
            stbtt_GetBakedQuad(cdata, 512, 512, ch - 32, &cx2, &cy2, &q, 1);

            // Применяем масштаб вокруг начальной точки
            float qx0 = x + (q.x0 - x) * scaleX + (cx2 - currentX) * (scaleX - 1.0f);
            // Упрощённый вариант: просто масштабируем координаты
            float ox = currentX, oy = currentY;
            q.x0 = ox + (q.x0 - ox) * scaleX;
            q.x1 = ox + (q.x1 - ox) * scaleX;
            q.y0 = oy + (q.y0 - oy) * scaleY;
            q.y1 = oy + (q.y1 - oy) * scaleY;
            currentX = ox + (cx2 - ox) * scaleX;
            currentY = cy2;
            (void)qx0;

            float* v = vertices[vertexCount];
            v[0]=q.x0; v[1]=q.y0; v[2]=q.s0; v[3]=q.t0; vertexCount++;
            v[4]=q.x1; v[5]=q.y0; v[6]=q.s1; v[7]=q.t0; vertexCount++;
            v[8]=q.x1; v[9]=q.y1; v[10]=q.s1;v[11]=q.t1; vertexCount++;
            v[12]=q.x0;v[13]=q.y0;v[14]=q.s0;v[15]=q.t0; vertexCount++;
            v[16]=q.x1;v[17]=q.y1;v[18]=q.s1;v[19]=q.t1; vertexCount++;
            v[20]=q.x0;v[21]=q.y1;v[22]=q.s0;v[23]=q.t1; vertexCount++;
        }
    }

    if (vertexCount > 0) {
        glBindBuffer(GL_ARRAY_BUFFER, VBOText);
        glBufferData(GL_ARRAY_BUFFER,
                     sizeof(float) * vertexCount * FLOATS_PER_VERTEX,
                     vertices, GL_DYNAMIC_DRAW);
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    }

    glBindVertexArray(0);
}

// ─────────────────────────────────────────────
//  HUD управления (справа от поля)
// ─────────────────────────────────────────────
void render_controls_hud(void)
{
    Level* lvl = &levels[currentLevelIndex];
    
    char text[64];
    sprintf(text, "LIVES: %d", player.lives);
    render_text(text, outerX - 400, outerY + 150, 1.0f, 1.0f, 1.0f, 1.0f);

    sprintf(text, "ENEMIES: %d", lvl->botCurrent);
    render_text(text, outerX - 400, outerY + 220, 1.0f, 1.0f, 1.0f, 1.0f);
    
    sprintf(text, "HP BASE:%d", gBase.hp);
    render_text(text, outerX - 400, outerY + 290, 1.0f, 1.0f, 1.0f, 1.0f);
    
    render_text(gPlayerNick, outerX - 400, outerY + 360, 1.0f, 0.2f, 1.0f, 0.4f);
    
    float panelX = outerX + OUTER_SIZE + 100.0f;
    float panelY = (float)fieldY + 20.0f;
    float lineH  = 55.0f;

    render_text("CONTROLS", panelX + 10, panelY, 1.2f, 1.0f, 0.8f, 0.2f);
    panelY += lineH * 1.4f;

    // Клавиши движения
    render_text("MOVE", panelX + 100, panelY, 0.9f, 0.6f, 0.6f, 0.6f);
    panelY += lineH;
    render_text("W", panelX + 140, panelY, 1.0f, 1.0f, 1.0f, 1.0f);
    panelY += lineH * 0.85f;
    render_text("A  S  D", panelX + 80, panelY, 1.0f, 1.0f, 1.0f, 1.0f);
    panelY += lineH * 1.4f;

    render_text("SHOOT", panelX + 80, panelY, 0.9f, 0.6f, 0.6f, 0.6f);
    panelY += lineH;
    render_text("SPACE", panelX + 73, panelY, 1.0f, 1.0f, 1.0f, 1.0f);
    panelY += lineH * 1.4f;

    render_text("PAUSE", panelX + 80, panelY, 0.9f, 0.6f, 0.6f, 0.6f);
    panelY += lineH;
    render_text("ESC", panelX + 100, panelY, 1.0f, 1.0f, 1.0f, 1.0f);
}

// ─────────────────────────────────────────────
//  Обновление проекции при ресайзе
// ─────────────────────────────────────────────
void update_projection_and_field(GLuint projLoc, int width, int height)
{
    int oldFieldX = fieldX;
    int oldFieldY = fieldY;

    windowWidth  = width;
    windowHeight = height;

    ortho_projection(0.0f, (float)width, (float)height, 0.0f, projectionMatrix);

    // Цветной шейдер (рамка, прямоугольники) — нужен явный glUseProgram
    glUseProgram(gRender.shaderProgram);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, projectionMatrix);

    // Текстурный шейдер
    glUseProgram(gRender.texShaderProgram);
    glUniformMatrix4fv(gRender.texProjLoc, 1, GL_FALSE, projectionMatrix);

    // Текстовый шейдер
    if (shaderProgramText) {
        glUseProgram(shaderProgramText);
        glUniformMatrix4fv(projLocText, 1, GL_FALSE, projectionMatrix);
    }

    fieldX = (width  - FIELD_SIZE) / 2;
    fieldY = (height - FIELD_SIZE) / 2;
    outerX = fieldX - BORDER_WIDTH;
    outerY = fieldY - BORDER_WIDTH;

    int dx = fieldX - oldFieldX;
    int dy = fieldY - oldFieldY;

    player.x    += dx;
    player.y    += dy;
    sp_player.x += dx;
    sp_player.y += dy;

    for (int i = 0; i < MAX_BOTS; i++) {
        if (!bots[i].active) continue;
        bots[i].x += dx;
        bots[i].y += dy;
        if (bots[i].b_bullet.active) {
            bots[i].b_bullet.x += dx;
            bots[i].b_bullet.y += dy;
        }
    }

    for (int i = 0; i < GRID_SIZE * GRID_SIZE; i++) {
        if (sp_bots[i].x == 0 && sp_bots[i].y == 0) continue;
        sp_bots[i].x += dx;
        sp_bots[i].y += dy;
    }

    gBase.x += dx;
    gBase.y += dy;

    if (player.p_bullet.active) {
        player.p_bullet.x += dx;
        player.p_bullet.y += dy;
    }

    for (int j = 0; j < GRID_SIZE; j++) {
        for (int i = 0; i < GRID_SIZE; i++) {
            if (map[j][i] == 5) {
                woods[j][i].x += dx;
                woods[j][i].y += dy;
            }
        }
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
    GLuint projLoc = *(GLuint*)glfwGetWindowUserPointer(window);
    update_projection_and_field(projLoc, width, height);
}

// ─────────────────────────────────────────────
//  main
// ─────────────────────────────────────────────
int main(void)
{
    srand((unsigned int)time(NULL));

    // ── GLFW ──
    if (!glfwInit()) {
        fprintf(stderr, "Не удалось инициализировать GLFW\n");
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight,
                                          "ТАНКИ - КТО ТЫ ВОИН ?", NULL, NULL);
    if (!window) {
        fprintf(stderr, "Не удалось создать окно\n");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        fprintf(stderr, "Не удалось загрузить glad\n");
        return -1;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // ── Шейдеры ──
    GLuint shaderProgram = create_program(vertexShaderSource, fragmentShaderSource);
    glUseProgram(shaderProgram);
    GLint projLoc  = glGetUniformLocation(shaderProgram, "uProjection");
    GLint modelLoc = glGetUniformLocation(shaderProgram, "uModel");
    GLint colorLoc = glGetUniformLocation(shaderProgram, "uColor");

    shaderProgramText = create_program(textVertexShaderSource, textFragmentShaderSource);
    projLocText  = glGetUniformLocation(shaderProgramText, "uProjection");
    textColorLoc = glGetUniformLocation(shaderProgramText, "textColor");

    glfwSetWindowUserPointer(window, &projLoc);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCharCallback(window, char_callback); // для ввода ника

    // ── Проекция и поле ──
    fieldX = (windowWidth  - FIELD_SIZE) / 2;
    fieldY = (windowHeight - FIELD_SIZE) / 2;
    outerX = fieldX - BORDER_WIDTH;
    outerY = fieldY - BORDER_WIDTH;
    ortho_projection(0.0f, (float)windowWidth, (float)windowHeight,
                     0.0f, projectionMatrix);
    glUseProgram(shaderProgram);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, projectionMatrix);

    // ── Шрифт ──
    init_font();

    // ── VAO для текста ──
    glGenVertexArrays(1, &VAOText);
    glGenBuffers(1, &VBOText);
    glBindVertexArray(VAOText);
    glBindBuffer(GL_ARRAY_BUFFER, VBOText);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // ── VAO для квадратов ──
    float quadVerts[] = {
        -0.5f, -0.5f,  0.0f, 0.0f,
         0.5f, -0.5f,  1.0f, 0.0f,
         0.5f,  0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.0f, 1.0f
    };
    unsigned int quadIdx[] = { 0,1,2, 0,2,3 };

    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerts), quadVerts, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIdx), quadIdx, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    GLuint texShaderProgram = create_program(texVertexShaderSource, texFragmentShaderSource);
    GLint  texProjLoc   = glGetUniformLocation(texShaderProgram, "uProjection");
    GLint  texModelLoc  = glGetUniformLocation(texShaderProgram, "uModel");

    textures_load();

    render_init(shaderProgram, VAO, modelLoc, colorLoc, projLoc,
                texShaderProgram, texModelLoc, texProjLoc,
                shaderProgramText, VAOText, VBOText,
                projLocText, textColorLoc, fontTexture);
    levels_init();
    sound_init();
    sound_play_music("menu_music");

    // Загружаем таблицу рекордов из файла
    leaderboard_load();

    // ── Переменные цикла ──
    double lastTime    = glfwGetTime();
    double lastKeyTime = 0.0;
    double keyDelay    = 0.2;
    int    once        = 0;
    GameState lastGameState = GAME_STATE_NICK_INPUT;

    // Инициализируем ник пустой строкой
    gPlayerNick[0] = '\0';

    // ── Основной цикл ──
    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        float  deltaTime   = (float)(currentTime - lastTime);
        lastTime = currentTime;
        sound_update_delayed(currentTime);

        // ── Ввод ника ─────────────────────────────────────────────────
        if (gameState == GAME_STATE_NICK_INPUT) {
            // Backspace
            if (glfwGetKey(window, GLFW_KEY_BACKSPACE) == GLFW_PRESS &&
                currentTime - lastKeyTime > 0.1)
            {
                int len = (int)strlen(gPlayerNick);
                if (len > 0) gPlayerNick[len - 1] = '\0';
                lastKeyTime = currentTime;
            }
            // Enter — подтверждение
            if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS &&
                currentTime - lastKeyTime > keyDelay)
            {
                if (strlen(gPlayerNick) == 0)
                    strncpy(gPlayerNick, "PLAYER", NICK_MAX_LEN - 1);
                gameState        = GAME_STATE_MENU;
                selectedMenuItem = 0;
                lastKeyTime      = currentTime;
            }
        }

        // ── Меню ──────────────────────────────────────────────────────
        else if (gameState == GAME_STATE_MENU) {
            if (currentTime - lastKeyTime > keyDelay) {
                if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
                    selectedMenuItem = (selectedMenuItem - 1 + 3) % 3;
                    lastKeyTime = currentTime;
                } else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
                    selectedMenuItem = (selectedMenuItem + 1) % 3;
                    lastKeyTime = currentTime;
                } else if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
                    if (selectedMenuItem == 0) {
                        gameState     = GAME_STATE_LEVEL_SELECT;
                        selectedLevel = 0;
                    } else if (selectedMenuItem == 1) {
                        gameState = GAME_STATE_LEADERBOARD;
                    } else {
                        glfwSetWindowShouldClose(window, 1);
                    }
                    lastKeyTime = currentTime;
                }
            }
        }

        // ── Таблица рекордов ──────────────────────────────────────────
        else if (gameState == GAME_STATE_LEADERBOARD) {
            if (currentTime - lastKeyTime > keyDelay) {
                if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS ||
                    glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
                {
                    gameState        = GAME_STATE_MENU;
                    selectedMenuItem = 0;
                    lastKeyTime      = currentTime;
                }
            }
        }

        // ── Выбор уровня ──────────────────────────────────────────────
        else if (gameState == GAME_STATE_LEVEL_SELECT) {
            if (currentTime - lastKeyTime > keyDelay) {
                int items = total_levels + 1;
                if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
                    selectedLevel = (selectedLevel - 1 + items) % items;
                    lastKeyTime = currentTime;
                } else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
                    selectedLevel = (selectedLevel + 1) % items;
                    lastKeyTime = currentTime;
                } else if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
                    if (selectedLevel < total_levels) {
                        load_level(selectedLevel);
                        gameState    = GAME_STATE_PLAYING;
                        lastTime     = glfwGetTime();
                        once         = 0;
                        gScoreSaved  = 0;
                    } else {
                        gameState        = GAME_STATE_MENU;
                        selectedMenuItem = 0;
                    }
                    lastKeyTime = currentTime;
                }
            }
        }

        // ── Пауза ─────────────────────────────────────────────────────
        else if (gameState == GAME_STATE_PAUSED) {
            if (currentTime - lastKeyTime > keyDelay) {
                // ESC — возобновить
                if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
                    gameState   = GAME_STATE_PLAYING;
                    lastKeyTime = currentTime;
                } else if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
                    selectedMenuItem = (selectedMenuItem - 1 + 3) % 3;
                    lastKeyTime = currentTime;
                } else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
                    selectedMenuItem = (selectedMenuItem + 1) % 3;
                    lastKeyTime = currentTime;
                } else if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
                    if (selectedMenuItem == 0) {
                        // Resume
                        gameState = GAME_STATE_PLAYING;
                    } else if (selectedMenuItem == 1) {
                        // Restart
                        load_level(selectedLevel);
                        gameState   = GAME_STATE_PLAYING;
                        once        = 0;
                        gScoreSaved = 0;
                    } else {
                        // Main menu
                        gameState        = GAME_STATE_MENU;
                        selectedMenuItem = 0;
                    }
                    lastKeyTime = currentTime;
                }
            }
        }

        // ── Game over ─────────────────────────────────────────────────
        else if (gameState == GAME_STATE_GAME_OVER) {
            double timePassed = currentTime - gameOverTimer;

            if (timePassed < 4.0) {
                if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS &&
                    currentTime - lastKeyTime > keyDelay) {
                    gameOverTimer = currentTime - 4.0;
                    lastKeyTime   = currentTime;
                }
            } else {
                if (currentTime - lastKeyTime > keyDelay) {
                    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
                        selectedMenuItem = (selectedMenuItem - 1 + 2) % 2;
                        lastKeyTime = currentTime;
                    } else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
                        selectedMenuItem = (selectedMenuItem + 1) % 2;
                        lastKeyTime = currentTime;
                    } else if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
                        if (selectedMenuItem == 0) {
                            load_level(selectedLevel);
                            gameState   = GAME_STATE_PLAYING;
                            once        = 0;
                            gScoreSaved = 0;
                        } else {
                            gameState        = GAME_STATE_MENU;
                            selectedMenuItem = 0;
                        }
                        lastKeyTime = currentTime;
                    }
                }
            }
        }

        // ── Victory ───────────────────────────────────────────────────
        else if (gameState == GAME_STATE_VICTORY) {
            double timePassed = currentTime - victoryTimer;

            if (timePassed < 4.0) {
                if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS &&
                    currentTime - lastKeyTime > keyDelay) {
                    victoryTimer = currentTime - 4.0;
                    lastKeyTime  = currentTime;
                }
            } else {
                if (currentTime - lastKeyTime > keyDelay) {
                    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
                        selectedMenuItem = (selectedMenuItem - 1 + 2) % 2;
                        lastKeyTime = currentTime;
                    } else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
                        selectedMenuItem = (selectedMenuItem + 1) % 2;
                        lastKeyTime = currentTime;
                    } else if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
                        if (selectedMenuItem == 0) {
                            int next = selectedLevel + 1;
                            if (next < total_levels) {
                                selectedLevel = next;
                                load_level(selectedLevel);
                                gameState   = GAME_STATE_PLAYING;
                                once        = 0;
                                gScoreSaved = 0;
                            } else {
                                gameState        = GAME_STATE_MENU;
                                selectedMenuItem = 0;
                            }
                        } else {
                            gameState        = GAME_STATE_MENU;
                            selectedMenuItem = 0;
                        }
                        lastKeyTime = currentTime;
                    }
                }
            }
        }

        // ── Игровая логика ────────────────────────────────────────────
        else if (gameState == GAME_STATE_PLAYING) {

            // ESC → пауза
            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS &&
                currentTime - lastKeyTime > keyDelay)
            {
                gameState        = GAME_STATE_PAUSED;
                selectedMenuItem = 0;
                lastKeyTime      = currentTime;
            }

            player_update(window, deltaTime, currentTime,
                          fieldX, fieldY, FIELD_SIZE);

            int spawn_count = 0;
            for (int j = 0; j < GRID_SIZE; j++)
                for (int i = 0; i < GRID_SIZE; i++)
                    if (map[j][i] == 6) spawn_count++;

            bots_update(deltaTime, currentTime,
                        fieldX, fieldY, FIELD_SIZE,
                        sp_bots, spawn_count);

            // Победа
            if (level_is_complete()) {
                double elapsed = glfwGetTime() - gLevelStartTime;
                gTimeBonusAwarded = score_time_bonus(selectedLevel, elapsed);
                victoryTimer     = glfwGetTime();
                gameState        = GAME_STATE_VICTORY;
                selectedMenuItem = 0;
                sound_play_delayed("victory", 1.5);
                // Сохраняем рекорд
                if (!gScoreSaved) {
                    leaderboard_add(gPlayerNick, gScore, selectedLevel);
                    leaderboard_save();
                    gScoreSaved = 1;
                }
            }

            // База уничтожена
            if (base_is_destroyed()) {
                gameOverMessageIndex = rand() % gameOverMessagesCount;
                gameOverTimer        = glfwGetTime();
                gameState            = GAME_STATE_GAME_OVER;
                selectedMenuItem     = 0;
                once                 = 1;
                sound_play("game_over");
                if (!gScoreSaved) {
                    leaderboard_add(gPlayerNick, gScore, selectedLevel);
                    leaderboard_save();
                    gScoreSaved = 1;
                }
            }

            // Смерть игрока
            if (player.dead && !once) {
                once                 = 1;
                gameOverMessageIndex = rand() % gameOverMessagesCount;
                gameOverTimer        = glfwGetTime();
                gameState            = GAME_STATE_GAME_OVER;
                selectedMenuItem     = 0;
                sound_play_delayed("game_over", 1.5);
                if (!gScoreSaved) {
                    leaderboard_add(gPlayerNick, gScore, selectedLevel);
                    leaderboard_save();
                    gScoreSaved = 1;
                }
            }
        }

        // ── Музыка при смене состояний ────────────────────────────────
        if (gameState != lastGameState) {
            if (gameState == GAME_STATE_PLAYING ||
                gameState == GAME_STATE_GAME_OVER ||
                gameState == GAME_STATE_VICTORY) {
                if (lastGameState != GAME_STATE_PAUSED)
                    sound_stop_music();
            } else if ((gameState == GAME_STATE_MENU ||
                        gameState == GAME_STATE_LEVEL_SELECT ||
                        gameState == GAME_STATE_LEADERBOARD ||
                        gameState == GAME_STATE_NICK_INPUT) &&
                       (lastGameState == GAME_STATE_PLAYING ||
                        lastGameState == GAME_STATE_GAME_OVER ||
                        lastGameState == GAME_STATE_VICTORY)) {
                sound_play_music("menu_music");
            }
            lastGameState = gameState;
        }

        // ─────────────────────────────────────────────
        //  Отрисовка
        // ─────────────────────────────────────────────
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        if (gameState == GAME_STATE_PLAYING || gameState == GAME_STATE_PAUSED) {
            float gray[4] = {0.5f, 0.5f, 0.5f, 1.0f};
            draw_rect(outerX + OUTER_SIZE / 2.0f,
                      outerY + OUTER_SIZE / 2.0f,
                      OUTER_SIZE, OUTER_SIZE, gray);

            render_map();
            render_base();
            render_player();
            render_bots();
            render_foliage();
            render_controls_hud();

        if (gameState == GAME_STATE_PAUSED)
                render_pause_menu();
        }
        else if (gameState == GAME_STATE_NICK_INPUT) {
            render_nick_input();
        }
        else if (gameState == GAME_STATE_MENU) {
            render_menu();
        }
        else if (gameState == GAME_STATE_LEADERBOARD) {
            render_leaderboard();
        }
        else if (gameState == GAME_STATE_LEVEL_SELECT) {
            render_level_select();
        }
        else if (gameState == GAME_STATE_VICTORY) {
            render_victory_screen();
        }
        else if (gameState == GAME_STATE_GAME_OVER) {
            if (glfwGetTime() - gameOverTimer >= 4.0)
                render_death_menu();
            else
                render_game_over_screen();
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // ── Очистка ──
    leaderboard_free();
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);
    if (shaderProgramText) glDeleteProgram(shaderProgramText);
    if (fontTexture)       glDeleteTextures(1, &fontTexture);
    if (VAOText)           glDeleteVertexArrays(1, &VAOText);
    if (VBOText)           glDeleteBuffers(1, &VBOText);

    sound_cleanup();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
