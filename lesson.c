#include "lesson.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static char *trim(char *s)
{
    char *end;
    while (*s && isspace((unsigned char)*s)) ++s;
    end = s + strlen(s);
    while (end > s && isspace((unsigned char)end[-1])) --end;
    *end = '\0';
    return s;
}

static char *unescaped(char *s, int wanted)
{
    int quoted = 0;
    while (*s) {
        if (!quoted && *s == wanted) return s;
        if (!quoted && *s == '\\') quoted = 1;
        else quoted = 0;
        ++s;
    }
    return NULL;
}

static int copy_unescaped(char *out, unsigned size, const char *in)
{
    unsigned used = 0;
    int quoted = 0;
    while (*in) {
        if (!quoted && *in == '\\') { quoted = 1; ++in; continue; }
        if (used + 1 >= size) return 0;
        out[used++] = *in++;
        quoted = 0;
    }
    out[used] = '\0';
    return 1;
}

static int parse_side(char *source, Side *side)
{
    char *part, *comma, *slash, *separator;
    side->count = 0;
    part = source;
    for (;;) {
        comma = unescaped(part, ',');
        slash = unescaped(part, '/');
        if (!comma) separator = slash;
        else if (!slash) separator = comma;
        else separator = comma < slash ? comma : slash;
        if (separator) *separator = '\0';
        part = trim(part);
        if (!*part || side->count >= MAX_SYNONYMS ||
            !copy_unescaped(side->text[side->count], MAX_TEXT, part)) return 0;
        ++side->count;
        if (!separator) break;
        part = separator + 1;
    }
    return side->count != 0;
}

static int parse_pair(char *line, Side *left, Side *right)
{
    char *colon = unescaped(line, ':');
    if (!colon) return 0;
    *colon = '\0';
    return parse_side(trim(line), left) && parse_side(trim(colon + 1), right);
}

int lesson_load(const char *name, Lesson *lesson, char *error, unsigned error_size)
{
    FILE *file;
    static char line[1024];
    unsigned number = 0;
    int have_header = 0;
    static Side a, b;
    lesson->count = 0;
    memset(lesson->cards, 0, sizeof(lesson->cards));
    file = fopen(name, "rt");
    if (!file) { snprintf(error, error_size, "Cannot open %s", name); return 0; }
    while (fgets(line, sizeof(line), file)) {
        char *p;
        ++number;
        line[strcspn(line, "\r\n")] = '\0';
        p = trim(line);
        if (!*p || *p == '#') continue;
        if (!parse_pair(p, &a, &b)) {
            snprintf(error, error_size, "Invalid or too long line %u", number);
            fclose(file); lesson_free(lesson); return 0;
        }
        if (!have_header) {
            strcpy(lesson->left_name, a.text[0]);
            strcpy(lesson->right_name, b.text[0]);
            have_header = 1;
        } else {
            if (lesson->count >= MAX_CARDS) {
                snprintf(error, error_size, "More than %u cards", MAX_CARDS);
                fclose(file); lesson_free(lesson); return 0;
            }
            lesson->cards[lesson->count] = (Card *)malloc(sizeof(Card));
            if (!lesson->cards[lesson->count]) {
                snprintf(error, error_size, "Not enough memory at card %u", lesson->count + 1);
                fclose(file); lesson_free(lesson); return 0;
            }
            lesson->cards[lesson->count]->left = a;
            lesson->cards[lesson->count]->right = b;
            ++lesson->count;
        }
    }
    if (ferror(file)) { snprintf(error, error_size, "Read error at line %u", number); fclose(file); lesson_free(lesson); return 0; }
    fclose(file);
    if (!have_header || !lesson->count) { snprintf(error, error_size, "Lesson has no cards"); lesson_free(lesson); return 0; }
    return 1;
}

void lesson_free(Lesson *lesson)
{
    unsigned i;
    for (i = 0; i < lesson->count; ++i) { free(lesson->cards[i]); lesson->cards[i] = NULL; }
    lesson->count = 0;
}

void answer_normalize(char *out, const char *in, unsigned size)
{
    unsigned used = 0;
    int pending_space = 0;
    while (*in && isspace((unsigned char)*in)) ++in;
    while (*in && used + 1 < size) {
        unsigned char c = (unsigned char)*in++;
        if (isspace(c)) { pending_space = used != 0; continue; }
        if (pending_space && used + 1 < size) out[used++] = ' ';
        pending_space = 0;
        if (c == 0x8e) c = 0x84;      /* CP850: A-umlaut */
        else if (c == 0x99) c = 0x94; /* CP850: O-umlaut */
        else if (c == 0x9a) c = 0x81; /* CP850: U-umlaut */
        else c = (unsigned char)tolower(c);
        out[used++] = (char)c;
    }
    out[used] = '\0';
}

int answer_matches(const char *answer, const Side *side)
{
    AnswerResult result;
    answer_assess(answer, side, &result);
    return result.kind == ANSWER_EXACT || result.kind == ANSWER_CAPITALIZATION ||
           result.kind == ANSWER_APOSTROPHE;
}

