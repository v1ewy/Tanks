#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>   // для fminf, fmaxf

// Размеры окна (могут меняться)
int WINDOW_WIDTH = 1920;
int WINDOW_HEIGHT = 1080;

// Параметры игрового поля
const int FIELD_SIZE = 832;          // размер поля (внутренний)
const int BORDER_WIDTH = 4;          // ширина обводки в пикселях
const int OUTER_SIZE = FIELD_SIZE + 2 * BORDER_WIDTH; // 840

// Параметры игрока (квадрат)
const int SQUARE_SIZE = 64;
const float PLAYER_SPEED = 175.0f;   // пикселей в секунду

// Структура игрока
typedef struct {
    float x, y;   // координаты центра (пиксели, относительно окна)
} Player;

// Координаты поля (левая верхняя точка чёрного поля)
int fieldX = 0, fieldY = 0;
int outerX = 0, outerY = 0;

// Функция для загрузки и компиляции шейдера
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

// Функция для создания шейдерной программы
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

// Вершинный шейдер
const char *vertexShaderSource =
    "#version 330 core\n"
    "layout (location = 0) in vec2 aPos;\n"
    "uniform mat4 uProjection;\n"
    "uniform mat4 uModel;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = uProjection * uModel * vec4(aPos, 0.0, 1.0);\n"
    "}\n";

// Фрагментный шейдер
const char *fragmentShaderSource =
    "#version 330 core\n"
    "out vec4 FragColor;\n"
    "uniform vec4 uColor;\n"
    "void main()\n"
    "{\n"
    "   FragColor = uColor;\n"
    "}\n";

// Функция для вычисления ортографической проекции (пиксельные координаты -> NDC)
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

// Обновление проекции и координат поля при изменении размера окна
void update_projection_and_field(GLuint projLoc, int width, int height) {
    WINDOW_WIDTH = width;
    WINDOW_HEIGHT = height;
    float projection[16];
    ortho_projection(0.0f, (float)WINDOW_WIDTH, (float)WINDOW_HEIGHT, 0.0f, projection);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, projection);

    // Пересчёт координат поля (центрирование)
    fieldX = (WINDOW_WIDTH - FIELD_SIZE) / 2;
    fieldY = (WINDOW_HEIGHT - FIELD_SIZE) / 2;
    outerX = fieldX - BORDER_WIDTH;
    outerY = fieldY - BORDER_WIDTH;
}

// Callback изменения размера окна
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    // Получим uniform-переменную проекции (необходимо передать через user pointer или глобальную)
    GLuint projLoc = *(GLuint*)glfwGetWindowUserPointer(window);
    update_projection_and_field(projLoc, width, height);
}

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

    GLFWwindow *window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Танчики", NULL, NULL);
    if (!window) {
        fprintf(stderr, "Не удалось создать окно\n");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        fprintf(stderr, "Не удалось загрузить glad\n");
        return -1;
    }

    GLuint shaderProgram = create_program(vertexShaderSource, fragmentShaderSource);
    glUseProgram(shaderProgram);

    GLint projLoc = glGetUniformLocation(shaderProgram, "uProjection");
    GLint modelLoc = glGetUniformLocation(shaderProgram, "uModel");
    GLint colorLoc = glGetUniformLocation(shaderProgram, "uColor");

    // Передаём projLoc в callback через user pointer
    glfwSetWindowUserPointer(window, &projLoc);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // Начальная проекция и координаты поля
    update_projection_and_field(projLoc, WINDOW_WIDTH, WINDOW_HEIGHT);

    // Создание VAO, VBO для единичного квадрата
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

    // Цвета
    float black[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    float gray[4] = {0.5f, 0.5f, 0.5f, 1.0f};
    float green[4] = {0.0f, 1.0f, 0.0f, 1.0f};

    // Инициализация игрока в центре поля
    Player player;
    player.x = fieldX + FIELD_SIZE / 2.0f;
    player.y = fieldY + FIELD_SIZE / 2.0f;

    // Переменные для delta time
    double lastTime = glfwGetTime();

    // Основной цикл
    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        float deltaTime = (float)(currentTime - lastTime);
        lastTime = currentTime;

        // --- Обработка ввода ---
        float dx = 0.0f, dy = 0.0f;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) dx -= 1.0f;
        else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) dx += 1.0f;
        else if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) dy -= 1.0f;
        else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) dy += 1.0f;
        if (dx != 0.0f || dy != 0.0f) {
            // Нормализуем для диагонального движения
            float len = sqrtf(dx*dx + dy*dy);
            dx /= len;
            dy /= len;
            player.x += dx * PLAYER_SPEED * deltaTime;
            player.y += dy * PLAYER_SPEED * deltaTime;
        }

        // Ограничение движения границами поля (с учётом половины размера квадрата)
        float half = SQUARE_SIZE / 2.0f;
        float minX = fieldX + half;
        float maxX = fieldX + FIELD_SIZE - half;
        float minY = fieldY + half;
        float maxY = fieldY + FIELD_SIZE - half;
        player.x = fmaxf(minX, fminf(maxX, player.x));
        player.y = fmaxf(minY, fminf(maxY, player.y));

        // --- Отрисовка ---
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);

        // Обводка (серый)
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

        // Поле (чёрный)
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

        // Игрок (зелёный квадрат)
        {
            float model[16] = {0};
            model[0] = SQUARE_SIZE;
            model[5] = SQUARE_SIZE;
            model[10] = 1.0f;
            model[15] = 1.0f;
            model[12] = player.x;
            model[13] = player.y;
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, model);
            glUniform4fv(colorLoc, 1, green);
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
