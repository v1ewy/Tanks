#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

// ------------------ Константы ------------------
//окно
#define WINDOW_WIDTH_INIT 1920
#define WINDOW_HEIGHT_INIT 1080

#define FIELD_SIZE 832          // 13 * 64 размер поля
#define BORDER_WIDTH 16         // ширина рамки
#define OUTER_SIZE (FIELD_SIZE + 2 * BORDER_WIDTH) // 864

#define PLAYER_SIZE 58    // танк
#define PLAYER_SPEED 175.0f  // скорость игрока
#define PLAYER_SHOOT_DELAY 0.4f  // задержка между выстрелами

#define BULLET_SPEED 600.0f  // скорость пули
#define BULLET_WIDTH 20.0f   // ширина пули
#define BULLET_HEIGHT 10.0f  // высота пули

// бот
#define MAX_BOTS 4 
#define BOT_SPAWN_INTERVAL 4.0f 
#define BOT_ROTATE_INTERVAL 1.0f
#define BOT_SIZE 58
#define BOT_SPEED 175.0f  
#define BOT_SHOOT_DELAY 2.0f  
#define INVINCIBLE_DURATION 2.0f 

// параметры карты
#define GRID_SIZE 13
#define CELL_SIZE 64
#define BLOCK_SIZE 64

// ------------------ Состояния игры ------------------
typedef enum {
    GAME_STATE_MENU,
    GAME_STATE_LEVEL_SELECT,
    GAME_STATE_PLAYING,
    GAME_STATE_GAME_OVER
} GameState;

// Структура для уровней
typedef struct {
    int map[GRID_SIZE][GRID_SIZE];
    int player_lives;
} Level;

// Уровень 1
Level levels[1];

// Текущая карта
int map[GRID_SIZE][GRID_SIZE];

// ------------------ Структуры ------------------
typedef struct {
    double lastShootTime;
    float x, y;
    float dirX, dirY;
    int active;
} Bullet;

typedef struct {
    Bullet p_bullet;
    float x, y;
    int dead;
    int lives;
    float invincibleTimer;
} Player;

typedef struct {
    Bullet b_bullet;
    double deathTime;
    double nextRotateTime;
    float x, y;
    float dirX, dirY;
    int active;
    float invincibleTimer;
} Bot;

typedef struct {
    float x, y;
} Spawner;

typedef struct {
    int width, height;
    float x, y;
} Wood;

// ------------------ Глобальные переменные ------------------
int windowWidth = WINDOW_WIDTH_INIT;
int windowHeight = WINDOW_HEIGHT_INIT;
int fieldX = 0, fieldY = 0;
int outerX = 0, outerY = 0;

Player player;
Spawner sp_player;

Bot bots[MAX_BOTS];
Spawner sp_bots[GRID_SIZE * GRID_SIZE];

Wood woods[GRID_SIZE][GRID_SIZE];

GameState gameState = GAME_STATE_MENU;
int selectedMenuItem = 0;
int selectedLevel = 0;
int total_levels = 1;

// Для Game Over
double gameOverTimer = 0.0;
int gameOverMessageIndex = 0;
int showDeathMenu = 0;

// Фразы для Game Over
const char* gameOverMessages[] = {
    "КАПИТАН УБИТ",
    "ЭКИПАЖ КОНТУЖЕН",
    "ТАНК ГОРИТ",
    "ВЗРЫВ БОЕКОМПЛЕКТА",
    "КРИТИЧЕСКИЕ ПОВРЕЖДЕНИЯ"
};
int gameOverMessagesCount = 5;

// Для текста
stbtt_bakedchar cdata[96];
GLuint fontTexture;
GLuint shaderProgramText;
GLint projLocText;
GLint textColorLoc;
GLuint VAOText;
GLuint VBOText;
float projectionMatrix[16];

// ------------------ Прототипы функций ------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void update_projection_and_field(GLuint projLoc, int width, int height);
GLuint load_shader(const char* source, GLenum type);
GLuint create_program(const char* vertexSource, const char* fragmentSource);
void ortho_projection(float left, float right, float bottom, float top, float* matrix);
int is_wall(int x, int y);
int is_water(int x, int y);
int is_foliage(int x, int y);
int is_wood(int x, int y);
int check_rect_collision_with_map(int who, float cx, float cy, float w, float h, float bulletDirX, float bulletDirY);
void load_level(int level_index);
void init_font(void);
void render_text(const char* text, float x, float y, float scale, float r, float g, float b);
void render_menu(void);
void render_level_select(void);
void render_game_over_screen(void);
void render_death_menu(void);

// ------------------ Шейдеры ------------------
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

