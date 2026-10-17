#include "lesson.h"
#include "stats.h"
#include "ui.h"
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#ifdef __WATCOMC__
#include <direct.h>
#include <dos.h>
#endif

#define INPUT_SIZE 160
#define PROGRAM_NAME "Vocabula"
#define PROGRAM_VERSION "0.1"
#define LESSON_DIRECTORY "VOC"
static char student_name[21] = "Student";

static int make_lesson_directory(void)
{
    struct stat information;
    if (stat(LESSON_DIRECTORY, &information) == 0) {
#ifdef __WATCOMC__
        return (information.st_mode & S_IFDIR) != 0;
#else
        return S_ISDIR(information.st_mode);
#endif
    }
#ifdef __WATCOMC__
    if (mkdir(LESSON_DIRECTORY) == 0) return 1;
#else
    if (mkdir(LESSON_DIRECTORY, 0777) == 0) return 1;
#endif
    return 0;
}

#ifdef __WATCOMC__
static void offer_lesson_migration(void)
{
    struct find_t found;
    unsigned rc, i, count = 0, moved = 0, skipped = 0;
    char names[100][13], target[32], line[80];

    rc = _dos_findfirst("*.VOC", _A_NORMAL, &found);
    while (!rc && count < 100) {
        strcpy(names[count++], found.name);
        rc = _dos_findnext(&found);
    }
    if (!count) return;

    ui_clear(UI_NORMAL);
    ui_box(7, 5, 74, 20, UI_NORMAL, "Organize lessons");
    sprintf(line, "I found %u lesson file(s) beside VC.EXE.", count);
    ui_center(8, UI_QUESTION, line);
    ui_center(11, UI_NORMAL, "Would you like me to move them into the VOC folder?");
    ui_center(14, UI_NORMAL, "Existing files with the same name will not be replaced.");
    ui_center(17, UI_QUESTION, "Move the lessons now? (y/N)");
    {
        char answer[8];
        int input = ui_readline(55, 17, 3, answer, sizeof(answer));
        if (input <= 0 || tolower((unsigned char)answer[0]) != 'y') return;
    }

    for (i = 0; i < count; ++i) {
        sprintf(target, LESSON_DIRECTORY "\\%s", names[i]);
        {
            FILE *existing = fopen(target, "rt");
            if (existing) { fclose(existing); ++skipped; }
            else if (rename(names[i], target) == 0) ++moved;
            else ++skipped;
        }
    }
    ui_clear(UI_NORMAL); ui_box(12, 8, 69, 17, UI_NORMAL, "Lessons organized");
    sprintf(line, "%u lesson(s) moved into VOC.", moved); ui_center(11, UI_GOOD, line);
    if (skipped) {
        sprintf(line, "%u file(s) were left where they were.", skipped);
        ui_center(13, UI_WARN, line);
    }
    ui_pause();
}
#endif

static int prepare_lesson_directory(void)
{
    if (!make_lesson_directory()) {
        ui_clear(UI_NORMAL); ui_box(9, 8, 72, 17, UI_NORMAL, "Folder problem");
        ui_center(11, UI_WARN, "I could not create or open the VOC lesson folder.");
        ui_center(13, UI_NORMAL, "Please check the disk and directory permissions.");
        ui_pause(); return 0;
    }
#ifdef __WATCOMC__
    offer_lesson_migration();
#endif
    return 1;
}

static void ask_student_name(void)
{
    char entered[64];
    ui_clear(UI_NORMAL); ui_box(12, 7, 69, 18, UI_NORMAL, "Welcome to Vocabula");
    ui_center(10, UI_QUESTION, "Hello! What is your name?");
    ui_write(18, 13, UI_NORMAL, "Name:");
    if (ui_readline(25, 13, 20, entered, sizeof(entered)) > 0 && *entered) {
        strncpy(student_name, entered, sizeof(student_name) - 1);
        student_name[sizeof(student_name) - 1] = '\0';
    }
}

static void strip_newline(char *s) { s[strcspn(s, "\r\n")] = '\0'; }

#ifdef __WATCOMC__
static int same_name(const char *a, const char *b)
{
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return 0;
        ++a; ++b;
    }
    return *a == *b;
}

