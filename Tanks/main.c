#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

// ------------------ Константы ------------------
#define WINDOW_WIDTH_INIT 1920
#define WINDOW_HEIGHT_INIT 1080

#define FIELD_SIZE 832          // 13 * 64
#define BORDER_WIDTH 4
#define OUTER_SIZE (FIELD_SIZE + 2 * BORDER_WIDTH) // 840

#define PLAYER_SIZE 56
#define PLAYER_SPEED 200.0f

#define BULLET_SPEED 800.0f
#define BULLET_WIDTH 20.0f
#define BULLET_HEIGHT 10.0f
#define SHOOT_DELAY 0.4f

#define BLOCK_SIZE 64

// ------------------ Параметры карты ------------------
#define GRID_SIZE 13               // 13x13 клеток
#define CELL_SIZE 64               // размер клетки в пикселях

// Карта: 0 - пусто, 1 - игрок, 2 - каменная стена, 3 - вода, 4 - листва
int map[GRID_SIZE][GRID_SIZE] = {
    // Пример: стены по краям, игрок внизу слева
    {0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,1,2,3,4,0,0,0,0,0,0,0},
    {0,0,0,2,3,4,0,0,0,0,0,0,0},
    {0,0,0,2,3,4,0,0,0,0,0,0,0},
    {0,0,0,2,3,4,0,0,0,0,0,0,0},
    {0,0,0,2,3,4,0,0,0,0,0,0,0},
    {0,0,0,2,3,4,0,0,0,0,0,0,0},
    {0,0,0,2,3,4,0,0,0,0,0,0,0},
    {0,0,0,2,3,4,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0},
};

// ------------------ Структуры ------------------
typedef struct {
    float x, y;
} Player;

typedef struct {
    float x, y;
    float dirX, dirY;
    int active;
} Bullet;

// ------------------ Глобальные переменные ------------------
int windowWidth = WINDOW_WIDTH_INIT;
int windowHeight = WINDOW_HEIGHT_INIT;
int fieldX = 0, fieldY = 0;
int outerX = 0, outerY = 0;

Player player;
Bullet bullet;

// ------------------ Прототипы функций ------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void update_projection_and_field(GLuint projLoc, int width, int height);
GLuint load_shader(const char *source, GLenum type);
GLuint create_program(const char *vertexSource, const char *fragmentSource);
void ortho_projection(float left, float right, float bottom, float top, float *matrix);
int rect_collision(float x1, float y1, float w1, float h1, float x2, float y2, float w2, float h2);
int is_wall(int x, int y);
int is_water(int x, int y);
int is_foliage(int x, int y);
int check_rect_collision_with_map(int who, float cx, float cy, float w, float h);
int is_player_over_cell(int cellX, int cellY);
void load_map(void);

// ------------------ Шейдеры ------------------
const char *vertexShaderSource =
    "#version 330 core\n"
    "layout (location = 0) in vec2 aPos;\n"
    "uniform mat4 uProjection;\n"
    "uniform mat4 uModel;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = uProjection * uModel * vec4(aPos, 0.0, 1.0);\n"
    "}\n";

const char *fragmentShaderSource =
    "#version 330 core\n"
    "out vec4 FragColor;\n"
    "uniform vec4 uColor;\n"
    "void main()\n"
    "{\n"
    "   FragColor = uColor;\n"
    "}\n";