// ------------------ Функции работы с шейдерами ------------------
GLuint load_shader(const char* source, GLenum type) {
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

GLuint create_program(const char* vertexSource, const char* fragmentSource) {
    GLuint vertex = load_shader(vertexSource, GL_VERTEX_SHADER);
    GLuint fragment = load_shader(fragmentSource, GL_FRAGMENT_SHADER);
    GLuint program = glCreateProgram();
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

// ------------------ Матрица ортографической проекции ------------------
void ortho_projection(float left, float right, float bottom, float top, float* matrix) {
    for (int i = 0; i < 16; i++) matrix[i] = 0.0f;
    matrix[0] = 2.0f / (right - left);
    matrix[5] = 2.0f / (top - bottom);
    matrix[10] = -1.0f;
    matrix[12] = -(right + left) / (right - left);
    matrix[13] = -(top + bottom) / (top - bottom);
    matrix[15] = 1.0f;
}

// ------------------ Проверка блока по карте ------------------
int is_wall(int x, int y) {
    if (x < 0 || x >= GRID_SIZE || y < 0 || y >= GRID_SIZE) return 1;
    return map[y][x] == 2;
}

int is_water(int x, int y) {
    if (x < 0 || x >= GRID_SIZE || y < 0 || y >= GRID_SIZE) return 0;
    return map[y][x] == 3;
}

int is_foliage(int x, int y) {
    if (x < 0 || x >= GRID_SIZE || y < 0 || y >= GRID_SIZE) return 0;
    return map[y][x] == 4;
}

int is_wood(int x, int y) {
    if (x < 0 || x >= GRID_SIZE || y < 0 || y >= GRID_SIZE) return 0;
    return map[y][x] == 5;
}

// ------------------ Проверка коллизии ------------------
int check_rect_collision_with_map(int who, float cx, float cy, float w, float h, float bulletDirX, float bulletDirY) {
    float halfW = w / 2.0f;
    float halfH = h / 2.0f;
    int left = (int)((cx - halfW - fieldX) / CELL_SIZE);
    int right = (int)((cx + halfW - fieldX) / CELL_SIZE);
    int top = (int)((cy - halfH - fieldY) / CELL_SIZE);
    int bottom = (int)((cy + halfH - fieldY) / CELL_SIZE);

    for (int j = top; j <= bottom; j++) {
        for (int i = left; i <= right; i++) {
            switch (who) {
            case 1: // игрок
                if (is_wall(i, j) || is_water(i, j)) return 1;
                else if (is_wood(i, j)) {
                    float dx = fabs(cx - woods[j][i].x);
                    float dy = fabs(cy - woods[j][i].y);
                    if (dx < (w + woods[j][i].width) / 2 && dy < (h + woods[j][i].height) / 2) {
                        return 1;
                    }
                }
                else {
                    for (int k = 0; k < MAX_BOTS; k++) {
                        if (bots[k].active) {
                            float dx = fabs(cx - bots[k].x);
                            float dy = fabs(cy - bots[k].y);
                            if (dx < (w + BOT_SIZE) / 2 && dy < (h + BOT_SIZE) / 2) return 1;
                        }
                    }
                }
                break;
            case 2: // пуля игрока
                if (is_wall(i, j)) return 1;
                if (is_wood(i, j)) {
                    float dx = fabs(cx - woods[j][i].x);
                    float dy = fabs(cy - woods[j][i].y);
                    if (dx < (w + woods[j][i].width) / 2 && dy < (h + woods[j][i].height) / 2) {
                        if (bulletDirX != 0) {
                            woods[j][i].width -= 16;
                            woods[j][i].x += bulletDirX * 8;
                        }
                        else if (bulletDirY != 0) {
                            woods[j][i].height -= 16;
                            woods[j][i].y += bulletDirY * 8;
                        }
                        if (woods[j][i].width <= 0 || woods[j][i].height <= 0) {
                            map[j][i] = 0;
                        }
                        return 1;
                    }
                }
                else {
                    for (int k = 0; k < MAX_BOTS; k++) {
                        if (bots[k].active) {
                            float dx = fabs(cx - bots[k].x);
                            float dy = fabs(cy - bots[k].y);
                            if (dx < (w + BOT_SIZE) / 2 && dy < (h + BOT_SIZE) / 2) {
                                if (bots[k].invincibleTimer <= 0) {
                                    bots[k].active = 0;
                                    bots[k].deathTime = glfwGetTime();
                                    return 1;
                                }
                            }
                        }
                    }
                }
                break;
            case 3: // пуля бота
                if (is_wall(i, j)) return 1;
                if (is_wood(i, j)) {
                    float dx = fabs(cx - woods[j][i].x);
                    float dy = fabs(cy - woods[j][i].y);
                    if (dx < (w + woods[j][i].width) / 2 && dy < (h + woods[j][i].height) / 2) {
                        if (bulletDirX != 0) {
                            woods[j][i].width -= 16;
                            woods[j][i].x += bulletDirX * 8;
                        }
                        else if (bulletDirY != 0) {
                            woods[j][i].height -= 16;
                            woods[j][i].y += bulletDirY * 8;
                        }
                        if (woods[j][i].width <= 0 || woods[j][i].height <= 0) {
                            map[j][i] = 0;
                        }
                        return 1;
                    }
                }
                else {
                    float dx = fabs(cx - player.x);
                    float dy = fabs(cy - player.y);
                    if (dx < (w + PLAYER_SIZE) / 2 && dy < (h + PLAYER_SIZE) / 2) {
                        if (player.invincibleTimer <= 0.0f) {
                            player.lives--;
                            if (player.lives <= 0) {
                                player.dead = 1;
                            }
                            else {
                                player.x = sp_player.x;
                                player.y = sp_player.y;
                                player.invincibleTimer = INVINCIBLE_DURATION;
                            }
                        }
                        return 1;
                    }
                }
                break;
            case 4: // бот
                if (is_wall(i, j) || is_water(i, j)) return 1;
                else if (is_wood(i, j)) {
                    float dx = fabs(cx - woods[j][i].x);
                    float dy = fabs(cy - woods[j][i].y);
                    if (dx < (w + woods[j][i].width) / 2 && dy < (h + woods[j][i].height) / 2) {
                        return 1;
                    }
                }
                else {
                    for (int k = 0; k < MAX_BOTS; k++) {
                        if (bots[k].active && k != (int)bulletDirX) {
                            float dx = fabs(cx - bots[k].x);
                            float dy = fabs(cy - bots[k].y);
                            if (dx < (w + BOT_SIZE) / 2 && dy < (h + BOT_SIZE) / 2) return 1;
                        }
                    }
                    float dxPlayer = fabs(cx - player.x);
                    float dyPlayer = fabs(cy - player.y);
                    if (dxPlayer < (w + PLAYER_SIZE) / 2 && dyPlayer < (h + PLAYER_SIZE) / 2) {
                        return 1;
                    }
                }
                break;
            }
        }
    }
    return 0;
}

// ------------------ Инициализация шрифта ------------------
void init_font(void) {
    const char* fontPaths = "Tanks/Assets/Fonts/arial.ttf"
    ;

    FILE* fontFile = NULL;
    fontFile = fopen(fontPaths, "rb");

    if (!fontFile) {
        fprintf(stderr, "Предупреждение: Шрифт не найден. Текст отображаться не будет.\n");
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
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 512, 512, 0, GL_RED, GL_UNSIGNED_BYTE, tempBitmap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    free(tempBitmap);
    free(fontBuffer);
}

// ------------------ Отрисовка текста ------------------
void render_text(const char* text, float x, float y, float scale, float r, float g, float b) {
    if (!shaderProgramText || !fontTexture) return;

    glUseProgram(shaderProgramText);
    glUniformMatrix4fv(projLocText, 1, GL_FALSE, projectionMatrix);
    glUniform3f(textColorLoc, r, g, b);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fontTexture);
    glUniform1i(glGetUniformLocation(shaderProgramText, "text"), 0);

    glBindVertexArray(VAOText);

    float currentX = x;
    float currentY = y;
    
    // Максимальное количество символов (можно увеличить при необходимости)
    #define MAX_CHARS 256
    #define VERTICES_PER_CHAR 6
    #define FLOATS_PER_VERTEX 4
    
    float vertices[MAX_CHARS * VERTICES_PER_CHAR][FLOATS_PER_VERTEX];
    int vertexCount = 0;

    for (const char* p = text; *p && vertexCount < MAX_CHARS * VERTICES_PER_CHAR; p++) {
        unsigned char ch = (unsigned char)*p;  // Исправление 1: приведение к unsigned char
        
        if (ch >= 32 && ch < 128) {  // Теперь условие работает корректно
            stbtt_aligned_quad q;
            stbtt_GetBakedQuad(cdata, 512, 512, ch - 32, &currentX, &currentY, &q, 1);
            
            // Добавляем вершины текущего символа в общий буфер
            float* v = vertices[vertexCount];
            v[0] = q.x0; v[1] = q.y0; v[2] = q.s0; v[3] = q.t0; vertexCount++;
            v[4] = q.x1; v[5] = q.y0; v[6] = q.s1; v[7] = q.t0; vertexCount++;
            v[8] = q.x1; v[9] = q.y1; v[10] = q.s1; v[11] = q.t1; vertexCount++;
            v[12] = q.x0; v[13] = q.y0; v[14] = q.s0; v[15] = q.t0; vertexCount++;
            v[16] = q.x1; v[17] = q.y1; v[18] = q.s1; v[19] = q.t1; vertexCount++;
            v[20] = q.x0; v[21] = q.y1; v[22] = q.s0; v[23] = q.t1; vertexCount++;
        }
    }

    // Отрисовываем все символы одним вызовом (Исправление 2: batch rendering)
    if (vertexCount > 0) {
        glBindBuffer(GL_ARRAY_BUFFER, VBOText);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertexCount * FLOATS_PER_VERTEX, vertices, GL_DYNAMIC_DRAW);
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    }

    glBindVertexArray(0);
}

// ------------------ Отрисовка меню ------------------
void render_menu(void) {
    float centerX = windowWidth / 2;
    float centerY = windowHeight / 2;

    render_text("TANKS GAME", centerX - 180, centerY - 350, 2.5f, 1.0f, 0.8f, 0.2f);

    const char* items[] = { "START GAME", "EXIT" };
    for (int i = 0; i < 2; i++) {
        float y = centerY - 50 + i * 100;
        float r = (selectedMenuItem == i) ? 1.0f : 0.5f;
        float g = (selectedMenuItem == i) ? 1.0f : 0.5f;
        float b = (selectedMenuItem == i) ? 1.0f : 0.5f;

        if (selectedMenuItem == i) {
            render_text(">", centerX - 220, y, 1.5f, 1.0f, 1.0f, 1.0f);
        }
        render_text(items[i], centerX - 170, y, 1.5f, r, g, b);
    }

    render_text("W/S - Navigate     SPACE - Select", centerX - 280, centerY + 400, 0.8f, 0.7f, 0.7f, 0.7f);
}

// ------------------ Отрисовка выбора уровня ------------------
void render_level_select(void) {
    float centerX = windowWidth / 2;
    float centerY = windowHeight / 2;

    render_text("SELECT LEVEL", centerX - 160, centerY - 350, 2.0f, 1.0f, 0.8f, 0.2f);

    char level_text[50];
    for (int i = 0; i < total_levels; i++) {
        float y = centerY - 50 + i * 100;
        float r = (selectedLevel == i) ? 1.0f : 0.5f;
        float g = (selectedLevel == i) ? 1.0f : 0.5f;
        float b = (selectedLevel == i) ? 1.0f : 0.5f;

        sprintf(level_text, "LEVEL %d", i + 1);

        if (selectedLevel == i) {
            render_text(">", centerX - 150, y, 1.5f, 1.0f, 1.0f, 1.0f);
        }
        render_text(level_text, centerX - 100, y, 1.5f, r, g, b);
    }

    float y = centerY + 200;
    float r = (selectedLevel == total_levels) ? 1.0f : 0.5f;
    float g = (selectedLevel == total_levels) ? 1.0f : 0.5f;
    float b = (selectedLevel == total_levels) ? 1.0f : 0.5f;

    if (selectedLevel == total_levels) {
        render_text(">", centerX - 150, y, 1.5f, 1.0f, 1.0f, 1.0f);
    }
    render_text("BACK", centerX - 100, y, 1.5f, r, g, b);

    render_text("W/S - Navigate     SPACE - Select", centerX - 280, centerY + 400, 0.8f, 0.7f, 0.7f, 0.7f);
}

// ------------------ Отрисовка Game Over ------------------
void render_game_over_screen(void) {
    float centerX = windowWidth / 2;
    float centerY = windowHeight / 2;

    render_text("GAME OVER", centerX - 50, centerY - 10, 2.5f, 1.0f, 0.2f, 0.2f);
    render_text(gameOverMessages[gameOverMessageIndex], centerX - 200, centerY - 30, 1.3f, 1.0f, 0.8f, 0.2f);

    int remaining = (int)(4.0 - (glfwGetTime() - gameOverTimer));
    if (remaining < 0) remaining = 0;
    render_text("PRESS SPACE TO SKIP", centerX - 160, centerY + 180, 0.7f, 0.5f, 0.5f, 0.5f);
}

// ------------------ Отрисовка меню после смерти ------------------
void render_death_menu(void) {
    float centerX = windowWidth / 2;
    float centerY = windowHeight / 2;

    render_text("TANK DESTROYED", centerX - 170, centerY - 200, 1.8f, 1.0f, 0.3f, 0.3f);
    render_text(gameOverMessages[gameOverMessageIndex], centerX - 200, centerY - 100, 1.2f, 1.0f, 0.8f, 0.2f);

    const char* items[] = { "PLAY AGAIN", "MAIN MENU" };
    for (int i = 0; i < 2; i++) {
        float y = centerY + 20 + i * 80;
        float r = (selectedMenuItem == i) ? 1.0f : 0.5f;
        float g = (selectedMenuItem == i) ? 1.0f : 0.5f;
        float b = (selectedMenuItem == i) ? 1.0f : 0.5f;

        if (selectedMenuItem == i) {
            render_text(">", centerX - 150, y, 1.2f, 1.0f, 1.0f, 1.0f);
        }
        render_text(items[i], centerX - 110, y, 1.2f, r, g, b);
    }

    render_text("W/S - Navigate     SPACE - Select", centerX - 280, centerY + 220, 0.7f, 0.5f, 0.5f, 0.5f);
}

// ------------------ Загрузка уровня ------------------
void load_level(int level_index) {
    // Инициализация уровня 1
    int level1_map[GRID_SIZE][GRID_SIZE] = {
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
        {0,0,0,0,0,0,1,0,0,0,0,0,0},
    };

    memcpy(map, level1_map, sizeof(int) * GRID_SIZE * GRID_SIZE);

    // Поиск спавна игрока
    int found = 0;
    for (int j = 0; j < GRID_SIZE; j++) {
        for (int i = 0; i < GRID_SIZE; i++) {
            if (map[j][i] == 1) {
                sp_player.x = fieldX + i * CELL_SIZE + CELL_SIZE / 2.0f;
                sp_player.y = fieldY + j * CELL_SIZE + CELL_SIZE / 2.0f;
                found = 1;
                break;
            }
        }
        if (found) break;
    }
    if (!found) {
        sp_player.x = fieldX + FIELD_SIZE / 2.0f;
        sp_player.y = fieldY + FIELD_SIZE / 2.0f;
    }

    player.x = sp_player.x;
    player.y = sp_player.y;
    player.dead = 0;
    player.lives = 3;
    player.invincibleTimer = 0.0f;
    player.p_bullet.active = 0;
    player.p_bullet.dirX = 0.0f;
    player.p_bullet.dirY = -1.0f;
    player.p_bullet.lastShootTime = 0.0;

    // Инициализация спавнеров ботов
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

    // Инициализация ботов
    for (int i = 0; i < MAX_BOTS; i++) {
        bots[i].active = 0;
        bots[i].deathTime = 0;
        bots[i].b_bullet.active = 0;
    }

    // Инициализация деревьев
    for (int j = 0; j < GRID_SIZE; j++) {
        for (int i = 0; i < GRID_SIZE; i++) {
            if (map[j][i] == 5) {
                woods[j][i].width = 64;
                woods[j][i].height = 64;
                woods[j][i].x = fieldX + i * CELL_SIZE + CELL_SIZE / 2.0f;
                woods[j][i].y = fieldY + j * CELL_SIZE + CELL_SIZE / 2.0f;
            }
        }
    }
}

// ------------------ Обновление проекции и поля ------------------
void update_projection_and_field(GLuint projLoc, int width, int height) {
    int oldFieldX = fieldX;
    int oldFieldY = fieldY;

    windowWidth = width;
    windowHeight = height;
    ortho_projection(0.0f, (float)windowWidth, (float)windowHeight, 0.0f, projectionMatrix);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, projectionMatrix);

    if (shaderProgramText) {
        glUseProgram(shaderProgramText);
        glUniformMatrix4fv(projLocText, 1, GL_FALSE, projectionMatrix);
    }

    fieldX = (windowWidth - FIELD_SIZE) / 2;
    fieldY = (windowHeight - FIELD_SIZE) / 2;
    outerX = fieldX - BORDER_WIDTH;
    outerY = fieldY - BORDER_WIDTH;

    // Пересчет позиции игрока
    float offsetX = player.x - oldFieldX;
    float offsetY = player.y - oldFieldY;
    player.x = fieldX + offsetX;
    player.y = fieldY + offsetY;

    // Пересчет деревьев
    for (int j = 0; j < GRID_SIZE; j++) {
        for (int i = 0; i < GRID_SIZE; i++) {
            if (map[j][i] == 5) {
                float oldCenterX = oldFieldX + i * CELL_SIZE + CELL_SIZE / 2;
                float oldCenterY = oldFieldY + j * CELL_SIZE + CELL_SIZE / 2;
                float offsetX_tree = woods[j][i].x - oldCenterX;
                float offsetY_tree = woods[j][i].y - oldCenterY;
                float newCenterX = fieldX + i * CELL_SIZE + CELL_SIZE / 2;
                float newCenterY = fieldY + j * CELL_SIZE + CELL_SIZE / 2;
                woods[j][i].x = newCenterX + offsetX_tree;
                woods[j][i].y = newCenterY + offsetY_tree;
            }
        }
    }
}