static void normalize_exact(char *out, const char *in, unsigned size)
{
    unsigned used = 0; int pending = 0;
    while (*in && isspace((unsigned char)*in)) ++in;
    while (*in && used + 1 < size) {
        unsigned char c = (unsigned char)*in++;
        if (isspace(c)) { pending = used != 0; continue; }
        if (pending && used + 1 < size) out[used++] = ' ';
        pending = 0; out[used++] = (char)c;
    }
    out[used] = '\0';
}

static void remove_ignored_characters(char *text)
{
    char *read = text, *write = text;
    while (*read) {
        unsigned char c = (unsigned char)*read++;
        if (!isspace(c) && c != '.') *write++ = (char)c;
    }
    *write = '\0';
}

static int prefix_equal(const char *text, const char *prefix)
{
    while (*prefix) {
        if (tolower((unsigned char)*text) != tolower((unsigned char)*prefix)) return 0;
        ++text; ++prefix;
    }
    return 1;
}

static int is_apostrophe(unsigned char c)
{
    /* ASCII straight/backtick and the acute-accent key in DOS CP850. */
    return c == '\'' || c == '`' || c == 0xef;
}

static int apostrophes_equivalent(const char *a, const char *b)
{
    int different = 0;
    while (*a && *b) {
        if (*a != *b) {
            if (!is_apostrophe((unsigned char)*a) ||
                !is_apostrophe((unsigned char)*b)) return 0;
            different = 1;
        }
        ++a; ++b;
    }
    return different && !*a && !*b;
}

/* Articles and infinitive markers are optional only at the start of an
   answer. Never remove one when it is the complete answer. */
static const char *skip_optional_prefixes(const char *text)
{
    static const char *words[] = { "the", "to", "der", "die", "das",
                                   "le", "la", "les", "il", "lo", "i",
                                   "gli", "el", "los", "las" };
    unsigned i;
    for (;;) {
        int removed = 0;
        if (tolower((unsigned char)text[0]) == 'l' &&
            is_apostrophe((unsigned char)text[1]) && text[2] != '\0') {
            text += 2; removed = 1;
        } else {
            for (i = 0; i < sizeof(words) / sizeof(words[0]); ++i) {
                unsigned length = (unsigned)strlen(words[i]);
                if (prefix_equal(text, words[i]) && text[length] == ' ' && text[length + 1] != '\0') {
                    text += length + 1; removed = 1; break;
                }
            }
        }
        if (!removed) return text;
    }
}

static unsigned edit_distance(const char *a, const char *b)
{
    unsigned char previous[MAX_TEXT], current[MAX_TEXT];
    unsigned i, j, la = (unsigned)strlen(a), lb = (unsigned)strlen(b);
    if (la >= MAX_TEXT || lb >= MAX_TEXT) return 255;
    for (j = 0; j <= lb; ++j) previous[j] = (unsigned char)j;
    for (i = 1; i <= la; ++i) {
        current[0] = (unsigned char)i;
        for (j = 1; j <= lb; ++j) {
            unsigned del = previous[j] + 1, ins = current[j - 1] + 1;
            unsigned sub = previous[j - 1] + (a[i - 1] != b[j - 1]);
            unsigned best = del < ins ? del : ins;
            current[j] = (unsigned char)(sub < best ? sub : best);
        }
        memcpy(previous, current, lb + 1);
    }
    return previous[lb];
}

static int adjacent_in_layout(char a, char b, const char *top, const char *bottom)
{
    const char *rows[3]; int ar = -1, ac = -1, br = -1, bc = -1, r, c;
    rows[0] = top; rows[1] = "asdfghjkl"; rows[2] = bottom;
    for (r = 0; r < 3; ++r) for (c = 0; rows[r][c]; ++c) {
        if (rows[r][c] == a) { ar = r; ac = c * 2 + r; }
        if (rows[r][c] == b) { br = r; bc = c * 2 + r; }
    }
    return ar >= 0 && br >= 0 && abs(ar - br) <= 1 && abs(ac - bc) <= 2;
}

static int keyboard_typo(const char *answer, const char *expected)
{
    unsigned i, differences = 0;
    if (strlen(answer) != strlen(expected)) return 0;
    for (i = 0; answer[i]; ++i) if (answer[i] != expected[i]) {
        ++differences;
        if (differences > 1 ||
            (!adjacent_in_layout(answer[i], expected[i], "qwertyuiop", "zxcvbnm") &&
             !adjacent_in_layout(answer[i], expected[i], "qwertzuiop", "yxcvbnm"))) return 0;
    }
    return differences == 1;
}

