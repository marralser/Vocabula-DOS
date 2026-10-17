#ifndef STATS_H
#define STATS_H

#include "lesson.h"

typedef struct {
    unsigned asked;
    unsigned right;
    unsigned wrong;
    unsigned hints;
    int score;
} SessionStats;

#define MAX_HIGH_SCORES 10
typedef struct {
    char date[17];
    char name[21];
    unsigned asked;
    unsigned right;
    unsigned wrong;
    int score;
} HighScore;

void stats_names(const char *lesson, char *sum, char *cards, char *errors);
void stats_save_session(const char *sum_name, const char *student, const SessionStats *stats);
void stats_record_card(const char *cards_name, unsigned card, int correct);
void stats_record_error(const char *errors_name, unsigned card, const char *question,
                        const char *answer, const Side *expected);
unsigned stats_high_scores(const char *sum_name, HighScore *scores, unsigned maximum);

#endif
