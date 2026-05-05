#include <stdio.h>
#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <menu.h>
#include "levels.h"
#include "render.h"
#include "score.h"
#include "leaderboard.h"

extern void render_text(const char*, float, float, float, float, float, float);
extern int  windowWidth, windowHeight;
extern int  selectedMenuItem;
extern int  selectedLevel;
extern int  total_levels;
extern double gameOverTimer;
extern int  gameOverMessageIndex;
extern const char* gameOverMessages[];

// ── Главное меню ──────────────────────────────────────────────────────
void render_menu(void)
{
    float cx = windowWidth  / 2.0f;
    float cy = windowHeight / 2.0f;

    render_text("TANKS GAME",  cx - 350, cy - 225, 2.5f, 1.0f, 0.8f, 0.2f);

    const char* items[] = { "START GAME", "LEADERBOARD", "EXIT" };
    for (int i = 0; i < 3; i++) {
        float y = cy - 50 + i * 100;
        float br = (selectedMenuItem == i) ? 1.0f : 0.5f;
        if (selectedMenuItem == i)
        render_text(">", cx - 260, y, 1.2f, 1.0f, 1.0f, 1.0f);
        render_text(items[i], cx - 200, y, 1.2f, br, br, br);
    }

    render_text("W/S - Navigate     SPACE - Select",
                cx - 290, cy + 400, 0.8f, 0.3f, 0.3f, 0.3f);
}

// ── Выбор уровня ──────────────────────────────────────────────────────
void render_level_select(void)
{
    float cx = windowWidth  / 2.0f;
    float cy = windowHeight / 2.0f;

    render_text("SELECT LEVEL", cx - 400, cy - 250, 2.5f, 1.0f, 0.8f, 0.2f);

    const char* descriptions[] = {
        "LEVEL 1 - OPEN FIELD",
        "LEVEL 2 - URBAN COMBAT",
        "LEVEL 3 - FORTRESS"
    };

    for (int i = 0; i < total_levels; i++) {
        float y  = cy - 70 + i * 100;
        float br = (selectedLevel == i) ? 1.0f : 0.5f;
        if (selectedLevel == i)
            render_text(">", cx - 350, y, 1.2f, 1.0f, 1.0f, 1.0f);
        render_text(descriptions[i], cx - 290, y, 1.2f, br, br, br);
    }

    float by = cy + 220;
    float br = (selectedLevel == total_levels) ? 1.0f : 0.5f;
    if (selectedLevel == total_levels)
        render_text(">", cx - 350, by, 1.2f, 1.0f, 1.0f, 1.0f);
    render_text("BACK", cx - 290, by, 1.2f, br, br, br);

    render_text("W/S - Navigate     SPACE - Select",
                cx - 290, cy + 400, 0.8f, 0.3f, 0.3f, 0.3f);
}

// ── Game Over ─────────────────────────────────────────────────────────
void render_game_over_screen(void)
{
    float cx = windowWidth  / 2.0f;
    float cy = windowHeight / 2.0f;

    render_text("GAME OVER",
                cx - 315, cy - 50, 2.5f, 1.0f, 0.2f, 0.2f);
    render_text(gameOverMessages[gameOverMessageIndex],
                cx - 270, cy + 80, 1.3f, 1.0f, 0.8f, 0.2f);
    render_text("PRESS SPACE TO SKIP",
                cx - 215, cy + 180, 0.8f, 0.3f, 0.3f, 0.3f);
}

// ── Death menu ────────────────────────────────────────────────────────
void render_death_menu(void)
{
    float cx = windowWidth  / 2.0f;
    float cy = windowHeight / 2.0f;

    render_text("TANK DESTROYED",
                cx - 350, cy - 200, 1.8f, 1.0f, 0.3f, 0.3f);

    const char* items[] = { "PLAY AGAIN", "MAIN MENU" };
    for (int i = 0; i < 2; i++) {
        float y  = cy + 20 + i * 80;
        float br = (selectedMenuItem == i) ? 1.0f : 0.5f;
        if (selectedMenuItem == i)
            render_text(">", cx - 230, y, 1.2f, 1.0f, 1.0f, 1.0f);
        render_text(items[i], cx - 170, y, 1.2f, br, br, br);
    }

    render_text("W/S - Navigate     SPACE - Select",
                cx - 300, cy + 220, 0.8f, 0.3f, 0.3f, 0.3f);
}