static unsigned find_lessons(const char *directory, char names[][13], unsigned maximum)
{
    struct find_t found;
    char pattern[32];
    unsigned rc, count = 0;
    snprintf(pattern, sizeof(pattern), "%s\\*.VOC", directory);
    rc = _dos_findfirst(pattern, _A_NORMAL, &found);
    while (!rc && count < maximum) {
        strcpy(names[count++], found.name);
        rc = _dos_findnext(&found);
    }
    return count;
}

static unsigned find_lesson_folders(char names[][13], unsigned maximum)
{
    struct find_t found;
    unsigned rc, count = 0;
    rc = _dos_findfirst("*.*", _A_SUBDIR, &found);
    while (!rc && count < maximum) {
        if ((found.attrib & _A_SUBDIR) && strcmp(found.name, ".") &&
            strcmp(found.name, "..") && !same_name(found.name, LESSON_DIRECTORY))
            strcpy(names[count++], found.name);
        rc = _dos_findnext(&found);
    }
    return count;
}

static int select_from_folder(const char *directory, char *name, unsigned size)
{
    char names[128][13], input[20], item[24];
    unsigned count, rc, choice, page = 0, pages, first, last;
    int input_result;
    count = find_lessons(directory, names, 128);
    if (!count) {
        ui_clear(UI_NORMAL); ui_center(1, UI_TITLE, " VOCABULA 0.1 ");
        ui_box(2, 3, 79, 22, UI_NORMAL, directory);
        ui_center(11, UI_WARN, "This folder contains no *.VOC lessons.");
        ui_center(14, UI_QUESTION, "Press a key to return."); ui_pause(); return 0;
    }
    pages = (count + 31) / 32;
    for (;;) {
        first = page * 32; last = first + 32;
        if (last > count) last = count;
        ui_clear(UI_NORMAL); ui_center(1, UI_TITLE, " VOCABULA 0.1 ");
        ui_box(2, 3, 79, 22, UI_NORMAL, directory);
        for (rc = first; rc < last; ++rc) {
            sprintf(item, "%3u %-12s", rc + 1, names[rc]);
            ui_write(5 + (int)((rc - first) / 16) * 24,
                     5 + (int)((rc - first) % 16), UI_NORMAL, item);
        }
        for (rc = 0; rc < pages; ++rc) {
            sprintf(item, "%c  Lessons %u-%u", (char)('A' + rc), rc * 32 + 1,
                    (rc + 1) * 32 > count ? count : (rc + 1) * 32);
            ui_write(53, 5 + (int)rc, rc == page ? UI_TITLE : UI_INFO, item);
        }
        ui_write(4, 24, UI_QUESTION, "Number, page letter (0=back):");
        input_result = ui_readline(36, 24, 4, input, sizeof(input));
        if (input_result <= 0) return 0;
        if (isalpha((unsigned char)input[0]) && input[1] == '\0') {
            rc = (unsigned)(toupper((unsigned char)input[0]) - 'A');
            if (rc < pages) page = rc;
            continue;
        }
        choice = (unsigned)atoi(input);
        if (!choice) return 0;
        if (choice >= first + 1 && choice <= last) {
            snprintf(name, size, "%s\\%s", directory, names[choice - 1]);
            return 1;
        }
    }
}
#endif

