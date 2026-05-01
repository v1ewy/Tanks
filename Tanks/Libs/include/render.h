#ifndef RENDER_H
#define RENDER_H

#include <glad/glad.h>

typedef struct {
    // Цветной шейдер (меню, рамка)
    GLuint shaderProgram;
    GLuint VAO;
    GLint  modelLoc;
    GLint  colorLoc;
    GLint  projLoc;

    // Текстурный шейдер
    GLuint texShaderProgram;
    GLint  texModelLoc;
    GLint  texProjLoc;

    // Текстовый рендер
    GLuint shaderProgramText;
    GLuint VAOText;
    GLuint VBOText;
    GLint  projLocText;
    GLint  textColorLoc;
    GLuint fontTexture;
} RenderContext;

extern RenderContext gRender;
extern float gPlayerAnimTimer;
extern int   gPlayerAnimFrame;
extern int gPlayerMoving;
extern float gPlayerDirX;
extern float gPlayerDirY;

void render_init(GLuint shaderProgram, GLuint VAO,
                 GLint modelLoc, GLint colorLoc, GLint projLoc,
                 GLuint texShaderProgram, GLint texModelLoc, GLint texProjLoc,
                 GLuint shaderProgramText, GLuint VAOText, GLuint VBOText,
                 GLint projLocText, GLint textColorLoc, GLuint fontTexture);

void draw_rect(float x, float y, float w, float h, float* color);
void draw_textured_rect(float x, float y, float w, float h, GLuint textureID);
void render_rotated_uv(float x, float y, float w, float h,
                       GLuint textureID,
                       float u0, float v0, float u1, float v1,
                       float angle);

void render_map(void);
void render_bullet(float x, float y, float w, float h,
                   float dirX, float dirY);
void render_foliage(void);
void render_player(void);
void render_base(void);
void render_bots(void);
void render_hud(void);

#endif // RENDER_H
