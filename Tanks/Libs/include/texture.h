#ifndef TEXTURE_H
#define TEXTURE_H

#include <glad/glad.h>

typedef struct {
    GLuint player;
    GLuint bot;
    GLuint bullet;
    GLuint wall;
    GLuint water;
    GLuint wood;
} Textures;

extern Textures gTextures;

void textures_load(void);

#endif // TEXTURE_H