static int select_lesson(char *name, unsigned size)
{
#ifdef __WATCOMC__
    unsigned rc, count, folder_count, choice, page = 0, pages, first, last;
    int input_result;
    char names[128][13], folders[12][13], input[20], item[24];
    for (;;) {
        count = find_lessons(LESSON_DIRECTORY, names, 128);
        folder_count = find_lesson_folders(folders, 12);
        pages = count ? (count + 31) / 32 : 0;
        if (page >= pages && pages) page = pages - 1;
        first = page * 32; last = first + 32;
        if (last > count) last = count;
        ui_clear(UI_NORMAL); ui_center(1, UI_TITLE, " VOCABULA 0.1 ");
        ui_box(2, 3, 79, 22, UI_NORMAL, "Which lesson would you like to practice?");
        ui_center(2, UI_QUESTION, "Which lesson would you like to practice?");
        for (rc = first; rc < last; ++rc) {
            sprintf(item, "%3u %-12s", rc + 1, names[rc]);
            ui_write(5 + (int)((rc - first) / 16) * 24,
                     5 + (int)((rc - first) % 16), UI_NORMAL, item);
        }
        for (rc = 0; rc < pages; ++rc) {
            sprintf(item, "%c  Lessons %u-%u", (char)('A' + rc), rc * 32 + 1,
                    (rc + 1) * 32 > count ? count : (rc + 1) * 32);
            ui_write(53, 5 + (int)rc, rc == page ? UI_TITLE : UI_INFO, item);
        }
        for (rc = 0; rc < folder_count; ++rc) {
            sprintf(item, "%c  [%-8s]", (char)('A' + pages + rc), folders[rc]);
            ui_write(53, 6 + (int)pages + (int)rc, UI_INFO, item);
        }
        if (!count && !folder_count) {
            ui_center(12, UI_WARN, "No *.VOC lessons or lesson folders found.");
            ui_pause(); return 0;
        }
        ui_write(4, 24, UI_QUESTION, "Number, page or folder letter:");
        input_result = ui_readline(35, 24, 4, input, sizeof(input));
        if (input_result <= 0) return 0;
        if (isalpha((unsigned char)input[0]) && input[1] == '\0') {
            rc = (unsigned)(toupper((unsigned char)input[0]) - 'A');
            if (rc < pages) { page = rc; continue; }
            rc -= pages;
            if (rc < folder_count && select_from_folder(folders[rc], name, size)) return 1;
            continue;
        }
        choice = (unsigned)atoi(input);
        if (choice >= first + 1 && choice <= last) {
            snprintf(name, size, LESSON_DIRECTORY "\\%s", names[choice - 1]);
            return 1;
        }
    }
#else
    ui_clear(UI_NORMAL); ui_box(8, 8, 72, 15, UI_NORMAL, "Open lesson");
    ui_write(12, 11, UI_NORMAL, "Lesson filename:");
    if (!ui_readline(30, 11, 34, name, size)) return 0;
    strip_newline(name);
    if (*name && !strchr(name, '/') && !strchr(name, '\\')) {
        char plain[128]; strncpy(plain, name, sizeof(plain) - 1); plain[sizeof(plain) - 1] = '\0';
        snprintf(name, size, LESSON_DIRECTORY "/%s", plain);
    }
    return *name != '\0';
#endif
}

static void shuffle(unsigned *order, unsigned count)
{
    unsigned i;
    for (i = 0; i < count; ++i) order[i] = i;
    while (count > 1) {
        unsigned j = (unsigned)(rand() % count), temp;
        --count; temp = order[count]; order[count] = order[j]; order[j] = temp;
    }
}

static void side_text(const Side *s, char *text, unsigned size)
{
    unsigned i; text[0] = '\0';
    for (i = 0; i < s->count; ++i) {
        unsigned needed = (unsigned)strlen(text) + (i ? 2 : 0) + (unsigned)strlen(s->text[i]);
        if (needed + 1 >= size) break;
        if (i) strcat(text, ", ");
        strcat(text, s->text[i]);
    }
}

static void show_hint(const Side *s, unsigned hint, char *text, unsigned size)
{
    unsigned i, reveal, length = (unsigned)strlen(s->text[0]);
    reveal = hint * 2; if (reveal > length) reveal = length;
    strcpy(text, "Hint: ");
    for (i = 0; i < length && i + 7 < size; ++i)
        text[i + 6] = i < reveal || isspace((unsigned char)s->text[0][i]) ? s->text[0][i] : '_';
    text[i + 6] = '\0';
}

static void training_screen(const Lesson *lesson, const Side *question, unsigned number,
                            int direction, const SessionStats *stats)
{
    char line[160], bar[31]; unsigned done, filled, i, percent;
    ui_clear(UI_NORMAL); ui_center(1, UI_TITLE, " VOCABULA 0.1 - Training ");
    sprintf(line, "Lesson: %s     Card: %u/%u     Score: %d", lesson->left_name,
            number + 1, lesson->count, stats->score);
    ui_write(3, 3, UI_TITLE, line);
    ui_box(3, 5, 78, 10, UI_NORMAL, direction ? lesson->right_name : lesson->left_name);
    side_text(question, line, sizeof(line)); ui_center(7, UI_FIELD, line);
    ui_box(3, 12, 78, 15, UI_NORMAL, direction ? lesson->left_name : lesson->right_name);
    ui_box(3, 17, 78, 21, UI_NORMAL, "Message");
    ui_write(3, 24, UI_TITLE, "Enter Answer   F1 Hint   F2 Reveal   Esc End session");
    done = stats->asked > lesson->count ? lesson->count : stats->asked;
    percent = lesson->count ? done * 100U / lesson->count : 0;
    filled = percent * 30U / 100U;
    for (i = 0; i < 30; ++i) bar[i] = i < filled ? (char)219 : (char)176;
    bar[30] = '\0';
    sprintf(line, " Progress [%s] %u/%u  %u%% ", bar, done, lesson->count, percent);
    ui_fill(1, 25, 80, UI_FIELD, ' '); ui_center(25, UI_FIELD, line);
}

