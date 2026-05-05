#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "leaderboard.h"

LeaderboardEntry* gLeaderboard = NULL;
char gPlayerNick[NICK_MAX_LEN] = "PLAYER";

// Вставляет запись в список в порядке убывания счёта.
// Если записей больше LEADERBOARD_MAX — обрезает хвост.
void leaderboard_add(const char* nick, int score, int level)
{
    LeaderboardEntry* entry = (LeaderboardEntry*)malloc(sizeof(LeaderboardEntry));
    if (!entry) return;
    strncpy(entry->nick, nick, NICK_MAX_LEN - 1);
    entry->nick[NICK_MAX_LEN - 1] = '\0';
    entry->score = score;
    entry->level = level;
    entry->next  = NULL;

    // Вставка в отсортированную позицию (по убыванию score)
    if (!gLeaderboard || score > gLeaderboard->score) {
        entry->next  = gLeaderboard;
        gLeaderboard = entry;
    } else {
        LeaderboardEntry* cur = gLeaderboard;
        while (cur->next && cur->next->score >= score)
            cur = cur->next;
        entry->next = cur->next;
        cur->next   = entry;
    }

    // Обрезаем хвост сверх лимита
    LeaderboardEntry* cur = gLeaderboard;
    int count = 1;
    while (cur->next) {
        if (count >= LEADERBOARD_MAX) {
            LeaderboardEntry* tail = cur->next;
            cur->next = NULL;
            while (tail) {
                LeaderboardEntry* tmp = tail->next;
                free(tail);
                tail = tmp;
            }
            break;
        }
        cur = cur->next;
        count++;
    }
}

void leaderboard_free(void)
{
    LeaderboardEntry* cur = gLeaderboard;
    while (cur) {
        LeaderboardEntry* tmp = cur->next;
        free(cur);
        cur = tmp;
    }
    gLeaderboard = NULL;
}

void leaderboard_save(void)
{
    FILE* f = fopen(LEADERBOARD_FILE, "w");
    if (!f) return;
    LeaderboardEntry* cur = gLeaderboard;
    while (cur) {
        fprintf(f, "%s %d %d\n", cur->nick, cur->score, cur->level);
        cur = cur->next;
    }
    fclose(f);
}

void leaderboard_load(void)
{
    leaderboard_free();
    FILE* f = fopen(LEADERBOARD_FILE, "r");
    if (!f) return;
    char nick[NICK_MAX_LEN];
    int  score, level;
    while (fscanf(f, "%s %d %d", nick, &score, &level) == 3)
        leaderboard_add(nick, score, level);
    fclose(f);
}

int leaderboard_count(void)
{
    int n = 0;
    LeaderboardEntry* cur = gLeaderboard;
    while (cur) { n++; cur = cur->next; }
    return n;
}

LeaderboardEntry* leaderboard_get(int index)
{
    LeaderboardEntry* cur = gLeaderboard;
    for (int i = 0; i < index && cur; i++)
        cur = cur->next;
    return cur;
}
