#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

// ------------------ Константы ------------------
#define WINDOW_WIDTH_INIT 1920
#define WINDOW_HEIGHT_INIT 1080

const int FIELD_SIZE = 832;
const int BORDER_WIDTH = 4;
const int OUTER_SIZE = FIELD_SIZE + 2 * BORDER_WIDTH;

const int PLAYER_SIZE = 60;
const float PLAYER_SPEED = 250.0f;

const float BULLET_SPEED = 800.0f;
const float BULLET_WIDTH = 20.0f;
const float BULLET_HEIGHT = 10.0f;
const float SHOOT_DELAY = 0.4f;

const int WALL_SIZE = 64;

// ------------------ Параметры карты ------------------
#define GRID_SIZE 13
#define CELL_SIZE 64

// Карта: 0 - пусто, 1 - игрок, 2 - каменная стена
int map[GRID_SIZE][GRID_SIZE] = {
    {0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,1,0,0,0,0,0,0,0,0},
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

typedef struct {
    float x, y;
    float width, height;
} Wall;

// ------------------ Глобальные переменные ------------------
int windowWidth = WINDOW_WIDTH_INIT;
int windowHeight = WINDOW_HEIGHT_INIT;
int fieldX = 0, fieldY = 0;
int outerX = 0, outerY = 0;

Player player;
Bullet bullet;
Wall walls[GRID_SIZE * GRID_SIZE];
int wallCount = 0;

// ------------------ Прототипы функций ------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void update_projection_and_field(GLuint projLoc, int width, int height);
GLuint load_shader(const char *source, GLenum type);
GLuint create_program(const char *vertexSource, const char *fragmentSource);
void ortho_projection(float left, float right, float bottom, float top, float *matrix);
int rect_collision(float x1, float y1, float w1, float h1, float x2, float y2, float w2, float h2);
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

// ------------------ Проверка коллизии прямоугольников ------------------
int rect_collision(float x1, float y1, float w1, float h1, float x2, float y2, float w2, float h2) {
    return (x1 < x2 + w2 && x1 + w1 > x2 && y1 < y2 + h2 && y1 + h1 > y2);
}

// ------------------ Загрузка карты (создание стен и позиции игрока) ------------------
void load_map(void) {
    wallCount = 0;
    
    for (int j = 0; j < GRID_SIZE; j++) {
        for (int i = 0; i < GRID_SIZE; i++) {
            if (map[j][i] == 2) {
                // Стена
                walls[wallCount].width = WALL_SIZE;
                walls[wallCount].height = WALL_SIZE;
                walls[wallCount].x = fieldX + i * CELL_SIZE + CELL_SIZE / 2.0f;
                walls[wallCount].y = fieldY + j * CELL_SIZE + CELL_SIZE / 2.0f;
                wallCount++;
            } else if (map[j][i] == 1) {
                // Игрок
                player.x = fieldX + i * CELL_SIZE + CELL_SIZE / 2.0f;
                player.y = fieldY + j * CELL_SIZE + CELL_SIZE / 2.0f;
            }
        }
    }
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

    load_map();

    float half = PLAYER_SIZE / 2.0f;
    int collision = 0;
    for (int i = 0; i < wallCount; i++) {
        if (rect_collision(player.x - half, player.y - half, PLAYER_SIZE, PLAYER_SIZE,
                           walls[i].x - walls[i].width/2, walls[i].y - walls[i].height/2,
                           walls[i].width, walls[i].height)) {
            collision = 1;
            break;
        }
    }
    
    if (collision) {
        for (int j = 0; j < GRID_SIZE; j++) {
            for (int i = 0; i < GRID_SIZE; i++) {
                if (map[j][i] == 0) {
                    player.x = fieldX + i * CELL_SIZE + CELL_SIZE / 2.0f;
                    player.y = fieldY + j * CELL_SIZE + CELL_SIZE / 2.0f;
                    break;
                }
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

            float half = PLAYER_SIZE / 2.0f;
            // Движение по X
            float newX = player.x + dx * PLAYER_SPEED * deltaTime;
            int collideX = 0;
            for (int i = 0; i < wallCount; i++) {
                if (rect_collision(newX - half, player.y - half, PLAYER_SIZE, PLAYER_SIZE,
                                   walls[i].x - walls[i].width/2, walls[i].y - walls[i].height/2,
                                   walls[i].width, walls[i].height)) {
                    collideX = 1;
                    break;
                }
            }
            if (!collideX) {
                player.x = newX;
            }

            // Движение по Y
            float newY = player.y + dy * PLAYER_SPEED * deltaTime;
            int collideY = 0;
            for (int i = 0; i < wallCount; i++) {
                if (rect_collision(player.x - half, newY - half, PLAYER_SIZE, PLAYER_SIZE,
                                   walls[i].x - walls[i].width/2, walls[i].y - walls[i].height/2,
                                   walls[i].width, walls[i].height)) {
                    collideY = 1;
                    break;
                }
            }
            if (!collideY) {
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
                    } else if (lastDirY != 0.0f) {
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

            if (bullet.active) {
                int hitWall = 0;
                for (int i = 0; i < wallCount; i++) {
                    if (rect_collision(bullet.x - halfW, bullet.y - halfH, BULLET_WIDTH, BULLET_HEIGHT,
                                       walls[i].x - walls[i].width/2, walls[i].y - walls[i].height/2,
                                       walls[i].width, walls[i].height)) {
                        hitWall = 1;
                        break;
                    }
                }
                if (hitWall) {
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

        // Стены
        for (int i = 0; i < wallCount; i++) {
            float model[16] = {0};
            model[0] = walls[i].width;
            model[5] = walls[i].height;
            model[10] = 1.0f;
            model[15] = 1.0f;
            model[12] = walls[i].x;
            model[13] = walls[i].y;
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, model);
            glUniform4fv(colorLoc, 1, gray);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
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