static int yes_no(int x, int y)
{
    char answer[8]; int result = ui_readline(x, y, 3, answer, sizeof(answer));
    return result > 0 && tolower((unsigned char)answer[0]) == 'y';
}

static int confirm_synonym(const Side *question, const char *translation)
{
    char line[66], greeting[66];
    ui_clear(UI_NORMAL); ui_box(5, 4, 76, 21, UI_NORMAL, "Confirm new synonym");
    sprintf(greeting, "%s, are you sure that", student_name); ui_center(7, UI_QUESTION, greeting);
    strncpy(line, question->text[0], sizeof(line) - 1); line[sizeof(line) - 1] = '\0';
    ui_center(9, UI_FIELD, line);
    ui_center(11, UI_NORMAL, "translates to");
    strncpy(line, translation, sizeof(line) - 1); line[sizeof(line) - 1] = '\0';
    ui_center(13, UI_FIELD, line);
    ui_center(16, UI_QUESTION, "If yes I save it. (y/N)");
    return yes_no(55, 16);
}

static int save_synonym(Lesson *lesson, const char *lesson_name,
                        const Side *question, Side *expected,
                        const char *translation, char *error, unsigned error_size)
{
    if (!*translation || strlen(translation) >= MAX_TEXT ||
        expected->count >= MAX_SYNONYMS) return 0;
    if (!confirm_synonym(question, translation)) return 0;
    strcpy(expected->text[expected->count], translation); ++expected->count;
    if (lesson_save(lesson_name, lesson, error, error_size)) return 1;
    --expected->count;
    return -1;
}

static void show_three_failures(const Side *expected)
{
    char line[66];
    ui_clear(UI_NORMAL); ui_box(5, 5, 76, 20, UI_NORMAL, "Three incorrect answers");
    sprintf(line, "Dear %s, I'm sorry; this was wrong three times.", student_name);
    ui_center(8, UI_WARN, line);
    ui_center(10, UI_NORMAL, "The correct answer would be:");
    strncpy(line, expected->text[0], sizeof(line) - 1); line[sizeof(line) - 1] = '\0';
    ui_center(12, UI_FIELD, line);
    ui_center(15, UI_WARN, "Please type this in now.");
    ui_pause();
}

static int confirm_stop(const SessionStats *stats)
{
    char line[160];
    ui_clear(UI_NORMAL); ui_box(7, 6, 74, 19, UI_NORMAL, "Stop practicing?");
    sprintf(line, "%s, you have answered %u cards and made %u mistakes.", student_name, stats->asked, stats->wrong);
    ui_center(9, UI_NORMAL, line);
    sprintf(line, "This gives you %d points.", stats->score); ui_center(11, UI_NORMAL, line);
    ui_center(14, UI_QUESTION, "Are you sure you want to stop practicing? (y/N)");
    return yes_no(64, 14);
}

static void show_session_status(const SessionStats *stats)
{
    char line[160];
    ui_clear(UI_NORMAL); ui_box(9, 7, 72, 18, UI_NORMAL, "Lesson complete");
    sprintf(line, "Well done, %s! You answered %u cards and made %u mistakes.", student_name, stats->asked, stats->wrong);
    ui_center(10, UI_NORMAL, line);
    sprintf(line, "This gives you %d points.", stats->score); ui_center(13, UI_GOOD, line);
    ui_pause();
}

