#include "stats.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

void stats_names(const char *lesson, char *sum, char *cards, char *errors)
{
    char base[128];
    char *dot, *slash;
    strncpy(base, lesson, sizeof(base) - 1); base[sizeof(base) - 1] = '\0';
    slash = strrchr(base, '\\');
    dot = strrchr(base, '.');
    if (dot && (!slash || dot > slash)) *dot = '\0';
    sprintf(sum, "%s.SUM", base); sprintf(cards, "%s.CAR", base); sprintf(errors, "%s.ERR", base);
}

static void stamp(FILE *f)
{
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    if (t) fprintf(f, "%04d-%02d-%02d %02d:%02d", t->tm_year + 1900, t->tm_mon + 1,
                   t->tm_mday, t->tm_hour, t->tm_min);
    else fputs("unknown-time", f);
}

void stats_save_session(const char *name, const char *student, const SessionStats *s)
{
    FILE *f = fopen(name, "at");
    if (!f) return;
    stamp(f); fprintf(f, " asked=%u right=%u wrong=%u hints=%u score=%d name=%s\n",
                      s->asked, s->right, s->wrong, s->hints, s->score,
                      student && *student ? student : "Student");
    fclose(f);
}

void stats_record_card(const char *name, unsigned card, int correct)
{
    FILE *f = fopen(name, "at");
    if (!f) return;
    stamp(f); fprintf(f, " card=%u result=%s\n", card + 1, correct ? "right" : "wrong");
    fclose(f);
}

void stats_record_error(const char *name, unsigned card, const char *question,
                        const char *answer, const Side *expected)
{
    FILE *f = fopen(name, "at"); unsigned i;
    if (!f) return;
    stamp(f); fprintf(f, " card=%u question=[%s] answer=[%s] expected=[", card + 1, question, answer);
    for (i = 0; i < expected->count; ++i) fprintf(f, "%s%s", i ? ", " : "", expected->text[i]);
    fputs("]\n", f); fclose(f);
}

unsigned stats_high_scores(const char *name, HighScore *scores, unsigned maximum)
{
    FILE *file = fopen(name, "rt"); char line[256], date[17];
    unsigned count = 0, asked, right, wrong, hints, position, i; int score; char *saved_name;
    if (!file) return 0;
    while (fgets(line, sizeof(line), file)) {
        if (sscanf(line, "%16c asked=%u right=%u wrong=%u hints=%u score=%d",
                   date, &asked, &right, &wrong, &hints, &score) != 6) continue;
        date[16] = '\0'; position = count;
        for (i = 0; i < count; ++i) if (score > scores[i].score) { position = i; break; }
        if (position >= maximum) continue;
        if (count < maximum) ++count;
        for (i = count - 1; i > position; --i) scores[i] = scores[i - 1];
        strcpy(scores[position].date, date);
        strcpy(scores[position].name, "Student");
        saved_name = strstr(line, " name=");
        if (saved_name) {
            saved_name += 6; saved_name[strcspn(saved_name, "\r\n")] = '\0';
            strncpy(scores[position].name, saved_name, 20); scores[position].name[20] = '\0';
        }
        scores[position].asked = asked;
        scores[position].right = right; scores[position].wrong = wrong; scores[position].score = score;
    }
    fclose(file); return count;
}
