#ifndef LEADERBOARD_H
#define LEADERBOARD_H

#define LEADERBOARD_FILE "leaderboard.txt"
#define LEADERBOARD_MAX  10
#define NICK_MAX_LEN     13

// ── Узел односвязного списка рекордов ────────────────────────────────
typedef struct LeaderboardEntry {
    char   nick[NICK_MAX_LEN];
    int    score;
    int    level;
    struct LeaderboardEntry* next;
} LeaderboardEntry;

extern LeaderboardEntry* gLeaderboard; // голова списка
extern char gPlayerNick[NICK_MAX_LEN]; // ник текущего игрока

void leaderboard_load(void);
void leaderboard_save(void);
void leaderboard_add(const char* nick, int score, int level);
void leaderboard_free(void);
int  leaderboard_count(void);
LeaderboardEntry* leaderboard_get(int index);

#endif 