static int repeat_mistakes(unsigned count)
{
    char line[100];
    if (!count) return 0;
    ui_clear(UI_NORMAL); ui_box(12, 7, 69, 18, UI_NORMAL, "Mistakes");
    sprintf(line, "%s, you have %u card(s) to practice again.", student_name, count); ui_center(10, UI_WARN, line);
    ui_center(13, UI_QUESTION, "Would you like to repeat them? (y/N)");
    return yes_no(59, 13);
}

static void show_high_scores(const char *sum_name)
{
    HighScore scores[MAX_HIGH_SCORES]; unsigned count, i; char line[100];
    count = stats_high_scores(sum_name, scores, MAX_HIGH_SCORES);
    ui_clear(UI_NORMAL); ui_center(1, UI_TITLE, " VOCABULA 0.1 - High scores ");
    ui_box(8, 3, 73, 22, UI_NORMAL, "Best sessions");
    ui_write(10, 5, UI_TITLE, "#  Name              Date/time        Score Right Wrong");
    if (!count) ui_center(12, UI_INFO, "No completed sessions yet.");
    for (i = 0; i < count; ++i) {
        sprintf(line, "%2u %-16.16s %-16s %5d %5u %5u", i + 1, scores[i].name,
                scores[i].date, scores[i].score, scores[i].right, scores[i].wrong);
        ui_write(10, 7 + (int)i, i < 3 ? UI_GOOD : UI_NORMAL, line);
    }
    ui_pause();
}

static void write_wrapped_message(unsigned char attribute, const char *text)
{
    char copy[180], *split;
    strncpy(copy, text, sizeof(copy) - 1); copy[sizeof(copy) - 1] = '\0';
    if (strlen(copy) <= 68) { ui_write(6, 19, attribute, copy); return; }
    split = copy + 68;
    while (split > copy && *split != ' ') --split;
    if (split == copy) split = copy + 68;
    *split++ = '\0'; while (*split == ' ') ++split;
    ui_write(6, 18, attribute, copy); ui_write(6, 19, attribute, split);
}

static void show_random_praise(void)
{
    static const char *messages[] = {
        "Congratulations, well done, this is correct.",
        "Well done, I never thought you would get this one!",
        "Super. It's really fun training with you.",
        "Yes!",
        "Super, I love it!",
        "Perfect, I fall in love with you!",
        "Perfect. You make me obsolete.",
        "Perfect. Do you ever make mistakes?",
        "Excellent. Admit it, are you a writer or a teacher?",
        "Can you not make a mistake, to make me work harder? This was right again!",
        "You win!",
        "Cool. Tell your mum!",
        "This is 1+!"
    };
    char message[180]; unsigned count = sizeof(messages) / sizeof(messages[0]);
    sprintf(message, "%s: %s", student_name, messages[rand() % count]);
    write_wrapped_message(UI_GOOD, message);
}