// ------------------ Функции работы с шейдерами ------------------
GLuint load_shader(const char *source, GLenum type) {
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

GLuint create_program(const char *vertexSource, const char *fragmentSource) {
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
void ortho_projection(float left, float right, float bottom, float top, float *matrix) {
    matrix[0] = 2.0f / (right - left);
    matrix[5] = 2.0f / (top - bottom);
    matrix[10] = -1.0f;
    matrix[12] = -(right + left) / (right - left);
    matrix[13] = -(top + bottom) / (top - bottom);
    matrix[15] = 1.0f;
    for (int i = 0; i < 16; ++i) {
        if (i == 0 || i == 5 || i == 10 || i == 12 || i == 13 || i == 15) continue;
        matrix[i] = 0.0f;
    }
}

int is_wall(int x, int y) {
    if (x < 0 || x >= GRID_SIZE || y < 0 || y >= GRID_SIZE) return 1;
    return map[y][x] == 2;
}

int is_water(int x, int y) {
    return map[y][x] == 3;
}

int is_foliage(int x, int y) {
    return map[y][x] == 4;
}

// ------------------ Проверка коллизии ------------------
int check_rect_collision_with_map(int who, float cx, float cy, float w, float h) {
    float halfW = w / 2.0f;
    float halfH = h / 2.0f;
    int left   = (int)((cx - halfW - fieldX) / CELL_SIZE);
    int right  = (int)((cx + halfW - fieldX) / CELL_SIZE);
    int top    = (int)((cy - halfH - fieldY) / CELL_SIZE);
    int bottom = (int)((cy + halfH - fieldY) / CELL_SIZE);
    
    for (int j = top; j <= bottom; j++) {
        for (int i = left; i <= right; i++) {
            switch (who) {
                case 1:
                    if (is_wall(i, j) || is_water(i, j)) return 1;
                    break;
                case 2:
                    if (is_wall(i, j)) return 1;
            }
        }
    }
    return 0;
}

// ------------------ Загрузка карты (установка игрока по map) ------------------
void load_map(void) {
    // Находим клетку с игроком (значение 1)
    int found = 0;
    for (int j = 0; j < GRID_SIZE; j++) {
        for (int i = 0; i < GRID_SIZE; i++) {
            if (map[j][i] == 1) {
                player.x = fieldX + i * CELL_SIZE + CELL_SIZE / 2.0f;
                player.y = fieldY + j * CELL_SIZE + CELL_SIZE / 2.0f;
                found = 1;
                break;
            }
        }
        if (found) break;
    }
    if (!found) {
        // Если игрока нет на карте, ставим в центр
        player.x = fieldX + FIELD_SIZE / 2.0f;
        player.y = fieldY + FIELD_SIZE / 2.0f;
    }
    
    // Ограничиваем игрока границами поля (на случай, если координаты вышли за пределы)
    float half = PLAYER_SIZE / 2.0f;
    float minX = fieldX + half;
    float maxX = fieldX + FIELD_SIZE - half;
    float minY = fieldY + half;
    float maxY = fieldY + FIELD_SIZE - half;
    player.x = fmaxf(minX, fminf(maxX, player.x));
    player.y = fmaxf(minY, fminf(maxY, player.y));
}

// ------------------ Обновление проекции и поля при изменении размера окна ------------------
void update_projection_and_field(GLuint projLoc, int width, int height) {
    windowWidth = width;
    windowHeight = height;
    float projection[16];
    ortho_projection(0.0f, (float)windowWidth, (float)windowHeight, 0.0f, projection);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, projection);

    fieldX = (windowWidth - FIELD_SIZE) / 2;
    fieldY = (windowHeight - FIELD_SIZE) / 2;
    outerX = fieldX - BORDER_WIDTH;
    outerY = fieldY - BORDER_WIDTH;

    // Перезагружаем карту
    load_map();
}

// ------------------ Callback изменения размера окна ------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    GLuint projLoc = *(GLuint*)glfwGetWindowUserPointer(window);
    update_projection_and_field(projLoc, width, height);
}