// ------------------ Callback изменения размера окна ------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    GLuint projLoc = *(GLuint*)glfwGetWindowUserPointer(window);
    update_projection_and_field(projLoc, width, height);
}

// ------------------ Основная функция ------------------
int main(void) {
    srand((unsigned int)time(NULL));

    if (!glfwInit()) {
        fprintf(stderr, "Не удалось инициализировать GLFW\n");
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "ТАНКИ - БОЕВАЯ АРЕНА", NULL, NULL);
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

    // Основная шейдерная программа
    GLuint shaderProgram = create_program(vertexShaderSource, fragmentShaderSource);
    glUseProgram(shaderProgram);
    GLint projLoc = glGetUniformLocation(shaderProgram, "uProjection");
    GLint modelLoc = glGetUniformLocation(shaderProgram, "uModel");
    GLint colorLoc = glGetUniformLocation(shaderProgram, "uColor");

    // Шейдерная программа для текста
    shaderProgramText = create_program(textVertexShaderSource, textFragmentShaderSource);
    projLocText = glGetUniformLocation(shaderProgramText, "uProjection");
    textColorLoc = glGetUniformLocation(shaderProgramText, "textColor");

    glfwSetWindowUserPointer(window, &projLoc);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // Инициализация проекции
    ortho_projection(0.0f, (float)windowWidth, (float)windowHeight, 0.0f, projectionMatrix);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, projectionMatrix);

    fieldX = (windowWidth - FIELD_SIZE) / 2;
    fieldY = (windowHeight - FIELD_SIZE) / 2;
    outerX = fieldX - BORDER_WIDTH;
    outerY = fieldY - BORDER_WIDTH;

    // Инициализация шрифта
    init_font();

    // Инициализация VAO для текста
    glGenVertexArrays(1, &VAOText);
    glGenBuffers(1, &VBOText);
    glBindVertexArray(VAOText);
    glBindBuffer(GL_ARRAY_BUFFER, VBOText);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Создание VAO для квадратов
    float vertices[] = {
        -0.5f, -0.5f,
        0.5f, -0.5f,
        0.5f,  0.5f,
        -0.5f,  0.5f
    };
    unsigned int indices[] = { 0, 1, 2, 0, 2, 3 };
    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // Загрузка уровня
    load_level(0);

    // Цвета
    float black[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    float gray[4] = { 0.5f, 0.5f, 0.5f, 1.0f };
    float white[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    float orange[4] = { 0.8f, 0.4f, 0.0f, 1.0f };
    float red[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
    float green[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
    float dark_green[4] = { 0.0f, 0.7f, 0.0f, 0.8f };
    float blue[4] = { 0.0f, 0.2f, 1.0f, 1.0f };

    double lastTime = glfwGetTime();
    double lastKeyTime = 0;
    double keyDelay = 0.2;
    int once = 0;

    // Основной цикл
    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        float deltaTime = (float)(currentTime - lastTime);
        lastTime = currentTime;

        // Обработка ввода в меню
        if (gameState == GAME_STATE_MENU) {
            if (currentTime - lastKeyTime > keyDelay) {
                if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
                    selectedMenuItem = (selectedMenuItem - 1 + 2) % 2;
                    lastKeyTime = currentTime;
                }
                else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
                    selectedMenuItem = (selectedMenuItem + 1) % 2;
                    lastKeyTime = currentTime;
                }
                else if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
                    if (selectedMenuItem == 0) {
                        gameState = GAME_STATE_LEVEL_SELECT;
                        selectedLevel = 0;
                    }
                    else if (selectedMenuItem == 1) {
                        glfwSetWindowShouldClose(window, 1);
                    }
                    lastKeyTime = currentTime;
                }
            }
        }
        else if (gameState == GAME_STATE_LEVEL_SELECT) {
            if (currentTime - lastKeyTime > keyDelay) {
                if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
                    selectedLevel = (selectedLevel - 1 + (total_levels + 1)) % (total_levels + 1);
                    lastKeyTime = currentTime;
                }
                else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
                    selectedLevel = (selectedLevel + 1) % (total_levels + 1);
                    lastKeyTime = currentTime;
                }
                else if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
                    if (selectedLevel < total_levels) {
                        load_level(selectedLevel);
                        gameState = GAME_STATE_PLAYING;
                        lastTime = glfwGetTime();
                        once = 0;
                    }
                    else {
                        gameState = GAME_STATE_MENU;
                        selectedMenuItem = 0;
                    }
                    lastKeyTime = currentTime;
                }
            }
        }
        else if (gameState == GAME_STATE_GAME_OVER) {
         
            if (currentTime - gameOverTimer >= 4.0) {
                // Показываем меню выбора
                if (currentTime - lastKeyTime > keyDelay) {
                    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
                        selectedMenuItem = (selectedMenuItem - 1 + 2) % 2;
                        lastKeyTime = currentTime;
                    }
                    else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
                        selectedMenuItem = (selectedMenuItem + 1) % 2;
                        lastKeyTime = currentTime;
                    }
                    else if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
                        if (selectedMenuItem == 0) {
                            // Играть заново
                            load_level(selectedLevel);
                            gameState = GAME_STATE_PLAYING;
                            once = 0;
                        }
                        else {
                            // В главное меню
                            gameState = GAME_STATE_MENU;
                            selectedMenuItem = 0;
                        }
                        lastKeyTime = currentTime;
                    }
                }
            }

            // Пропуск ожидания по пробелу
            if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && currentTime - lastKeyTime > keyDelay) {
                gameOverTimer = glfwGetTime() - 4.0;
                lastKeyTime = currentTime;
            }
        }
        else if (gameState == GAME_STATE_PLAYING) {
            // Движение игрока
            if (!player.dead) {
                float dx = 0.0f, dy = 0.0f;
                if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) dx -= 1.0f;
                if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) dx += 1.0f;
                if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) dy -= 1.0f;
                if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) dy += 1.0f;

                if (dx != 0 && dy != 0) {
                    dx *= 0.7071f;
                    dy *= 0.7071f;
                }

                if (dx != 0 || dy != 0) {
                    if (!player.p_bullet.active) {
                        if (dx != 0) {
                            player.p_bullet.dirX = (dx > 0) ? 1 : -1;
                            player.p_bullet.dirY = 0;
                        }
                        else if (dy != 0) {
                            player.p_bullet.dirX = 0;
                            player.p_bullet.dirY = (dy > 0) ? 1 : -1;
                        }
                    }

                    float newX = player.x + dx * PLAYER_SPEED * deltaTime;
                    if (!check_rect_collision_with_map(1, newX, player.y, PLAYER_SIZE, PLAYER_SIZE, 0, 0)) {
                        player.x = newX;
                    }
                    float newY = player.y + dy * PLAYER_SPEED * deltaTime;
                    if (!check_rect_collision_with_map(1, player.x, newY, PLAYER_SIZE, PLAYER_SIZE, 0, 0)) {
                        player.y = newY;
                    }
                }
            }

            float half = PLAYER_SIZE / 2.0f;
            float minX = fieldX + half;
            float maxX = fieldX + FIELD_SIZE - half;
            float minY = fieldY + half;
            float maxY = fieldY + FIELD_SIZE - half;
            player.x = fmaxf(minX, fminf(maxX, player.x));
            player.y = fmaxf(minY, fminf(maxY, player.y));

            if (player.invincibleTimer > 0.0f) {
                player.invincibleTimer -= deltaTime;
                if (player.invincibleTimer < 0.0f) player.invincibleTimer = 0.0f;
            }

            // Стрельба игрока
            if (!player.dead && glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
                if (currentTime - player.p_bullet.lastShootTime >= PLAYER_SHOOT_DELAY && !player.p_bullet.active) {
                    player.p_bullet.active = 1;
                    if (player.p_bullet.dirX != 0) {
                        player.p_bullet.x = player.x + player.p_bullet.dirX * PLAYER_SIZE / 2.0f;
                        player.p_bullet.y = player.y;
                    }
                    else {
                        player.p_bullet.x = player.x;
                        player.p_bullet.y = player.y + player.p_bullet.dirY * PLAYER_SIZE / 2.0f;
                    }
                    player.p_bullet.lastShootTime = currentTime;
                }
            }

            // Обновление пули игрока
            if (player.p_bullet.active) {
                player.p_bullet.x += player.p_bullet.dirX * BULLET_SPEED * deltaTime;
                player.p_bullet.y += player.p_bullet.dirY * BULLET_SPEED * deltaTime;

                float halfW = BULLET_WIDTH / 2.0f;
                float halfH = BULLET_HEIGHT / 2.0f;
                float minXb = fieldX + halfW;
                float maxXb = fieldX + FIELD_SIZE - halfW;
                float minYb = fieldY + halfH;
                float maxYb = fieldY + FIELD_SIZE - halfH;

                if (player.p_bullet.x < minXb || player.p_bullet.x > maxXb ||
                    player.p_bullet.y < minYb || player.p_bullet.y > maxYb) {
                    player.p_bullet.active = 0;
                }

                if (player.p_bullet.active) {
                    float w = (player.p_bullet.dirX != 0) ? BULLET_WIDTH : BULLET_HEIGHT;
                    float h = (player.p_bullet.dirX != 0) ? BULLET_HEIGHT : BULLET_WIDTH;
                    if (check_rect_collision_with_map(2, player.p_bullet.x, player.p_bullet.y, w, h,
                        player.p_bullet.dirX, player.p_bullet.dirY)) {
                        player.p_bullet.active = 0;
                    }
                }
            }

            // Спавн ботов
            static double nextSpawn = 0;
            static int spawnIndex = 0;
            int spawn_count = 0;
            for (int j = 0; j < GRID_SIZE; j++) {
                for (int i = 0; i < GRID_SIZE; i++) {
                    if (map[j][i] == 6) spawn_count++;
                }
            }

            int activeCount = 0;
            for (int i = 0; i < MAX_BOTS; i++) {
                if (bots[i].active) activeCount++;
            }

            if (activeCount < MAX_BOTS && spawn_count > 0 && currentTime >= nextSpawn) {
                for (int i = 0; i < MAX_BOTS; i++) {
                    if (!bots[i].active && (bots[i].deathTime == 0 || currentTime - bots[i].deathTime >= BOT_SPAWN_INTERVAL)) {
                        int idx = 0;
                        for (int j = 0; j < GRID_SIZE && idx <= spawnIndex; j++) {
                            for (int k = 0; k < GRID_SIZE && idx <= spawnIndex; k++) {
                                if (map[j][k] == 6) {
                                    if (idx == spawnIndex) {
                                        bots[i].x = fieldX + k * CELL_SIZE + CELL_SIZE / 2.0f;
                                        bots[i].y = fieldY + j * CELL_SIZE + CELL_SIZE / 2.0f;
                                    }
                                    idx++;
                                }
                            }
                        }

                        int dir = rand() % 4;
                        switch (dir) {
                        case 0: bots[i].dirX = 1.0f; bots[i].dirY = 0.0f; break;
                        case 1: bots[i].dirX = -1.0f; bots[i].dirY = 0.0f; break;
                        case 2: bots[i].dirX = 0.0f; bots[i].dirY = -1.0f; break;
                        case 3: bots[i].dirX = 0.0f; bots[i].dirY = 1.0f; break;
                        }

                        bots[i].active = 1;
                        bots[i].b_bullet.active = 0;
                        bots[i].b_bullet.dirX = bots[i].dirX;
                        bots[i].b_bullet.dirY = bots[i].dirY;
                        bots[i].b_bullet.lastShootTime = currentTime;
                        bots[i].nextRotateTime = currentTime + BOT_ROTATE_INTERVAL;
                        bots[i].deathTime = 0.0;
                        bots[i].invincibleTimer = INVINCIBLE_DURATION;

                        spawnIndex = (spawnIndex + 1) % spawn_count;
                        nextSpawn = currentTime + BOT_SPAWN_INTERVAL;
                        break;
                    }
                }
            }

            // Движение ботов
            for (int i = 0; i < MAX_BOTS; i++) {
                if (!bots[i].active) continue;

                if (currentTime >= bots[i].nextRotateTime) {
                    float dx = player.x - bots[i].x;
                    float dy = player.y - bots[i].y;
                    float desiredDirX = 0.0f, desiredDirY = 0.0f;
                    if (fabs(dx) > fabs(dy)) {
                        desiredDirX = (dx > 0) ? 1.0f : -1.0f;
                    }
                    else {
                        desiredDirY = (dy > 0) ? 1.0f : -1.0f;
                    }

                    int found = 0;
                    float bestDirX = 0, bestDirY = 0;
                    float dirs[4][2] = {
                        {desiredDirX, desiredDirY},
                        {desiredDirY, desiredDirX},
                        {-desiredDirY, -desiredDirX},
                        {-desiredDirX, -desiredDirY}
                    };

                    for (int d = 0; d < 4; d++) {
                        float testX = bots[i].x + dirs[d][0] * BOT_SPEED * deltaTime;
                        float testY = bots[i].y + dirs[d][1] * BOT_SPEED * deltaTime;
                        float halfB = BOT_SIZE / 2.0f;
                        float minXb = fieldX + halfB, maxXb = fieldX + FIELD_SIZE - halfB;
                        float minYb = fieldY + halfB, maxYb = fieldY + FIELD_SIZE - halfB;
                        if (testX < minXb || testX > maxXb || testY < minYb || testY > maxYb) continue;
                        if (!check_rect_collision_with_map(4, testX, testY, BOT_SIZE, BOT_SIZE, i, 0)) {
                            bestDirX = dirs[d][0];
                            bestDirY = dirs[d][1];
                            found = 1;
                            break;
                        }
                    }

                    if (found) {
                        bots[i].dirX = bestDirX;
                        bots[i].dirY = bestDirY;
                    }
                    bots[i].nextRotateTime = currentTime + BOT_ROTATE_INTERVAL;
                }

                float newX = bots[i].x + bots[i].dirX * BOT_SPEED * deltaTime;
                float newY = bots[i].y + bots[i].dirY * BOT_SPEED * deltaTime;
                float halfB = BOT_SIZE / 2.0f;
                float minXb = fieldX + halfB, maxXb = fieldX + FIELD_SIZE - halfB;
                float minYb = fieldY + halfB, maxYb = fieldY + FIELD_SIZE - halfB;
                newX = fmaxf(minXb, fminf(maxXb, newX));
                newY = fmaxf(minYb, fminf(maxYb, newY));

                if (!check_rect_collision_with_map(4, newX, newY, BOT_SIZE, BOT_SIZE, i, 0)) {
                    bots[i].x = newX;
                    bots[i].y = newY;
                    if (!bots[i].b_bullet.active) {
                        bots[i].b_bullet.dirX = bots[i].dirX;
                        bots[i].b_bullet.dirY = bots[i].dirY;
                    }
                }

                if (bots[i].invincibleTimer > 0.0f) {
                    bots[i].invincibleTimer -= deltaTime;
                    if (bots[i].invincibleTimer < 0.0f) bots[i].invincibleTimer = 0.0f;
                }
            }

            // Стрельба ботов
            for (int i = 0; i < MAX_BOTS; i++) {
                if (bots[i].active && currentTime - bots[i].b_bullet.lastShootTime >= BOT_SHOOT_DELAY && !bots[i].b_bullet.active) {
                    bots[i].b_bullet.active = 1;
                    if (bots[i].b_bullet.dirX != 0) {
                        bots[i].b_bullet.x = bots[i].x + bots[i].b_bullet.dirX * BOT_SIZE / 2.0f;
                        bots[i].b_bullet.y = bots[i].y;
                    }
                    else {
                        bots[i].b_bullet.x = bots[i].x;
                        bots[i].b_bullet.y = bots[i].y + bots[i].b_bullet.dirY * BOT_SIZE / 2.0f;
                    }
                    bots[i].b_bullet.lastShootTime = currentTime;
                }
            }

            // Обновление пуль ботов
            for (int i = 0; i < MAX_BOTS; i++) {
                if (bots[i].b_bullet.active) {
                    bots[i].b_bullet.x += bots[i].b_bullet.dirX * BULLET_SPEED * deltaTime;
                    bots[i].b_bullet.y += bots[i].b_bullet.dirY * BULLET_SPEED * deltaTime;

                    float halfW = BULLET_WIDTH / 2.0f;
                    float halfH = BULLET_HEIGHT / 2.0f;
                    float minXb = fieldX + halfW;
                    float maxXb = fieldX + FIELD_SIZE - halfW;
                    float minYb = fieldY + halfH;
                    float maxYb = fieldY + FIELD_SIZE - halfH;

                    if (bots[i].b_bullet.x < minXb || bots[i].b_bullet.x > maxXb ||
                        bots[i].b_bullet.y < minYb || bots[i].b_bullet.y > maxYb) {
                        bots[i].b_bullet.active = 0;
                    }

                    if (bots[i].b_bullet.active) {
                        float w = (bots[i].b_bullet.dirX != 0) ? BULLET_WIDTH : BULLET_HEIGHT;
                        float h = (bots[i].b_bullet.dirX != 0) ? BULLET_HEIGHT : BULLET_WIDTH;
                        if (check_rect_collision_with_map(3, bots[i].b_bullet.x, bots[i].b_bullet.y, w, h,
                            bots[i].b_bullet.dirX, bots[i].b_bullet.dirY)) {
                            bots[i].b_bullet.active = 0;
                        }
                    }
                }
            }

            // Проверка победы
            int any_bot = 0;
            for (int i = 0; i < MAX_BOTS; i++) {
                if (bots[i].active) any_bot = 1;
            }
            if (!any_bot && spawn_count == 0) {
                gameState = GAME_STATE_MENU;
                selectedMenuItem = 0;
            }

            // Проверка смерти игрока
            if (player.dead && !once) {
                once = 1;
                gameOverMessageIndex = rand() % gameOverMessagesCount;
                gameOverTimer = glfwGetTime();
                gameState = GAME_STATE_GAME_OVER;
                selectedMenuItem = 0;
            }
        }

        // Отрисовка
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        if (gameState == GAME_STATE_PLAYING) {
            glUseProgram(shaderProgram);
            glBindVertexArray(VAO);

            // Обводка
            float model[16] = { 0 };
            model[0] = OUTER_SIZE; model[5] = OUTER_SIZE; model[10] = 1.0f; model[15] = 1.0f;
            model[12] = outerX + OUTER_SIZE / 2; model[13] = outerY + OUTER_SIZE / 2;
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, model);
            glUniform4fv(colorLoc, 1, gray);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            // Поле
            model[0] = FIELD_SIZE; model[5] = FIELD_SIZE;
            model[12] = fieldX + FIELD_SIZE / 2; model[13] = fieldY + FIELD_SIZE / 2;
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, model);
            glUniform4fv(colorLoc, 1, black);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            // Блоки
            for (int j = 0; j < GRID_SIZE; j++) {
                for (int i = 0; i < GRID_SIZE; i++) {
                    if (is_wall(i, j) || is_water(i, j)) {
                        model[0] = BLOCK_SIZE; model[5] = BLOCK_SIZE;
                        model[12] = fieldX + i * CELL_SIZE + CELL_SIZE / 2;
                        model[13] = fieldY + j * CELL_SIZE + CELL_SIZE / 2;
                        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, model);
                        glUniform4fv(colorLoc, 1, is_wall(i, j) ? gray : blue);
                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                    }
                }
            }

            // Деревья
            for (int j = 0; j < GRID_SIZE; j++) {
                for (int i = 0; i < GRID_SIZE; i++) {
                    if (is_wood(i, j)) {
                        model[0] = woods[j][i].width; model[5] = woods[j][i].height;
                        model[12] = woods[j][i].x; model[13] = woods[j][i].y;
                        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, model);
                        glUniform4fv(colorLoc, 1, orange);
                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                    }
                }
            }

           

            // Игрок
            if (!player.dead) {
                int draw = 1;
                if (player.invincibleTimer > 0.0f && fmod(glfwGetTime(), 0.2f) > 0.1f) draw = 0;
                if (draw) {
                    model[0] = PLAYER_SIZE; model[5] = PLAYER_SIZE;
                    model[12] = player.x; model[13] = player.y;
                    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, model);
                    glUniform4fv(colorLoc, 1, green);
                    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                }
            }
            

            // Боты
            for (int i = 0; i < MAX_BOTS; i++) {
                if (bots[i].active) {
                    int draw = 1;
                    if (bots[i].invincibleTimer > 0.0f && fmod(glfwGetTime(), 0.2f) > 0.1f) draw = 0;
                    if (draw) {
                        model[0] = BOT_SIZE; model[5] = BOT_SIZE;
                        model[12] = bots[i].x; model[13] = bots[i].y;
                        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, model);
                        glUniform4fv(colorLoc, 1, red);
                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                    }
                }
            }

            // Пуля игрока
            if (player.p_bullet.active) {
                if (player.p_bullet.dirX != 0) {
                    model[0] = BULLET_WIDTH; model[5] = BULLET_HEIGHT;
                }
                else {
                    model[0] = BULLET_HEIGHT; model[5] = BULLET_WIDTH;
                }
                model[12] = player.p_bullet.x; model[13] = player.p_bullet.y;
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, model);
                glUniform4fv(colorLoc, 1, white);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            }

            // Пули ботов
            for (int i = 0; i < MAX_BOTS; i++) {
                if (bots[i].b_bullet.active) {
                    if (bots[i].b_bullet.dirX != 0) {
                        model[0] = BULLET_WIDTH; model[5] = BULLET_HEIGHT;
                    }
                    else {
                        model[0] = BULLET_HEIGHT; model[5] = BULLET_WIDTH;
                    }
                    model[12] = bots[i].b_bullet.x; model[13] = bots[i].b_bullet.y;
                    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, model);
                    glUniform4fv(colorLoc, 1, white);
                    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                }
            }
            // Листва
            for (int j = 0; j < GRID_SIZE; j++) {
                for (int i = 0; i < GRID_SIZE; i++) {
                    if (is_foliage(i, j)) {
                        model[0] = BLOCK_SIZE; model[5] = BLOCK_SIZE;
                        model[12] = fieldX + i * CELL_SIZE + CELL_SIZE / 2;
                        model[13] = fieldY + j * CELL_SIZE + CELL_SIZE / 2;
                        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, model);
                        glUniform4fv(colorLoc, 1, dark_green);
                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                    }
                }
            }

            // UI
            char text[50];
            sprintf(text, "LIVES: %d", player.lives);
            render_text(text, 20, 40, 0.9f, 1.0f, 1.0f, 1.0f);

            int enemies = 0;
            for (int i = 0; i < MAX_BOTS; i++) if (bots[i].active) enemies++;
            sprintf(text, "ENEMIES: %d", enemies);
            render_text(text, 20, 90, 0.9f, 1.0f, 1.0f, 1.0f);
        }
        else if (gameState == GAME_STATE_MENU) {
            render_menu();
        }
        else if (gameState == GAME_STATE_LEVEL_SELECT) {
            render_level_select();
        }
        else if (gameState == GAME_STATE_GAME_OVER) {
            if (glfwGetTime() - gameOverTimer >= 4.0) {
                render_death_menu();
            }
            else {
                render_game_over_screen();
            }
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Очистка
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);
    if (shaderProgramText) glDeleteProgram(shaderProgramText);
    if (fontTexture) glDeleteTextures(1, &fontTexture);
    if (VAOText) glDeleteVertexArrays(1, &VAOText);
    if (VBOText) glDeleteBuffers(1, &VBOText);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
