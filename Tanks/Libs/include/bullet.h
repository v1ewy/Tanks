#ifndef BULLET_H
#define BULLET_H

#define BULLET_SPEED 600.0f
#define BULLET_WIDTH 20.0f
#define BULLET_HEIGHT 10.0f

typedef struct {
    double lastShootTime;
    float  x, y;
    float  dirX, dirY;
    int    active;
} Bullet;

void bullet_update(Bullet* b, int collision_who,
                   float deltaTime,
                   int fieldX, int fieldY, int fieldSize);

#endif // BULLET_H
