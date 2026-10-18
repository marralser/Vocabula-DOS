#ifndef UI_H
#define UI_H

#define UI_NORMAL 0x17
#define UI_TITLE  0x1f
#define UI_FIELD  0x70
#define UI_GOOD   0x2f
#define UI_WARN   0x4f
#define UI_INFO   0x3f
#define UI_QUESTION 0x1e
#define UI_ERROR  0x1c

#define UI_INPUT_CANCEL -1
#define UI_INPUT_EOF     0
#define UI_INPUT_ENTER   1
#define UI_INPUT_F1      2
#define UI_INPUT_F2      3
#define UI_INPUT_F3      4
#define UI_INPUT_F4      5
#define UI_INPUT_F5      6

void ui_init(void);
void ui_done(void);
void ui_clear(unsigned char attribute);
void ui_box(int left, int top, int right, int bottom, unsigned char attribute, const char *title);
void ui_write(int x, int y, unsigned char attribute, const char *text);
void ui_center(int y, unsigned char attribute, const char *text);
void ui_fill(int x, int y, int width, unsigned char attribute, char character);
int ui_readline(int x, int y, int width, char *buffer, unsigned size);
void ui_pause(void);
void ui_sound_correct(void);
void ui_sound_minor_error(void);
void ui_sound_wrong(void);

#endif
