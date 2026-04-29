#include <bullet.h>
#include <collision.h>

void bullet_update(Bullet* b, int collision_who,
                   float deltaTime,
                   int fieldX, int fieldY, int fieldSize)
{
    if (!b->active) return;

    b->x += b->dirX * BULLET_SPEED * deltaTime;
    b->y += b->dirY * BULLET_SPEED * deltaTime;

    // Выход за границы поля
    float halfW = BULLET_WIDTH  / 2.0f;
    float halfH = BULLET_HEIGHT / 2.0f;
    if (b->x < fieldX + halfW || b->x > fieldX + fieldSize - halfW ||
        b->y < fieldY + halfH || b->y > fieldY + fieldSize - halfH) {
        b->active = 0;
        return;
    }

    // Коллизия с картой
    float w = (b->dirX != 0) ? BULLET_WIDTH  : BULLET_HEIGHT;
    float h = (b->dirX != 0) ? BULLET_HEIGHT : BULLET_WIDTH;
    if (check_rect_collision_with_map(collision_who, b->x, b->y,
                                      w, h, b->dirX, b->dirY)) {
        b->active = 0;
    }
}