// ── Victory ───────────────────────────────────────────────────────────
void render_victory_screen(void)
{
    extern int    gScore;
    extern int    gTimeBonusAwarded;
    extern int    gKills[4];
    extern int    currentLevelIndex;
    extern Level  levels[];

    float cx = windowWidth  / 2.0f;
    float cy = windowHeight / 2.0f;

    render_text("MISSION COMPLETE",
                cx - 475, cy - 375, 2.0f, 0.2f, 1.0f, 0.2f);

    Level* lvl = &levels[currentLevelIndex];
    int totalByType[4] = {0, 0, 0, 0};
    for (int i = 0; i < lvl->botCount; i++)
        totalByType[lvl->botQueue[i].type]++;

    const char* names[]  = { "NORMAL",  "HOUND",  "HUNTER", "ARMORED" };
    int points[] = { SCORE_NORMAL, SCORE_HOUND, SCORE_HUNTER, SCORE_ARMORED };

    float colActive[4][4] = {
        {0.75f, 0.75f, 0.75f, 1.0f},
        {1.0f,  1.0f,  0.0f,  1.0f},
        {1.0f,  0.0f,  0.0f,  1.0f},
        {1.0f,  0.5f,  0.0f,  1.0f},
    };
    float colDead[4] = {0.25f, 0.25f, 0.25f, 1.0f};

    float rowY = cy - 280.0f;
    float rectW = 40.0f, rectH = 40.0f;

    for (int t = 0; t < 4; t++) {
        int killed = gKills[t];
        int total  = totalByType[t];
        int score  = killed * points[t];

        float iconX = cx - 200.0f;
        float* col  = (killed > 0) ? colActive[t] : colDead;
        float nr = col[0], ng = col[1], nb = col[2];

        draw_rect(iconX, rowY + 15.0f, rectW, rectH, col);

        render_text(names[t], iconX + rectW + 15.0f, rowY, 1.0f, nr, ng, nb);

        char line[64];
        sprintf(line, "%d/%d  x%d = %d pts", killed, total, points[t], score);
        render_text(line, iconX + rectW + 15.0f, rowY + 45.0f, 0.85f, nr, ng, nb);

        rowY += 120.0f;
    }

    rowY += 20.0f;

    if (gTimeBonusAwarded > 0) {
        char txt[64];
        sprintf(txt, "SPEED BONUS:  +%d", gTimeBonusAwarded);
        render_text(txt, cx - 250, rowY - 25, 1.0f, 0.2f, 1.0f, 0.8f);
        rowY += 60.0f;
    }

    char txt[64];
    sprintf(txt, "TOTAL SCORE:  %d", gScore);
    render_text(txt, cx - 275, rowY, 1.3f, 1.0f, 1.0f, 0.2f);

    extern double victoryTimer;
    if (glfwGetTime() - victoryTimer < 2.0) {
        render_text("...", cx - 20, rowY + 100.0f, 1.5f, 0.5f, 0.5f, 0.5f);
        return;
    }

    const char* items[] = { "NEXT LEVEL", "MAIN MENU" };
    for (int i = 0; i < 2; i++) {
        float y  = rowY + 80.0f + i * 70.0f;
        float br = (selectedMenuItem == i) ? 1.0f : 0.5f;
        if (selectedMenuItem == i)
            render_text(">", cx - 235, y, 1.2f, 1.0f, 1.0f, 1.0f);
        render_text(items[i], cx - 175, y, 1.2f, br, br, br);
    }

    render_text("W/S - Navigate     SPACE - Select",
                cx - 300, rowY + 240.0f, 0.8f, 0.3f, 0.3f, 0.3f);
}