static int ask_card(Lesson *lesson, const char *lesson_name, unsigned number, int direction,
                    SessionStats *stats, const char *car_name, const char *err_name)
{
    Card *card = lesson->cards[number];
    const Side *question = direction ? &card->right : &card->left;
    Side *expected = direction ? &card->left : &card->right;
    char input[INPUT_SIZE], choice[20], save_error[160], message[180], last_wrong[MAX_TEXT];
    unsigned hints = 0, factual_tries = 0;
    int may_add = 1, input_result, revealed = 0, save_result; AnswerResult assessment;
    last_wrong[0] = '\0';
    for (;;) {
        training_screen(lesson, question, number, direction, stats);
        ui_write(5, 13, UI_QUESTION, "Answer:");
        input_result = ui_readline(14, 13, 61, input, sizeof(input));
        if (input_result <= 0) return -1;
        if ((unsigned char)input[0] == 27) return -1;
        if (input_result == UI_INPUT_F1 || !strcmp(input, "?")) {
            ++hints; ++stats->hints; --stats->score; show_hint(expected, hints, message, sizeof(message));
            ui_write(6, 19, UI_INFO, message); ui_pause(); continue;
        }
        if (input_result == UI_INPUT_F2 || !strcmp(input, "!")) {
            revealed = 1; may_add = 0;
            ui_sound_wrong();
            side_text(expected, message, sizeof(message));
            ui_write(6, 18, UI_WARN, "Let's learn this one together. The answer is:");
            ui_write(6, 19, UI_WARN, message);
            ui_write(6, 20, UI_QUESTION, "Please remember it, then type it in yourself."); ui_pause();
            strcpy(last_wrong, "[revealed]"); continue;
        }
        answer_assess(input, expected, &assessment);
        if (assessment.kind == ANSWER_SPELLING) {
            ui_sound_minor_error();
            sprintf(message, "%s, nearly right! Correct spelling: %s", student_name, expected->text[assessment.match]);
            ui_write(6, 18, UI_WARN, message);
            ui_write(6, 19, UI_WARN, "Please retype it, or press F3 if your answer was correct.");
            input_result = ui_readline(76, 20, 1, choice, sizeof(choice));
            if (input_result == UI_INPUT_F3 && may_add) {
                save_result = save_synonym(lesson, lesson_name, question, expected,
                                           input, save_error, sizeof(save_error));
                if (save_result > 0) {
                    ++stats->asked; ++stats->right; stats->score += hints ? 1 : 2;
                    sprintf(message, "%s, thank you! I saved it and counted it correct.", student_name);
                    ui_write(6, 19, UI_GOOD, message); ui_pause();
                    stats_record_card(car_name, number, 1); return 1;
                }
                if (save_result < 0) {
                    strcpy(message, "Could not save synonym: ");
                    strncat(message, save_error, sizeof(message) - strlen(message) - 1);
                    ui_write(6, 19, UI_WARN, message); ui_pause();
                }
            }
            continue;
        }
        if (assessment.kind == ANSWER_WRONG) {
            ui_sound_wrong();
            ++factual_tries;
            strncpy(last_wrong, input, sizeof(last_wrong) - 1); last_wrong[sizeof(last_wrong) - 1] = '\0';
            if (factual_tries < 3) {
                ui_write(6, 18, UI_ERROR, "Oh no, this was wrong. Let's try again?");
                ui_write(6, 19, UI_NORMAL, "If you think it's right after all, press F3. Enter retries.");
                input_result = ui_readline(76, 20, 1, choice, sizeof(choice));
                if (input_result == UI_INPUT_F3 && may_add) {
                    save_result = save_synonym(lesson, lesson_name, question, expected,
                                               input, save_error, sizeof(save_error));
                    if (save_result > 0) {
                        ++stats->asked; ++stats->right; stats->score += hints ? 1 : 2;
                        sprintf(message, "%s, thank you! I saved it and counted it correct.", student_name);
                        ui_write(6, 19, UI_GOOD, message); ui_pause();
                        stats_record_card(car_name, number, 1); return 1;
                    }
                    if (save_result < 0) {
                        strcpy(message, "Could not save synonym: ");
                        strncat(message, save_error, sizeof(message) - strlen(message) - 1);
                        ui_write(6, 19, UI_WARN, message); ui_pause();
                    }
                }
            }
            if (factual_tries >= 3) {
                may_add = 0; show_three_failures(expected);
            }
            continue;
        }
        if (assessment.kind == ANSWER_TYPO) ui_sound_minor_error();
        else ui_sound_correct();
        if (factual_tries || revealed) {
            ++stats->asked; ++stats->wrong; stats->score -= 2;
            sprintf(message, "%s, that's correct now! We'll practice it once more.", student_name);
            ui_write(6, 18, UI_GOOD, message); ui_pause();
            stats_record_card(car_name, number, 0);
            stats_record_error(err_name, number, question->text[0], last_wrong, expected);
            return 0;
        }
        ++stats->asked; ++stats->right; stats->score += hints ? 1 : 2;
        if (assessment.kind == ANSWER_CAPITALIZATION)
            sprintf(message, "%s, correct! Please note the capitals: %s", student_name, expected->text[assessment.match]);
        else if (assessment.kind == ANSWER_APOSTROPHE)
            sprintf(message, "%s, correct! Please use this apostrophe: %s", student_name, expected->text[assessment.match]);
        else if (assessment.kind == ANSWER_TYPO)
            sprintf(message, "%s, correct! That was only a typo: %s", student_name, expected->text[assessment.match]);
        else { show_random_praise(); ui_pause(); stats_record_card(car_name, number, 1); return 1; }
        ui_write(6, 19, UI_GOOD, message); ui_pause();
        stats_record_card(car_name, number, 1); return 1;
    }
}