void answer_assess(const char *answer, const Side *side, AnswerResult *result)
{
    char raw_answer[MAX_TEXT], raw_expected[MAX_TEXT], folded_answer[MAX_TEXT], folded_expected[MAX_TEXT];
    char full_raw_answer[MAX_TEXT], full_raw_expected[MAX_TEXT];
    char full_folded_answer[MAX_TEXT], full_folded_expected[MAX_TEXT];
    const char *plain_answer, *plain_expected, *simple_answer, *simple_expected;
    unsigned i, distance, full_distance, best_distance = 255, best = 0;
    result->kind = ANSWER_WRONG; result->match = 0; result->distance = 255;
    normalize_exact(raw_answer, answer, sizeof(raw_answer));
    answer_normalize(folded_answer, answer, sizeof(folded_answer));
    strcpy(full_raw_answer, raw_answer); remove_ignored_characters(full_raw_answer);
    strcpy(full_folded_answer, folded_answer); remove_ignored_characters(full_folded_answer);
    plain_answer = skip_optional_prefixes(raw_answer);
    simple_answer = skip_optional_prefixes(folded_answer);
    remove_ignored_characters((char *)plain_answer);
    remove_ignored_characters((char *)simple_answer);
    for (i = 0; i < side->count; ++i) {
        normalize_exact(raw_expected, side->text[i], sizeof(raw_expected));
        answer_normalize(folded_expected, side->text[i], sizeof(folded_expected));
        strcpy(full_raw_expected, raw_expected); remove_ignored_characters(full_raw_expected);
        strcpy(full_folded_expected, folded_expected); remove_ignored_characters(full_folded_expected);
        if (!strcmp(full_raw_answer, full_raw_expected)) {
            result->kind = ANSWER_EXACT; result->match = i; result->distance = 0; return;
        }
        if (apostrophes_equivalent(full_raw_answer, full_raw_expected)) {
            result->kind = ANSWER_APOSTROPHE; result->match = i; result->distance = 0; return;
        }
        if (!strcmp(full_folded_answer, full_folded_expected)) {
            result->kind = ANSWER_CAPITALIZATION; result->match = i; result->distance = 0; return;
        }
        if (apostrophes_equivalent(full_folded_answer, full_folded_expected)) {
            result->kind = ANSWER_APOSTROPHE; result->match = i; result->distance = 0; return;
        }
        plain_expected = skip_optional_prefixes(raw_expected);
        simple_expected = skip_optional_prefixes(folded_expected);
        remove_ignored_characters((char *)plain_expected);
        remove_ignored_characters((char *)simple_expected);
        if (!strcmp(plain_answer, plain_expected)) {
            result->kind = ANSWER_EXACT; result->match = i; result->distance = 0; return;
        }
        if (apostrophes_equivalent(plain_answer, plain_expected)) {
            result->kind = ANSWER_APOSTROPHE; result->match = i; result->distance = 0; return;
        }
        if (!strcmp(simple_answer, simple_expected)) {
            result->kind = ANSWER_CAPITALIZATION; result->match = i; result->distance = 0; return;
        }
        if (apostrophes_equivalent(simple_answer, simple_expected)) {
            result->kind = ANSWER_APOSTROPHE; result->match = i; result->distance = 0; return;
        }
        if (keyboard_typo(full_folded_answer, full_folded_expected) ||
            keyboard_typo(simple_answer, simple_expected)) {
            result->kind = ANSWER_TYPO; result->match = i; result->distance = 1; return;
        }
        distance = edit_distance(simple_answer, simple_expected);
        full_distance = edit_distance(full_folded_answer, full_folded_expected);
        if (full_distance < distance) distance = full_distance;
        if (distance < best_distance) { best_distance = distance; best = i; }
    }
    result->match = best; result->distance = best_distance;
    if (best_distance == 1 || best_distance == 2) result->kind = ANSWER_SPELLING;
}

static void write_escaped(FILE *file, const char *text)
{
    while (*text) {
        if (*text == ',' || *text == '/' || *text == ':' || *text == '\\') fputc('\\', file);
        fputc((unsigned char)*text++, file);
    }
}

static void write_side(FILE *file, const Side *side)
{
    unsigned i;
    for (i = 0; i < side->count; ++i) {
        if (i) fputs(", ", file);
        write_escaped(file, side->text[i]);
    }
}

int lesson_save(const char *name, const Lesson *lesson, char *error, unsigned error_size)
{
    char temporary[160], backup[160], *dot, *slash; FILE *file; unsigned i;
    strncpy(temporary, name, sizeof(temporary) - 1); temporary[sizeof(temporary) - 1] = '\0';
    slash = strrchr(temporary, '\\'); dot = strrchr(temporary, '.');
    if (dot && (!slash || dot > slash)) *dot = '\0';
    strcpy(backup, temporary); strcat(temporary, ".$$$"); strcat(backup, ".BAK");
    file = fopen(temporary, "wt");
    if (!file) { snprintf(error, error_size, "Cannot create %s", temporary); return 0; }
    write_escaped(file, lesson->left_name); fputs(" : ", file); write_escaped(file, lesson->right_name); fputc('\n', file);
    for (i = 0; i < lesson->count; ++i) {
        write_side(file, &lesson->cards[i]->left); fputs(" : ", file);
        write_side(file, &lesson->cards[i]->right); fputc('\n', file);
    }
    if (ferror(file) || fclose(file)) { snprintf(error, error_size, "Error writing %s", temporary); return 0; }
    remove(backup);
    if (rename(name, backup)) { snprintf(error, error_size, "Cannot back up lesson"); remove(temporary); return 0; }
    if (rename(temporary, name)) { rename(backup, name); snprintf(error, error_size, "Cannot replace lesson"); return 0; }
    return 1;
}
