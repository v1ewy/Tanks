#include <pathfinding.h>
#include <map.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

// ── A* ────────────────────────────────────────────────────────────────

typedef struct {
    int   x, y;
    float g, f;
    int   parentX, parentY;
    int   open;    // 1=open, 2=closed, 0=не посещён
} PFNode;

static PFNode nodes[GRID_SIZE][GRID_SIZE];

// Стоимость прохода через клетку
static float tile_cost(int x, int y, int canBreakWood)
{
    if (x < 0 || x >= GRID_SIZE || y < 0 || y >= GRID_SIZE)
        return 1e9f;

    int t = map[y][x];
    if (t == 2) return 1e9f;   // стена
    if (t == 3) return 1e9f;   // вода
    if (t == 5) return canBreakWood ? 10.0f : 1e9f; // дерево
    if (t == 4) return 1.0f;   // листва — проходима как пустота ← уже должно быть так
    return 1.0f;
}

static float heuristic(int x, int y, int gx, int gy)
{
    return (float)(abs(x - gx) + abs(y - gy)); // Манхэттен
}


PFPoint pathfind_next_step(int fromX, int fromY,
                            int toX,   int toY,
                            int canBreakWood)
{
    PFPoint fail = {-1, -1};

    // Если уже в цели или цель недостижима
    if (fromX == toX && fromY == toY) return fail;
    if (toX < 0 || toX >= GRID_SIZE || toY < 0 || toY >= GRID_SIZE) return fail;

    // Сброс
    memset(nodes, 0, sizeof(nodes));
    for (int j = 0; j < GRID_SIZE; j++)
        for (int i = 0; i < GRID_SIZE; i++) {
            nodes[j][i].parentX = -1;
            nodes[j][i].parentY = -1;
        }

    nodes[fromY][fromX].g    = 0.0f;
    nodes[fromY][fromX].f    = heuristic(fromX, fromY, toX, toY);
    nodes[fromY][fromX].open = 1;

    int dx4[] = { 1, -1,  0,  0};
    int dy4[] = { 0,  0, -1,  1};

    for (int iter = 0; iter < PF_MAX_NODES * 8; iter++) {
        // Найти узел с минимальным f
        float bestF = 1e10f;
        int   cx = -1, cy = -1;
        for (int j = 0; j < GRID_SIZE; j++) {
            for (int i = 0; i < GRID_SIZE; i++) {
                if (nodes[j][i].open == 1 && nodes[j][i].f < bestF) {
                    bestF = nodes[j][i].f;
                    cx = i; cy = j;
                }
            }
        }

        if (cx == -1) break;

        // Достигли цели
        if (cx == toX && cy == toY) {
            // Восстанавливаем путь до первого шага
            int px = cx, py = cy;
            for (;;) {
                int npx = nodes[py][px].parentX;
                int npy = nodes[py][px].parentY;
                if (npx == -1 || (npx == fromX && npy == fromY)) {
                    PFPoint result = {px, py};
                    return result;
                }
                px = npx;
                py = npy;
            }
        }

        nodes[cy][cx].open = 2; // closed

        for (int d = 0; d < 4; d++) {
            int nx = cx + dx4[d];
            int ny = cy + dy4[d];

            if (nx < 0 || nx >= GRID_SIZE ||
                ny < 0 || ny >= GRID_SIZE) continue;
            if (nodes[ny][nx].open == 2) continue;

            float cost = tile_cost(nx, ny, canBreakWood);
            if (cost >= 1e8f) continue;

            float newG = nodes[cy][cx].g + cost;

            if (nodes[ny][nx].open == 0 || newG < nodes[ny][nx].g) {
                nodes[ny][nx].g       = newG;
                nodes[ny][nx].f       = newG + heuristic(nx, ny, toX, toY);
                nodes[ny][nx].parentX = cx;
                nodes[ny][nx].parentY = cy;
                nodes[ny][nx].open    = 1;
            }
        }
    }

    return fail;
}