// ------------------ Основная функция ------------------
int main(void) {
    // Инициализация GLFW
    if (!glfwInit()) {
        fprintf(stderr, "Не удалось инициализировать GLFW\n");
        return -1;
    }
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow *window = glfwCreateWindow(windowWidth, windowHeight, "Танчики", NULL, NULL);
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

    GLuint shaderProgram = create_program(vertexShaderSource, fragmentShaderSource);
    glUseProgram(shaderProgram);

    GLint projLoc = glGetUniformLocation(shaderProgram, "uProjection");
    GLint modelLoc = glGetUniformLocation(shaderProgram, "uModel");
    GLint colorLoc = glGetUniformLocation(shaderProgram, "uColor");

    glfwSetWindowUserPointer(window, &projLoc);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // Начальная настройка проекции и координат поля (вызовет load_map)
    update_projection_and_field(projLoc, windowWidth, windowHeight);

    // ------------------ Создание VAO для квадрата ------------------
    float vertices[] = {
        -0.5f, -0.5f,
         0.5f, -0.5f,
         0.5f,  0.5f,
        -0.5f,  0.5f
    };
    unsigned int indices[] = {
        0, 1, 2,
        0, 2, 3
    };
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
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // ------------------ Цвета ------------------
    float black[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    float gray[4] = {0.5f, 0.5f, 0.5f, 1.0f};
    float green[4] = {0.0f, 1.0f, 0.0f, 1.0f};
    float red[4] = {1.0f, 0.0f, 0.0f, 1.0f};
    float blue[4] = {0.0f, 0.0f, 1.0f, 1.0f};
    float dark_green[4] = {0.0f, 0.5f, 0.0f, 1.0f};

    // ------------------ Инициализация пули ------------------
    bullet.active = 0;
    float lastDirX = 0.0f, lastDirY = -1.0f;
    double lastShootTime = 0.0;

    // ------------------ Переменные для delta time ------------------
    double lastTime = glfwGetTime();

    // ------------------ Основной цикл ------------------
    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        float deltaTime = (float)(currentTime - lastTime);
        lastTime = currentTime;

        // ---------- Обработка движения игрока ----------
        float dx = 0.0f, dy = 0.0f;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) dx -= 1.0f;
        else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) dx += 1.0f;
        else if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) dy -= 1.0f;
        else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) dy += 1.0f;

        if (dx || dy) {
            lastDirX = dx;
            lastDirY = dy;

            // Движение по X
            float newX = player.x + dx * PLAYER_SPEED * deltaTime;
            if (!check_rect_collision_with_map(1, newX, player.y, PLAYER_SIZE, PLAYER_SIZE)) {
                player.x = newX;
            }
            // Движение по Y
            float newY = player.y + dy * PLAYER_SPEED * deltaTime;
            if (!check_rect_collision_with_map(1, player.x, newY, PLAYER_SIZE, PLAYER_SIZE)) {
                player.y = newY;
            }
        }

        // Ограничение игрока границами поля
        float half = PLAYER_SIZE / 2.0f;
        float minX = fieldX + half;
        float maxX = fieldX + FIELD_SIZE - half;
        float minY = fieldY + half;
        float maxY = fieldY + FIELD_SIZE - half;
        player.x = fmaxf(minX, fminf(maxX, player.x));
        player.y = fmaxf(minY, fminf(maxY, player.y));

        // ---------- Стрельба ----------
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
            if (currentTime - lastShootTime >= SHOOT_DELAY) {
                if (!bullet.active) {
                    bullet.active = 1;
                    if (lastDirX != 0.0f) {
                        bullet.x = player.x + lastDirX * PLAYER_SIZE / 2.0f;
                        bullet.y = player.y;
                        bullet.dirX = lastDirX;
                        bullet.dirY = 0.0f;
                    } else {
                        bullet.x = player.x;
                        bullet.y = player.y + lastDirY * PLAYER_SIZE / 2.0f;
                        bullet.dirX = 0.0f;
                        bullet.dirY = lastDirY;
                    }
                    lastShootTime = currentTime;
                }
            }
        }

        // ---------- Обновление пули ----------
        if (bullet.active) {
            bullet.x += bullet.dirX * BULLET_SPEED * deltaTime;
            bullet.y += bullet.dirY * BULLET_SPEED * deltaTime;

            // Проверка выхода за границы поля
            float halfW = BULLET_WIDTH / 2.0f;
            float halfH = BULLET_HEIGHT / 2.0f;
            float minXb = fieldX + halfW;
            float maxXb = fieldX + FIELD_SIZE - halfW;
            float minYb = fieldY + halfH;
            float maxYb = fieldY + FIELD_SIZE - halfH;

            if (bullet.x < minXb || bullet.x > maxXb ||
                bullet.y < minYb || bullet.y > maxYb) {
                bullet.active = 0;
            }

            // Проверка коллизии со стенами
            if (bullet.active) {
                float w, h;
                if (bullet.dirX != 0) {
                    w = BULLET_WIDTH;
                    h = BULLET_HEIGHT;
                } else {
                    w = BULLET_HEIGHT;
                    h = BULLET_WIDTH;
                }
                if (check_rect_collision_with_map(2, bullet.x, bullet.y, w, h)) {
                    bullet.active = 0;
                }
            }
        }

        // ---------- Отрисовка ----------
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);

        // Серая обводка
        {
            float model[16] = {0};
            float cx = outerX + OUTER_SIZE / 2.0f;
            float cy = outerY + OUTER_SIZE / 2.0f;
            model[0] = OUTER_SIZE;
            model[5] = OUTER_SIZE;
            model[10] = 1.0f;
            model[15] = 1.0f;
            model[12] = cx;
            model[13] = cy;
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, model);
            glUniform4fv(colorLoc, 1, gray);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }

        // Чёрное поле
        {
            float model[16] = {0};
            float cx = fieldX + FIELD_SIZE / 2.0f;
            float cy = fieldY + FIELD_SIZE / 2.0f;
            model[0] = FIELD_SIZE;
            model[5] = FIELD_SIZE;
            model[10] = 1.0f;
            model[15] = 1.0f;
            model[12] = cx;
            model[13] = cy;
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, model);
            glUniform4fv(colorLoc, 1, black);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }

        // Блоки
        for (int j = 0; j < GRID_SIZE; j++) {
            for (int i = 0; i < GRID_SIZE; i++) {
                float model[16] = {0};
                float cx = fieldX + i * CELL_SIZE + CELL_SIZE / 2.0f;
                float cy = fieldY + j * CELL_SIZE + CELL_SIZE / 2.0f;
                model[0] = BLOCK_SIZE;
                model[5] = BLOCK_SIZE;
                model[10] = 1.0f;
                model[15] = 1.0f;
                model[12] = cx;
                model[13] = cy;
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, model);
                if (is_wall(i, j)) {
                    glUniform4fv(colorLoc, 1, gray);
                    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                } else if (is_water(i, j)) {
                    glUniform4fv(colorLoc, 1, blue);
                    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                }
            }
        }

        // Игрок
        {
            float model[16] = {0};
            model[0] = PLAYER_SIZE;
            model[5] = PLAYER_SIZE;
            model[10] = 1.0f;
            model[15] = 1.0f;
            model[12] = player.x;
            model[13] = player.y;
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, model);
            glUniform4fv(colorLoc, 1, green);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }

        // Пуля
        if (bullet.active) {
            float model[16] = {0};
            if (bullet.dirX != 0) {
                model[0] = BULLET_WIDTH;
                model[5] = BULLET_HEIGHT;
            } else {
                model[0] = BULLET_HEIGHT;
                model[5] = BULLET_WIDTH;
            }
            model[10] = 1.0f;
            model[15] = 1.0f;
            model[12] = bullet.x;
            model[13] = bullet.y;
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, model);
            glUniform4fv(colorLoc, 1, red);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }
        
        // Листва
        for (int j = 0; j < GRID_SIZE; j++) {
            for (int i = 0; i < GRID_SIZE; i++) {
                if (is_foliage(i, j)) {
                    float model[16] = {0};
                    float cx = fieldX + i * CELL_SIZE + CELL_SIZE / 2.0f;
                    float cy = fieldY + j * CELL_SIZE + CELL_SIZE / 2.0f;
                    model[0] = BLOCK_SIZE;
                    model[5] = BLOCK_SIZE;
                    model[10] = 1.0f;
                    model[15] = 1.0f;
                    model[12] = cx;
                    model[13] = cy;
                    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, model);
                    glUniform4fv(colorLoc, 1, dark_green);
                    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                }
            }
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Очистка ресурсов
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
