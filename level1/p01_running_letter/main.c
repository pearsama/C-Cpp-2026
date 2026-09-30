#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#ifdef _WIN32
#include <windows.h>
#define sleep_ms(ms) Sleep(ms)
#else
#include <unistd.h>
#include <sys/ioctl.h>
#define sleep_ms(ms) usleep((ms)*1000)
#endif

static int term_width(void) {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi))
        return csbi.srWindow.Right - csbi.srWindow.Left + 1;
#else
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0)
        return ws.ws_col;
#endif
    return 80;
}

static void cleanup(int sig) {
    (void)sig;
    fputs("\x1b[?25h\x1b[0m\x1b[5;1H", stdout);
    fflush(stdout);
    _exit(0);
}

int main(void) {
    char text[128];
    int len, w, x = 0, dir = 1;

    printf("输入一个要弹跳的字符串: ");
    if (!fgets(text, sizeof(text), stdin)) return 1;
    text[strcspn(text, "\r\n")] = '\0';
    if ((len = (int)strlen(text)) == 0) return 1;

#ifdef _WIN32
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode;
    if (GetConsoleMode(h, &mode))
        SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif
    signal(SIGINT, cleanup);

    w = term_width();
    if (len >= w) { len = w - 1; text[len] = '\0'; }

    fputs("\x1b[2J\x1b[?25l", stdout);
    for (;;) {
        printf("\x1b[3;%dH%*s", x + 1, len, "");
        x += dir;
        if (x <= 0 || x + len >= w) {
            dir = -dir;
            x = x < 0 ? 0 : (x + len > w ? w - len : x);
        }
        printf("\x1b[3;%dH%s", x + 1, text);
        fflush(stdout);
        sleep_ms(80);
    }
}