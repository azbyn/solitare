#include <curses.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>

#include <array>
#include <string>

#define COLOR_ORANGE 16
#define COLOR_BASE00 0
#define COLOR_BASE01 18
#define COLOR_BASE02 19
#define COLOR_BASE03 8
#define COLOR_BASE04 20
#define COLOR_BASE05 7
#define COLOR_BASE06 21
#define COLOR_BASE07 15

#define LEN(x) (sizeof(x) / sizeof(*x))

// clang-format off
enum class Piece { I, L, J, O, S, T, Z, Size };
// clang-format on
constexpr const char PieceRotations[(int)Piece::Size][16 * 4+1] = {
    // I
    "...." "..x." "...." ".x.."
    "xxxx" "..x." "...." ".x.."
    "...." "..x." "xxxx" ".x.."
    "...." "..x." "...." ".x..",
    // L
    "x..." ".xx." "...." ".x.."
    "xxx." ".x.." "xxx." ".x.."
    "...." ".x.." "..x." "xx.."
    "...." "...." "...." "....",
    // J
    "..x." ".x.." "...." "xx.."
    "xxx." ".x.." "xxx." ".x.."
    "...." ".xx." "x..." ".x.."
    "...." "...." "...." "....",
    // O
    ".xx." ".xx." ".xx." ".xx."
    ".xx." ".xx." ".xx." ".xx."
    "...." "...." "...." "...."
    "...." "...." "...." "....",
    // S
    ".xx." ".x.." "...." "x..."
    "xx.." ".xx." ".xx." "xx.."
    "...." "..x." "xx.." ".x.."
    "...." "...." "...." "....",
    // T
    ".x.." ".x.." "...." ".x.."
    "xxx." ".xx." "xxx." "xx.."
    "...." ".x.." ".x.." ".x.."
    "...." "...." "...." "....",
    // Z
    "xx.." "..x." "...." ".x.."
    ".xx." ".xx." "xx.." "xx.."
    "...." ".x.." ".xx." "x..."
    "...." "...." "...." "....",
};

enum Pairs {
    PAIR_BG = (int)Piece::Size,
    PAIR_FIELD0,
    PAIR_FIELD1,
    PAIR_BORDER,
    PAIR_TEXT,
};
constexpr int Width = 10;
constexpr int Height = 20;

constexpr int LeftPad = 2;

int score = 420;
uint8_t _board[24 * 10];
uint8_t& board(int x, int y) { return _board[y * 10 + x]; }
//std::bitset<Width> board[40];
struct {
    int x = 5;
    int y = 5;
    int rotation = 0;
    Piece piece = Piece::T;
} player;

static void finish(int sig);
void coladdstr(short col, const char* str) {
    attron(COLOR_PAIR(col));
    addstr(str);
}
void coladdstr(Piece p, const char* str) {
    coladdstr((short)p, str);
}

int main() {
    signal(SIGINT, finish); /* arrange interrupts to terminate */

    initscr(); /* initialize the curses library */
    keypad(stdscr, true); /* enable keyboard mapping */
    nonl(); /* tell curses not to do NL->CR/NL on output */
    cbreak(); /* take input chars one at a time, no wait for \n */
    noecho();
    nodelay(stdscr, true);
    meta(stdscr, true);
    curs_set(0);
    //putenv("ESCDELAY=25");
    if (COLS < 2 * Width + 15 + LeftPad || LINES < Height + 1) {
        fprintf(stderr, "terminal too small %dx%d", COLS, LINES);
        finish(0);
    }

    if (has_colors()) {
        start_color();
        constexpr short bgColor = COLOR_BASE00;
        auto addPiece = [](Piece p, short fg) { init_pair((int)p+1, fg, fg); };
        auto addColor = [](int i, short col) { init_pair(i, col, col); };
        addPiece(Piece::I, COLOR_CYAN);
        addPiece(Piece::L, COLOR_BLUE);
        addPiece(Piece::J, COLOR_ORANGE);
        addPiece(Piece::O, COLOR_YELLOW);
        addPiece(Piece::S, COLOR_GREEN);
        addPiece(Piece::T, COLOR_MAGENTA);
        addPiece(Piece::Z, COLOR_RED);
        addColor(PAIR_BG, bgColor);
        addColor(PAIR_FIELD0, COLOR_BASE00);
        addColor(PAIR_FIELD1, COLOR_BASE01);
        addColor(PAIR_BORDER, COLOR_BASE03);
        init_pair(PAIR_TEXT, COLOR_WHITE, bgColor);
    }
    const std::string verticalBorder = std::string(Width * 2 + 4, ' ');
    for (;;) {
//Input
#if 0
        int c = getch(); /* refresh, accept single keystroke of input */
        switch (c) {
        case ERR: continue;
        case KEY_BACKSPACE:
        case 27:
            finish(0);
            break;
        case KEY_UP:
        case 'w':
            break;
        case KEY_DOWN:
        case 's':
            break;
        case KEY_LEFT:
        case 'a':
            break;
        case KEY_RIGHT:
        case 'd':
            break;
        default:
            printf("%x ", c);
            break;
        }
#endif
        board(0, 0) = 1;
        board(9, 9) = 2;
        board(8, 1) = 3;
        board(5, 20) = 4;
        board(5, 19) = 4;
        //Draw
        move(0, LeftPad);
        coladdstr(PAIR_BORDER, verticalBorder.c_str());
        for (int y = 0; y < Height; ++y) {
            move(Height - y - 1, LeftPad);
            coladdstr(PAIR_BORDER, "  ");
            for (int x = 0; x < Width; ++x) {
                /*uint8_t col;
                int x = 5;
                int y = 5;
                int rotation = 0;
                Piece piece = Piece::T;*/
                auto col = board(x, y);
                coladdstr(col ? col : (x % 2 ? PAIR_FIELD0 : PAIR_FIELD1), "  ");
            }
            coladdstr(PAIR_BORDER, "  ");
        }
        move(Height, LeftPad);
        coladdstr(PAIR_BORDER, verticalBorder.c_str());

        attron(COLOR_PAIR(PAIR_TEXT));
        mvprintw(2, LeftPad + (Width * 2) + 5, "Score:");
        mvprintw(3, LeftPad + (Width * 2) + 5, "%d", score);
        while (getch() == -1)
            ;
        break;
    }

    finish(0);
}

static void finish(int) {
    endwin();

    exit(0);
}
