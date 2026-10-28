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

#define MAXR 32
#define MAXC 64
#define MAXB 16
#define MAXLV 32

enum { UP = 0, DOWN = 1, LEFT = 2, RIGHT = 3, QUIT = 4, RESTART = 5, NEXT = 6, NONE = 7 };
enum { BLOCK_WALL = 0, MOVED = 1, BLOCK_BOX = 2 };
enum { PLAYING = 0, CLEARED = 1 };

static const int DR[4] = {-1, 1, 0, 0};
static const int DC[4] = {0, 0, -1, 1};

typedef struct {
    char name[128];
    int rows, cols;
    char ter[MAXR][MAXC];
    int sr, sc;
    int nb;
    int sbr[MAXB], sbc[MAXB];
    int best;
} Level;

typedef struct {
    int pr, pc;
    int br[MAXB], bc[MAXB];
    int steps;
} State;

static Level lv[MAXLV];
static int nlv;

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

static void trim(char *s) {
    size_t n = strlen(s);
    while (n && (s[n - 1] == ' ' || s[n - 1] == '\t' || s[n - 1] == '\r')) s[--n] = 0;
    while (s[0] == ' ' || s[0] == '\t') memmove(s, s + 1, strlen(s));
}

static void add_box(Level *L, int r, int c) {
    if (L->nb < MAXB) {
        L->sbr[L->nb] = r;
        L->sbc[L->nb] = c;
        L->nb++;
    }
}

static int load_levels(const char *path) {
    FILE *f = fopen(path, "r");
    char line[MAXC + 32];
    int cur = -1;

    if (!f) return 0;
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\r\n")] = 0;

        if (line[0] == ';') {
            if (cur + 1 >= MAXLV) break;
            cur++;
            memset(&lv[cur], 0, sizeof(lv[cur]));
            snprintf(lv[cur].name, sizeof(lv[cur].name), "%s", line + 1);
            trim(lv[cur].name);
            lv[cur].sr = lv[cur].sc = -1;
            continue;
        }
        if (cur < 0 || line[0] == 0) continue;

        int r = lv[cur].rows;
        if (r >= MAXR) continue;
        int len = (int)strlen(line);
        if (len > lv[cur].cols) lv[cur].cols = len;
        for (int c = 0; c < MAXC; c++) {
            char ch = c < len ? line[c] : '#';
            lv[cur].ter[r][c] = (ch == '#') ? '#' : (ch == '.' || ch == '*' || ch == '+') ? '.' : ' ';
            if (ch == '$' || ch == '*') add_box(&lv[cur], r, c);
            if (ch == '@' || ch == '+') {
                lv[cur].sr = r;
                lv[cur].sc = c;
            }
        }
        lv[cur].rows = r + 1;
    }
    fclose(f);
    return cur + 1;
}

static void load_scores(const char *path) {
    FILE *f = fopen(path, "r");
    char line[256];
    int i = 0;

    if (!f) return;
    while (fgets(line, sizeof(line), f) && i < nlv) {
        char *p = strrchr(line, '\t');
        lv[i].best = p ? atoi(p + 1) : 0;
        i++;
    }
    fclose(f);
}

static void save_scores(const char *path) {
    FILE *f = fopen(path, "w");
    if (!f) return;
    for (int i = 0; i < nlv; i++) fprintf(f, "%s\t%d\n", lv[i].name, lv[i].best);
    fclose(f);
}

static void level_start(int i, State *st) {
    const Level *L = &lv[i];
    st->pr = L->sr;
    st->pc = L->sc;
    st->steps = 0;
    for (int k = 0; k < L->nb; k++) {
        st->br[k] = L->sbr[k];
        st->bc[k] = L->sbc[k];
    }
}

static int inside(const Level *L, int r, int c) {
    return r >= 0 && r < L->rows && c >= 0 && c < L->cols;
}

static int box_at(const State *st, const Level *L, int r, int c) {
    for (int k = 0; k < L->nb; k++)
        if (st->br[k] == r && st->bc[k] == c) return k;
    return -1;
}

static int move(const Level *L, State *st, int d) {
    int nr = st->pr + DR[d], nc = st->pc + DC[d];

    if (!inside(L, nr, nc) || L->ter[nr][nc] == '#') return BLOCK_WALL;

    int bi = box_at(st, L, nr, nc);
    if (bi >= 0) {
        int br = nr + DR[d], bc = nc + DC[d];
        if (!inside(L, br, bc) || L->ter[br][bc] == '#' || box_at(st, L, br, bc) >= 0)
            return BLOCK_BOX;
        st->br[bi] = br;
        st->bc[bi] = bc;
    }
    st->pr = nr;
    st->pc = nc;
    st->steps++;
    return MOVED;
}

