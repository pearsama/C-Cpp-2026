#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <conio.h>
#include <io.h>
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

#define ROWS 7
#define COLS 9
#define BEST 10

enum { UP = 0, DOWN = 1, LEFT = 2, RIGHT = 3, QUIT = 4, RESTART = 5, NONE = 6 };
enum { MOVED = 0, BLOCKED = 1, WIN = 2 };

static const char MAP[ROWS][COLS + 1] = {
    "#########",
    "#@#     #",
    "# ### ###",
    "#   #   #",
    "### ### #",
    "#      E#",
    "#########"
};

static char g[ROWS][COLS + 1];
static int pr, pc, sr, sc, er, ec, steps, won;

static void setup(void) {
#ifdef _WIN32
    DWORD m = 0;
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleOutputCP(65001);
    if (GetConsoleMode(h, &m)) SetConsoleMode(h, m | 0x0004);
#endif
}

static void clear(void) {
#ifdef _WIN32
    system("cls");
#else
    printf("\033[2J\033[H");
#endif
}

static void reset(void) {
    for (int r = 0; r < ROWS; r++) {
        strcpy(g[r], MAP[r]);
        for (int c = 0; c < COLS; c++) {
            if (g[r][c] == '@') { sr = r; sc = c; g[r][c] = ' '; }
            if (g[r][c] == 'E') { er = r; ec = c; }
        }
    }
    pr = sr;
    pc = sc;
    steps = 0;
    won = 0;
}

static int is_wall(int r, int c) {
    return r < 0 || r >= ROWS || c < 0 || c >= COLS || g[r][c] == '#';
}

static int move(int d) {
    static const int dr[4] = {-1, 1, 0, 0};
    static const int dc[4] = {0, 0, -1, 1};

    if (won) return WIN;
    if (is_wall(pr + dr[d], pc + dc[d])) return BLOCKED;

    g[pr][pc] = '.';
    pr += dr[d];
    pc += dc[d];
    steps++;

    if (pr == er && pc == ec) {
        won = 1;
        return WIN;
    }
    return MOVED;
}

static void draw(const char *msg) {
    clear();
    printf("迷宫小游戏 —— 走到 'E' 就赢\n\n");
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++)
            putchar((r == pr && c == pc) ? '@' : g[r][c]);
        putchar('\n');
    }
    printf("\n步数: %d\n", steps);
    printf("[↑↓←→ / WASD] 移动    [R] 重开    [Q] 退出\n");
    if (msg && msg[0]) printf("%s\n", msg);
    fflush(stdout);
}

static int from_char(int ch) {
    switch (ch) {
        case 'w': case 'W': return UP;
        case 's': case 'S': return DOWN;
        case 'a': case 'A': return LEFT;
        case 'd': case 'D': return RIGHT;
        case 'r': case 'R': return RESTART;
        case 'q': case 'Q': return QUIT;
        default: return NONE;
    }
}

static int read_key(void) {
#ifdef _WIN32
    int ch = _getch();
    if (ch == 0 || ch == 224) {
        switch (_getch()) {
            case 72: return UP;
            case 80: return DOWN;
            case 75: return LEFT;
            case 77: return RIGHT;
            default: return NONE;
        }
    }
    if (ch == 3 || ch == EOF) return QUIT;
#else
    struct termios old, raw;
    if (tcgetattr(0, &old) != 0) return QUIT;
    raw = old;
    raw.c_lflag &= ~(unsigned int)(ICANON | ECHO);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    tcsetattr(0, TCSANOW, &raw);

    int ch = getchar();
    if (ch == 27) {
        int c2 = getchar();
        int c3 = (c2 == '[') ? getchar() : 0;
        tcsetattr(0, TCSANOW, &old);
        if (c2 != '[') return NONE;
        switch (c3) {
            case 'A': return UP;
            case 'B': return DOWN;
            case 'C': return RIGHT;
            case 'D': return LEFT;
            default: return NONE;
        }
    }
    tcsetattr(0, TCSANOW, &old);
    if (ch == 3 || ch == EOF) return QUIT;
#endif
    return from_char(ch);
}

static int read_key_plain(void) {
    int ch = getchar();
    if (ch == EOF) return QUIT;
    return from_char(ch);
}

static int stdin_tty(void) {
#ifdef _WIN32
    return _isatty(_fileno(stdin));
#else
    return isatty(0);
#endif
}

int main(void) {
    setup();
    reset();

    int tty = stdin_tty();
    char msg[128];

    while (1) {
        draw("");
        int k = tty ? read_key() : read_key_plain();

        if (k == QUIT) {
            printf("\n已退出。\n");
            return 0;
        }
        if (k == RESTART) {
            reset();
            continue;
        }
        if (k == NONE || k > 3) continue;

        if (move(k) == BLOCKED) {
            draw("撞墙了，换条路。");
            continue;
        }
        if (won) {
            snprintf(msg, sizeof(msg), ">>> 你赢了！共 %d 步（最短 %d 步）<<<", steps, BEST);
            draw(msg);
            printf("按 [R] 再来一局，按 [Q] 退出。\n");
            fflush(stdout);
            while (1) {
                int k2 = tty ? read_key() : read_key_plain();
                if (k2 == QUIT) {
                    printf("\n已退出。\n");
                    return 0;
                }
                if (k2 == RESTART) {
                    reset();
                    break;
                }
            }
        }
    }
}
