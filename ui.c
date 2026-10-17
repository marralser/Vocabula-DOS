#include "ui.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

#ifdef __WATCOMC__
#include <dos.h>
static unsigned short far *video;

static unsigned char port_read(unsigned port);
#pragma aux port_read = "in al,dx" parm [dx] value [al] modify exact [al];

static void port_write(unsigned port, unsigned char value);
#pragma aux port_write = "out dx,al" parm [dx] [al];

static void cursor(int x, int y)
{
    union REGS registers;
    registers.h.ah = 2; registers.h.bh = 0;
    registers.h.dh = (unsigned char)(y - 1); registers.h.dl = (unsigned char)(x - 1);
    int86(0x10, &registers, &registers);
}

static int read_key(void)
{
    union REGS registers;
    registers.h.ah = 0; int86(0x16, &registers, &registers);
    return registers.x.ax;
}

void ui_init(void)
{
    union REGS registers;
    registers.h.ah = 0x0f; int86(0x10, &registers, &registers);
    video = (unsigned short far *)MK_FP(registers.h.al == 7 ? 0xb000 : 0xb800, 0);
}

void ui_write(int x, int y, unsigned char attribute, const char *text)
{
    unsigned offset;
    if (y < 1 || y > 25 || x > 80) return;
    if (x < 1) x = 1;
    offset = (unsigned)((y - 1) * 80 + x - 1);
    while (*text && x++ <= 80) video[offset++] = ((unsigned)attribute << 8) | (unsigned char)*text++;
}

void ui_fill(int x, int y, int width, unsigned char attribute, char character)
{
    unsigned offset; int i;
    if (y < 1 || y > 25 || x < 1 || x > 80) return;
    if (x + width > 81) width = 81 - x;
    offset = (unsigned)((y - 1) * 80 + x - 1);
    for (i = 0; i < width; ++i) video[offset + i] = ((unsigned)attribute << 8) | (unsigned char)character;
}
#else
static void cursor(int x, int y) { printf("\033[%d;%dH", y, x); }
void ui_init(void) { }
void ui_write(int x, int y, unsigned char attribute, const char *text)
{ (void)attribute; cursor(x, y); fputs(text, stdout); }
void ui_fill(int x, int y, int width, unsigned char attribute, char character)
{ int i; (void)attribute; cursor(x, y); for (i = 0; i < width; ++i) putchar(character); }
#endif

void ui_clear(unsigned char attribute)
{
    int y;
    for (y = 1; y <= 25; ++y) ui_fill(1, y, 80, attribute, ' ');
    cursor(1, 1); fflush(stdout);
}

void ui_done(void)
{
    ui_clear(0x07); cursor(1, 1);
}

void ui_center(int y, unsigned char attribute, const char *text)
{
    int x = (80 - (int)strlen(text)) / 2 + 1;
    ui_write(x < 1 ? 1 : x, y, attribute, text);
}

void ui_box(int left, int top, int right, int bottom, unsigned char attribute, const char *title)
{
    int x, y; char corner[2]; corner[1] = '\0';
    ui_fill(left + 1, top, right - left - 1, attribute, (char)196);
    ui_fill(left + 1, bottom, right - left - 1, attribute, (char)196);
    for (y = top + 1; y < bottom; ++y) {
        corner[0] = (char)179; ui_write(left, y, attribute, corner); ui_write(right, y, attribute, corner);
        ui_fill(left + 1, y, right - left - 1, attribute, ' ');
    }
    corner[0] = (char)218; ui_write(left, top, attribute, corner);
    corner[0] = (char)191; ui_write(right, top, attribute, corner);
    corner[0] = (char)192; ui_write(left, bottom, attribute, corner);
    corner[0] = (char)217; ui_write(right, bottom, attribute, corner);
    if (title && *title) {
        x = left + 3; ui_write(x, top, attribute, " "); ui_write(x + 1, top, attribute, title);
        ui_write(x + 1 + (int)strlen(title), top, attribute, " ");
    }
}

int ui_readline(int x, int y, int width, char *buffer, unsigned size)
{
    ui_fill(x, y, width, UI_FIELD, ' '); cursor(x, y); fflush(stdout);
#ifdef __WATCOMC__
    {
        unsigned used = 0; int key, ascii, scan; char shown[2]; shown[1] = '\0';
        for (;;) {
            key = read_key();
            ascii = key & 0xff; scan = (key >> 8) & 0xff;
            if ((ascii == 0 || ascii == 0xe0) && scan == 59) { buffer[0] = '\0'; return UI_INPUT_F1; }
            if ((ascii == 0 || ascii == 0xe0) && scan == 60) { buffer[0] = '\0'; return UI_INPUT_F2; }
            if ((ascii == 0 || ascii == 0xe0) && scan == 61) { buffer[0] = '\0'; return UI_INPUT_F3; }
            if (ascii == 27) { buffer[0] = '\0'; return UI_INPUT_CANCEL; }
            if (ascii == 13) { buffer[used] = '\0'; return UI_INPUT_ENTER; }
            if (ascii == 8 && used) {
                --used; ui_fill(x + (int)used, y, 1, UI_FIELD, ' ');
                cursor(x + (int)used, y); continue;
            }
            if (ascii >= 32 && ascii <= 255 && used + 1 < size && (int)used < width) {
                buffer[used++] = (char)ascii; shown[0] = (char)ascii;
                ui_write(x + (int)used - 1, y, UI_FIELD, shown); cursor(x + (int)used, y);
            }
        }
    }
#else
    if (!fgets(buffer, size, stdin)) return 0;
    buffer[strcspn(buffer, "\r\n")] = '\0'; return UI_INPUT_ENTER;
#endif
}

void ui_pause(void)
{
    char input[8];
    ui_write(27, 22, UI_INFO, "Press Enter to continue");
    ui_readline(50, 22, 1, input, sizeof(input));
}

static void pause_milliseconds(unsigned milliseconds)
{
    clock_t until = clock() + (clock_t)((unsigned long)CLOCKS_PER_SEC * milliseconds / 1000UL);
    while (clock() < until) { }
}

static void tone(unsigned frequency, unsigned milliseconds)
{
#ifdef __WATCOMC__
    unsigned divisor = 1193180UL / frequency;
    unsigned char speaker = port_read(0x61);
    port_write(0x43, 0xb6);
    port_write(0x42, (unsigned char)(divisor & 0xff));
    port_write(0x42, (unsigned char)(divisor >> 8));
    port_write(0x61, (unsigned char)(speaker | 3));
    pause_milliseconds(milliseconds); port_write(0x61, speaker);
#else
    (void)frequency; fputc('\a', stdout); fflush(stdout); pause_milliseconds(milliseconds);
#endif
}

void ui_sound_correct(void) { tone(880, 90); }
void ui_sound_minor_error(void) { tone(660, 70); pause_milliseconds(70); tone(660, 70); }
void ui_sound_wrong(void) { tone(220, 240); }