// ── Ввод ника ─────────────────────────────────────────────────────────
void render_nick_input(void)
{
    float cx = windowWidth  / 2.0f;
    float cy = windowHeight / 2.0f;

    render_text("ENTER YOUR NAME",
                cx - 420, cy - 100, 2.0f, 1.0f, 0.8f, 0.2f);

    // Рамка поля ввода
    float boxW = 500.0f, boxH = 80.0f;
    float boxX = cx - boxW / 2.0f;
    float boxY = cy - 10.0f;
    float frame[4] = {0.4f, 0.4f, 0.4f, 1.0f};
    draw_rect(cx, cy + 20.0f, boxW + 8, boxH + 8, frame);
    float bg[4] = {0.1f, 0.1f, 0.1f, 1.0f};
    draw_rect(cx, cy + 20.0f, boxW, boxH, bg);

    // Текст + курсор мигающий
    char display[NICK_MAX_LEN + 2];
    double t = glfwGetTime();
    if ((int)(t * 2) % 2 == 0)
        snprintf(display, sizeof(display), "%s_", gPlayerNick);
    else
        snprintf(display, sizeof(display), "%s ", gPlayerNick);

    render_text(display, boxX + 12.0f, boxY + 50.0f, 1.3f, 1.0f, 1.0f, 1.0f);

    render_text("SPACE - confirm    BACKSPACE - delete",
                cx - 350, cy + 130, 0.8f, 0.3f, 0.3f, 0.3f);
}

// ── Таблица рекордов ──────────────────────────────────────────────────
void render_leaderboard(void)
{
    float cx = windowWidth  / 2.0f;
    float cy = windowHeight / 2.0f;

    render_text("LEADERBOARD", cx - 375, cy - 400, 2.2f, 1.0f, 0.8f, 0.2f);

    // Заголовки колонок
    render_text("#",      cx - 375, cy - 300, 0.9f, 0.6f, 0.6f, 0.6f);
    render_text("NAME",   cx - 300, cy - 300, 0.9f, 0.6f, 0.6f, 0.6f);
    render_text("SCORE",  cx +  50, cy - 300, 0.9f, 0.6f, 0.6f, 0.6f);
    render_text("LEVEL",  cx + 200, cy - 300, 0.9f, 0.6f, 0.6f, 0.6f);

    int count = leaderboard_count();
    for (int i = 0; i < count; i++) {
        LeaderboardEntry* e = leaderboard_get(i);
        if (!e) break;

        float y = cy - 250 + i * 55;
        // Подсветка первой тройки
        float r = 0.9f, g = 0.9f, b = 0.9f;
        if      (i == 0) { r = 1.0f; g = 0.85f; b = 0.1f; }  // золото
        else if (i == 1) { r = 0.8f; g = 0.8f;  b = 0.9f; }  // серебро
        else if (i == 2) { r = 0.9f; g = 0.5f;  b = 0.2f; }  // бронза

        char num[8], score[16], level[8];
        snprintf(num,   sizeof(num),   "%d.",    i + 1);
        snprintf(score, sizeof(score), "%d",     e->score);
        snprintf(level, sizeof(level), "LVL %d", e->level + 1);

        render_text(num,     cx - 375, y, 0.8f, r, g, b);
        render_text(e->nick, cx - 300, y, 0.8f, r, g, b);
        render_text(score,   cx +  50, y, 0.8f, r, g, b);
        render_text(level,   cx + 200, y, 0.8f, r, g, b);
    }

    if (count == 0)
        render_text("No records yet. Be the first!",
                    cx - 240, cy, 1.0f, 0.5f, 0.5f, 0.5f);

    render_text("SPACE / ESC - Back",
                cx - 175, cy + 400, 0.8f, 0.3f, 0.3f, 0.3f);
}

// ── Пауза ─────────────────────────────────────────────────────────────
void render_pause_menu(void)
{
    float cx = windowWidth  / 2.0f;
    float cy = windowHeight / 2.0f;

    // Полупрозрачный фон
    float dark[4] = {0.0f, 0.0f, 0.0f, 0.6f};
    draw_rect(cx, cy, (float)windowWidth, (float)windowHeight, dark);

    render_text("PAUSED", cx - 240, cy - 180, 2.5f, 1.0f, 1.0f, 0.2f);

    const char* items[] = { "RESUME", "RESTART LEVEL", "MAIN MENU" };
    for (int i = 0; i < 3; i++) {
        float y  = cy - 40 + i * 90;
        float br = (selectedMenuItem == i) ? 1.0f : 0.5f;
        if (selectedMenuItem == i)
            render_text(">", cx - 225, y, 1.2f, 1.0f, 1.0f, 1.0f);
        render_text(items[i], cx - 165, y, 1.2f, br, br, br);
    }

    render_text("W/S - Navigate     SPACE - Select     ESC - Resume",
                cx - 425, cy + 240, 0.75f, 0.5f, 0.5f, 0.5f);
}
