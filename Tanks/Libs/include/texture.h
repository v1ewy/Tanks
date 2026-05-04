#ifndef TEXTURE_H
#define TEXTURE_H

#include <glad/glad.h>

typedef struct {
    GLuint player;
    GLuint bot_normal;
    GLuint bot_hound;
    GLuint bot_hunter;
    GLuint bot_armored;
    GLuint bullet;
    GLuint wall;
    GLuint water;
    GLuint wood;
    GLint foliage;
} Textures;

extern Textures gTextures;

void textures_load(void);

#endif // TEXTURE_H
