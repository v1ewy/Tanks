#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <texture.h>

Textures gTextures;

static GLuint load_texture(const char* path)
{
    GLuint id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);

    // Параметры фильтрации
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    int width, height, channels;
    stbi_set_flip_vertically_on_load(1);
    unsigned char* data = stbi_load(path, &width, &height, &channels, 4);
    if (!data) {
        fprintf(stderr, "Не удалось загрузить текстуру: %s\n", path);
        return 0;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, data);

    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);
    return id;
}

void textures_load(void)
{
    gTextures.player = load_texture("Assets/Image/Player.png");
    gTextures.bot_normal  = load_texture("Assets/Image/Normal.png");
    gTextures.bot_hound   = load_texture("Assets/Image/Hound.png");
    gTextures.bot_hunter  = load_texture("Assets/Image/Hunter.png");
    gTextures.bot_armored = load_texture("Assets/Image/Armored.png");
    gTextures.bullet = load_texture("Assets/Image/Bullet.png");
    gTextures.wall   = load_texture("Assets/Image/Wall.png");
    gTextures.base   = load_texture("Assets/Image/Base.png");
    gTextures.water  = load_texture("Assets/Image/Water.png");
    gTextures.wood   = load_texture("Assets/Image/Wood.png");
    gTextures.foliage   = load_texture("Assets/Image/Foliage.png");
}
