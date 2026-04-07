#ifndef BULLET_H
#define BULLET_H

// Константы пули
#define BULLET_SPEED 600.0f
#define BULLET_WIDTH 20.0f
#define BULLET_HEIGHT 10.0f

// Структура пули
typedef struct {
    double lastShootTime;
    float x, y;
    float dirX, dirY;
    int active;
} Bullet;

#endif // BULLET_H
