#include <score.h>

int    gScore          = 0;
double gLevelStartTime = 0.0;

int gKills[4] = {0, 0, 0, 0};

void score_reset(void) {
    gScore = 0;
    gKills[0] = gKills[1] = gKills[2] = gKills[3] = 0;
}

void score_add_kill(BotType type) {
    gKills[type]++;
    switch (type) {
    case BOT_HUNTER:  gScore += SCORE_HUNTER;  break;
    case BOT_HOUND:   gScore += SCORE_HOUND;   break;
    case BOT_ARMORED: gScore += SCORE_ARMORED; break;
    default:          gScore += SCORE_NORMAL;  break;
    }
}

int score_time_bonus(int levelIndex, double elapsed)
{
    double limits[] = {
        SCORE_TIME_LIMIT_L1,
        SCORE_TIME_LIMIT_L2,
        SCORE_TIME_LIMIT_L3
    };
    int bonuses[] = {
        SCORE_TIME_BONUS_L1,
        SCORE_TIME_BONUS_L2,
        SCORE_TIME_BONUS_L3
    };

    if (levelIndex < 0 || levelIndex > 2) return 0;
    if (elapsed <= limits[levelIndex]) {
        gScore += bonuses[levelIndex];
        return bonuses[levelIndex];
    }
    return 0;
}
