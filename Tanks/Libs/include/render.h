#ifndef RENDER_H
#define RENDER_H

#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

// Контекст рендера — передаём один раз при инициализации
typedef struct {
    GLuint shaderProgram;
    GLuint VAO;
    GLint  modelLoc;
    GLint  colorLoc;
    GLint  projLoc;

    // Текстовый рендер
    GLuint shaderProgramText;
    GLuint VAOText;
    GLuint VBOText;
    GLint  projLocText;
    GLint  textColorLoc;
    GLuint fontTexture;
} RenderContext;

extern RenderContext gRender;

void render_init(GLuint shaderProgram, GLuint VAO,
                 GLint modelLoc, GLint colorLoc, GLint projLoc,
                 GLuint shaderProgramText, GLuint VAOText, GLuint VBOText,
                 GLint projLocText, GLint textColorLoc, GLuint fontTexture);

// Базовые примитивы
void draw_rect(float x, float y, float w, float h, float* color);
void draw_rect_with_label(float x, float y, float w, float h,
                          float* color, const char* label,
                          float lr, float lg, float lb);

// Отрисовка игровых объектов
void render_map(void);
void render_player(void);
void render_bots(void);
void render_foliage(void); 

// UI во время игры
void render_hud(void);

#endif // RENDER_H