static int done_count(const Level *L, const State *st) {
    int n = 0;
    for (int k = 0; k < L->nb; k++)
        if (L->ter[st->br[k]][st->bc[k]] == '.') n++;
    return n;
}

static void draw(const Level *L, int idx, const State *st, const char *msg) {
    clear();
    printf("==============================\n");
    printf(" 推箱子   第 %d/%d 关   %s\n", idx + 1, nlv, L->name);
    printf("==============================\n");
    for (int r = 0; r < L->rows; r++) {
        for (int c = 0; c < L->cols; c++) {
            int bi = box_at(st, L, r, c);
            if (bi >= 0)
                putchar(L->ter[r][c] == '.' ? '*' : '$');
            else if (r == st->pr && c == st->pc)
                putchar(L->ter[r][c] == '.' ? '+' : '@');
            else
                putchar(L->ter[r][c]);
        }
        putchar('\n');
    }
    printf("------------------------------\n");
    printf(" 步数: %d    最佳: ", st->steps);
    if (L->best) printf("%d", L->best);
    else printf("--");
    printf("    箱子归位: %d/%d\n", done_count(L, st), L->nb);
    printf(" [箭头/WASD] 移动   [R] 重开   [Q] 退出\n");
    if (msg && msg[0]) printf(" %s\n", msg);
    fflush(stdout);
}

static int from_char(int ch) {
    switch (ch) {
        case 'w': case 'W': return UP;
        case 's': case 'S': return DOWN;
        case 'a': case 'A': return LEFT;
        case 'd': case 'D': return RIGHT;
        case 'r': case 'R': return RESTART;
        case 'n': case 'N': return NEXT;
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

int main(int argc, char **argv) {
    const char *lpath = (argc > 1) ? argv[1] : "levels.txt";
    const char *spath = (argc > 2) ? argv[2] : "scores.txt";
    State st;
    int idx = 0, phase = PLAYING, tty, cleared_all = 0;
    char msg[160];

    setup();
    nlv = load_levels(lpath);
    if (nlv <= 0) {
        printf("读不到关卡文件: %s\n", lpath);
        return 1;
    }
    for (int i = 0; i < nlv; i++)
        if (lv[i].sr < 0 || lv[i].nb == 0) {
            printf("第 %d 关缺少玩家起点或箱子\n", i + 1);
            return 1;
        }
    load_scores(spath);

    tty = stdin_tty();
    level_start(idx, &st);

    while (1) {
        draw(&lv[idx], idx, &st, msg);
        msg[0] = 0;
        int k = tty ? read_key() : read_key_plain();
        Level *L = &lv[idx];

        if (k == QUIT) {
            save_scores(spath);
            printf("\n已退出。成绩已保存到 %s\n", spath);
            return 0;
        }
        if (k == NEXT) {
            if (phase == CLEARED && idx + 1 < nlv) {
                idx++;
                level_start(idx, &st);
                phase = PLAYING;
            }
            continue;
        }
        if (k == RESTART) {
            level_start(idx, &st);
            phase = PLAYING;
            continue;
        }
        if (phase == CLEARED) continue;
        if (k == NONE || k > 3) continue;

        int res = move(L, &st, k);
        if (res != MOVED) {
            snprintf(msg, sizeof(msg), "%s",
                     res == BLOCK_WALL ? "前面是墙，走不过去。" : "箱子推不动，换个方向。");
            continue;
        }

        if (done_count(L, &st) == L->nb) {
            int improved = (L->best == 0 || st.steps < L->best);
            if (improved) L->best = st.steps;
            save_scores(spath);
            phase = CLEARED;

            int total = 0;
            for (int i = 0; i < nlv; i++) total += lv[i].best;
            cleared_all = 1;
            for (int i = 0; i < nlv; i++)
                if (lv[i].best == 0) cleared_all = 0;

            if (idx + 1 < nlv)
                snprintf(msg, sizeof(msg),
                         ">>> 过关！本关 %d 步（最佳 %d）<<<  按 [N] 下一关，[R] 重玩",
                         st.steps, L->best);
            else if (cleared_all)
                snprintf(msg, sizeof(msg),
                         ">>> 全部通关！本关 %d 步，总分 %d 步 <<<  按 [R] 重玩，[Q] 退出",
                         st.steps, total);
            else
                snprintf(msg, sizeof(msg),
                         ">>> 过关！本关 %d 步（最佳 %d）<<<  按 [R] 重玩，[Q] 退出",
                         st.steps, L->best);
        }
    }
}
