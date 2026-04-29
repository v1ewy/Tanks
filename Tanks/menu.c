#include <stdio.h>
#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <menu.h>

extern void render_text(const char*, float, float, float, float, float, float);
extern int  windowWidth, windowHeight;
extern int  selectedMenuItem;
extern int  selectedLevel;
extern int  total_levels;
extern double gameOverTimer;
extern int  gameOverMessageIndex;
extern const char* gameOverMessages[];

void render_menu(void)
{
    float cx = windowWidth  / 2.0f;
    float cy = windowHeight / 2.0f;

    render_text("TANKS GAME",  cx - 180, cy - 350, 2.5f, 1.0f, 0.8f, 0.2f);

    const char* items[] = { "START GAME", "EXIT" };
    for (int i = 0; i < 2; i++) {
        float y = cy - 50 + i * 100;
        float br = (selectedMenuItem == i) ? 1.0f : 0.5f;
        if (selectedMenuItem == i)
            render_text(">", cx - 220, y, 1.5f, 1.0f, 1.0f, 1.0f);
        render_text(items[i], cx - 170, y, 1.5f, br, br, br);
    }

    render_text("W/S - Navigate     SPACE - Select",
                cx - 280, cy + 400, 0.8f, 0.7f, 0.7f, 0.7f);
}

void render_level_select(void)
{
    float cx = windowWidth  / 2.0f;
    float cy = windowHeight / 2.0f;

    render_text("SELECT LEVEL", cx - 160, cy - 350, 2.0f, 1.0f, 0.8f, 0.2f);

    // Описания уровней
    const char* descriptions[] = {
        "LEVEL 1 - OPEN FIELD",
        "LEVEL 2 - URBAN COMBAT",
        "LEVEL 3 - FORTRESS"
    };

    for (int i = 0; i < total_levels; i++) {
        float y  = cy - 100 + i * 100;
        float br = (selectedLevel == i) ? 1.0f : 0.5f;

        if (selectedLevel == i)
            render_text(">", cx - 250, y, 1.5f, 1.0f, 1.0f, 1.0f);

        render_text(descriptions[i], cx - 200, y, 1.2f, br, br, br);
    }

    // Кнопка BACK
    float by = cy + 200;
    float br = (selectedLevel == total_levels) ? 1.0f : 0.5f;
    if (selectedLevel == total_levels)
        render_text(">", cx - 250, by, 1.5f, 1.0f, 1.0f, 1.0f);
    render_text("BACK", cx - 200, by, 1.5f, br, br, br);

    render_text("W/S - Navigate     SPACE - Select",
                cx - 280, cy + 400, 0.8f, 0.7f, 0.7f, 0.7f);
}

void render_game_over_screen(void)
{
    float cx = windowWidth  / 2.0f;
    float cy = windowHeight / 2.0f;

    render_text("GAME OVER",
                cx - 150, cy - 50, 2.5f, 1.0f, 0.2f, 0.2f);
    render_text(gameOverMessages[gameOverMessageIndex],
                cx - 200, cy + 60, 1.3f, 1.0f, 0.8f, 0.2f);
    render_text("PRESS SPACE TO SKIP",
                cx - 160, cy + 180, 0.7f, 0.5f, 0.5f, 0.5f);
}

void render_death_menu(void)
{
    float cx = windowWidth  / 2.0f;
    float cy = windowHeight / 2.0f;

    render_text("TANK DESTROYED",
                cx - 170, cy - 200, 1.8f, 1.0f, 0.3f, 0.3f);
    render_text(gameOverMessages[gameOverMessageIndex],
                cx - 200, cy - 100, 1.2f, 1.0f, 0.8f, 0.2f);

    const char* items[] = { "PLAY AGAIN", "MAIN MENU" };
    for (int i = 0; i < 2; i++) {
        float y  = cy + 20 + i * 80;
        float br = (selectedMenuItem == i) ? 1.0f : 0.5f;
        if (selectedMenuItem == i)
            render_text(">", cx - 150, y, 1.2f, 1.0f, 1.0f, 1.0f);
        render_text(items[i], cx - 110, y, 1.2f, br, br, br);
    }

    render_text("W/S - Navigate     SPACE - Select",
                cx - 280, cy + 220, 0.7f, 0.5f, 0.5f, 0.5f);
}
