#ifndef LESSON_H
#define LESSON_H

#include <stdio.h>

#define MAX_CARDS 500
#define MAX_SYNONYMS 8
#define MAX_TEXT 80

typedef struct {
    char text[MAX_SYNONYMS][MAX_TEXT];
    unsigned count;
} Side;

typedef struct {
    Side left;
    Side right;
} Card;

typedef struct {
    char left_name[MAX_TEXT];
    char right_name[MAX_TEXT];
    Card *cards[MAX_CARDS];
    unsigned count;
} Lesson;

typedef enum {
    ANSWER_EXACT,
    ANSWER_CAPITALIZATION,
    ANSWER_APOSTROPHE,
    ANSWER_TYPO,
    ANSWER_SPELLING,
    ANSWER_WRONG
} AnswerKind;

typedef struct {
    AnswerKind kind;
    unsigned match;
    unsigned distance;
} AnswerResult;

int lesson_load(const char *name, Lesson *lesson, char *error, unsigned error_size);
int lesson_save(const char *name, const Lesson *lesson, char *error, unsigned error_size);
void lesson_free(Lesson *lesson);
int answer_matches(const char *answer, const Side *side);
void answer_assess(const char *answer, const Side *side, AnswerResult *result);
void answer_normalize(char *destination, const char *source, unsigned size);

#endif
