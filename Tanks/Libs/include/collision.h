#ifndef COLLISION_H
#define COLLISION_H

#define COLLISION_PLAYER 1
#define COLLISION_PLAYER_BULLET 2
#define COLLISION_BOT_BULLET 3
#define COLLISION_BOT 4

int check_rect_collision_with_map(int who, float cx, float cy,
                                  float w, float h,
                                  float bulletDirX, float bulletDirY);

#endif // COLLISION_H
