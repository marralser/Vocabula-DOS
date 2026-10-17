#include "lesson.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    static const char *prefixes[] = { "the", "to", "der", "die", "das",
                                      "le", "la", "les", "il", "lo", "i",
                                      "gli", "l'", "el", "los", "las" };
    Lesson lesson; char error[160], normalized[80], prefixed[80];
    AnswerResult result; Side optional; unsigned p;
    if (argc == 3 && !strcmp(argv[1], "--validate")) {
        if (!lesson_load(argv[2], &lesson, error, sizeof(error))) {
            fprintf(stderr, "%s: %s\n", argv[2], error); return 1;
        }
        printf("%s: %u cards\n", argv[2], lesson.count);
        lesson_free(&lesson); return 0;
    }
    assert(argc == 2);
    assert(lesson_load(argv[1], &lesson, error, sizeof(error)));
    assert(lesson.count == 5);
    assert(answer_matches(" LOOKING ", &lesson.cards[0]->left));
    assert(answer_matches("sehen", &lesson.cards[0]->right));
    assert(answer_matches("new york", &lesson.cards[3]->left));
    assert(answer_matches("newyork", &lesson.cards[3]->left));
    assert(answer_matches("new... york.", &lesson.cards[3]->left));
    assert(!answer_matches("wrong", &lesson.cards[0]->right));
    assert(lesson.cards[1]->left.count == 2);
    assert(!strcmp(lesson.cards[1]->left.text[0], "house"));
    assert(!strcmp(lesson.cards[1]->left.text[1], "home"));
    assert(lesson.cards[1]->right.count == 2);
    assert(!strcmp(lesson.cards[4]->left.text[0], "comma,word"));
    assert(!strcmp(lesson.cards[4]->left.text[1], "path/name"));
    answer_assess("LOOKING", &lesson.cards[0]->left, &result);
    assert(result.kind == ANSWER_CAPITALIZATION && result.match == 1);
    answer_assess("looling", &lesson.cards[0]->left, &result);
    assert(result.kind == ANSWER_TYPO && result.match == 1);
    answer_assess("lookin", &lesson.cards[0]->left, &result);
    assert(result.kind == ANSWER_SPELLING && result.distance == 1);
    answer_assess("lxokin", &lesson.cards[0]->left, &result);
    assert(result.kind == ANSWER_SPELLING && result.distance == 2);
    answer_assess("banana", &lesson.cards[0]->left, &result);
    assert(result.kind == ANSWER_WRONG);
    answer_normalize(normalized, "  Too   MANY  Spaces ", sizeof(normalized));
    assert(!strcmp(normalized, "too many spaces"));
    memset(&optional, 0, sizeof(optional)); optional.count = 1;
    for (p = 0; p < sizeof(prefixes) / sizeof(prefixes[0]); ++p) {
        if (!strcmp(prefixes[p], "l'")) sprintf(prefixed, "%shouse", prefixes[p]);
        else sprintf(prefixed, "%s house", prefixes[p]);
        strcpy(optional.text[0], "house");
        assert(answer_matches(prefixed, &optional));
        strcpy(optional.text[0], prefixed);
        assert(answer_matches("house", &optional));
    }
    strcpy(optional.text[0], "Haus");
    assert(answer_matches("das Haus", &optional));
    strcpy(optional.text[0], "das Haus");
    assert(answer_matches("Haus", &optional));
    strcpy(optional.text[0], "to play");
    assert(answer_matches("play", &optional));
    strcpy(optional.text[0], "the house");
    assert(answer_matches("thehouse", &optional));
    strcpy(optional.text[0], "homme");
    assert(answer_matches("l'homme", &optional));
    strcpy(optional.text[0], "l'amico");
    answer_assess("l`amico", &optional, &result);
    assert(result.kind == ANSWER_APOSTROPHE && answer_matches("l`amico", &optional));
    answer_assess("l\xef" "amico", &optional, &result);
    assert(result.kind == ANSWER_APOSTROPHE && answer_matches("l\xef" "amico", &optional));
    strcpy(optional.text[0], "the");
    assert(!answer_matches("", &optional));
    strcpy(optional.text[0], "hotel");
    assert(!answer_matches("hot", &optional));
    strcpy(optional.text[0], "there");
    assert(!answer_matches("re", &optional));
    strcpy(optional.text[0], "stehen bleiben...");
    assert(answer_matches("stehenbleiben", &optional));
    assert(answer_matches("stehen ... bleiben.", &optional));
    lesson_free(&lesson); puts("All tests passed."); return 0;
}