int main(int argc, char **argv)
{
    Lesson *lesson; unsigned *order, *mistakes; unsigned i, mistake_count = 0;
    char filename[128], error[160], sum_name[140], car_name[140], err_name[140], input[20];
    int mode, result, stop_round, repeated = 0; SessionStats stats = {0,0,0,0,0};
    ui_init();
    if (!prepare_lesson_directory()) { ui_done(); return 1; }
    ask_student_name();
    if (argc > 1) {
        FILE *supplied;
        strncpy(filename, argv[1], sizeof(filename)-1); filename[sizeof(filename)-1]='\0';
        supplied = fopen(filename, "rt");
        if (supplied) fclose(supplied);
        else if (!strchr(filename, '/') && !strchr(filename, '\\')) {
            char plain[128]; strncpy(plain, filename, sizeof(plain) - 1); plain[sizeof(plain) - 1] = '\0';
            snprintf(filename, sizeof(filename), LESSON_DIRECTORY "\\%s", plain);
        }
    }
    else if (!select_lesson(filename, sizeof(filename))) { ui_done(); return 1; }
    lesson = (Lesson *)malloc(sizeof(Lesson));
    if (!lesson) { ui_done(); puts("Not enough memory for lesson."); return 1; }
    if (!lesson_load(filename, lesson, error, sizeof(error))) { ui_done(); puts(error); free(lesson); return 1; }
    order = (unsigned *)malloc(lesson->count * sizeof(unsigned));
    mistakes = (unsigned *)malloc(lesson->count * sizeof(unsigned));
    if (!order || !mistakes) { ui_done(); puts("Not enough memory for session."); free(order); free(mistakes); free(lesson); return 1; }
    ui_clear(UI_NORMAL); ui_center(4, UI_QUESTION, "How should we practice?");
    ui_box(8, 6, 73, 19, UI_NORMAL, "How should we practice?");
    sprintf(error, "1  From %.24s to %.24s", lesson->left_name, lesson->right_name);
    ui_write(12, 9, UI_NORMAL, error);
    sprintf(error, "2  From %.24s to %.24s", lesson->right_name, lesson->left_name);
    ui_write(12, 11, UI_NORMAL, error);
    sprintf(error, "3  Mixed %.22s / %.22s", lesson->left_name, lesson->right_name);
    ui_write(12, 13, UI_NORMAL, error); ui_write(21, 16, UI_QUESTION, "Mode:");
    if (!ui_readline(29, 16, 3, input, sizeof(input))) { ui_done(); return 1; }
    mode = atoi(input); if (mode < 1 || mode > 3) { ui_done(); puts("Invalid mode."); return 1; }
    srand((unsigned)time(NULL)); shuffle(order, lesson->count); stats_names(filename, sum_name, car_name, err_name);
    stop_round = 0;
    for (i = 0; i < lesson->count && !stop_round; ++i) {
        int direction = mode == 2 || (mode == 3 && (rand() & 1));
        do {
            result = ask_card(lesson, filename, order[i], direction, &stats, car_name, err_name);
            if (result < 0 && confirm_stop(&stats)) stop_round = 1;
        } while (result < 0 && !stop_round);
        if (stop_round) break;
        if (!result) mistakes[mistake_count++] = order[i];
    }
    if (!stop_round) show_session_status(&stats);
    stop_round = 0;
    if (repeat_mistakes(mistake_count)) {
        unsigned new_count = 0;
        repeated = 1;
        ui_clear(UI_NORMAL); ui_box(18, 9, 63, 16, UI_NORMAL, "Repeat mistakes");
        sprintf(error, "%u card(s) will be repeated.", mistake_count); ui_center(12, UI_INFO, error); ui_pause();
        for (i = 0; i < mistake_count; ++i) {
            int direction = mode == 2 || (mode == 3 && (rand() & 1));
            do {
                result = ask_card(lesson, filename, mistakes[i], direction, &stats, car_name, err_name);
                if (result < 0 && confirm_stop(&stats)) stop_round = 1;
            } while (result < 0 && !stop_round);
            if (stop_round) break;
            if (!result) mistakes[new_count++] = mistakes[i];
        }
        mistake_count = new_count;
    }
    if (repeated) show_session_status(&stats);
    stats_save_session(sum_name, student_name, &stats);
    show_high_scores(sum_name); ui_done();
    free(order); free(mistakes); lesson_free(lesson); free(lesson); return 0;
}
