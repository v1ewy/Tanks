#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>

#define GL_CHECK(expr) expr

// Размеры окна
const int WINDOW_WIDTH = 1920;
const int WINDOW_HEIGHT = 1080;

// Параметры игрового поля
const int FIELD_SIZE = 832;          // размер поля (внутренний)
const int BORDER_WIDTH = 4;          // ширина обводки в пикселях
const int OUTER_SIZE = FIELD_SIZE + 2 * BORDER_WIDTH; // 424

// --- ДОБАВЛЕНО: размер квадрата в центре ---
const int SQUARE_SIZE = 64;

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

    // Проекционная матрица (пиксельные координаты)
    float projection[16];
    ortho_projection(0.0f, (float)WINDOW_WIDTH, (float)WINDOW_HEIGHT, 0.0f, projection);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, projection);

    // Координаты поля
    int fieldX = (WINDOW_WIDTH - FIELD_SIZE) / 2;
    int fieldY = (WINDOW_HEIGHT - FIELD_SIZE) / 2;
    int outerX = fieldX - BORDER_WIDTH;
    int outerY = fieldY - BORDER_WIDTH;

    float black[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    float gray[4] = {0.5f, 0.5f, 0.5f, 1.0f};

    // --- Цвет для квадрата ---
    float green[4] = {0.0f, 1.0f, 0.0f, 1.0f};

    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);

        // Отрисовка серого внешнего прямоугольника (обводка)
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

        // Отрисовка чёрного внутреннего прямоугольника (поле)
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

        // --- ДОБАВЛЕНО: отрисовка зелёного квадрата в центре поля ---
        {
            float model[16] = {0};
            float cx = fieldX + FIELD_SIZE / 2.0f;
            float cy = fieldY + FIELD_SIZE / 2.0f;
            model[0] = SQUARE_SIZE;
            model[5] = SQUARE_SIZE;
            model[10] = 1.0f;
            model[15] = 1.0f;
            model[12] = cx;
            model[13] = cy;
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, model);
            glUniform4fv(colorLoc, 1, green);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
