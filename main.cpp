#include "point.h"

#include <curses.h>
#include <signal.h>
#include <stdint.h>
//#include <stdlib.h>

#include <array>
#include <algorithm>
#include <string>
#include <stdexcept>
#include <exception>
#include <random>

using azbyn::Point;

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
enum Piece { TP_I, TP_L, TP_J, TP_O, TP_S, TP_T, TP_Z, TP_SIZE };
constexpr const char PieceRotations[TP_SIZE][16 * 4 + 1] = {
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
// clang-format on

enum Pairs {
    PAIR_BG = TP_SIZE + 1,
    PAIR_BOARD0,
    PAIR_BOARD1,
    PAIR_BORDER,
    PAIR_TEXT,
};
constexpr int Width = 10;
constexpr int Height = 20;

constexpr int LeftPad = 2;

int score = 420;
uint8_t _board[24 * 10];

uint8_t& board(Point p) { return _board[p.y * 10 + p.x];}
uint8_t& board(int x, int y) { return _board[y * 10 + x]; }
//std::bitset<Width> board[40];

class RandomGenerator {
    std::array<Piece, TP_SIZE> pieces = {};
    std::random_device rd;
    std::mt19937 gen;
    int i = 0;
public:
    RandomGenerator() : rd(), gen(rd()) {
        for (int i = 0; i < TP_SIZE; ++i){
            pieces[i] = (Piece) i;
        }
        std::shuffle(pieces.begin(), pieces.end(), gen);
    }

    Piece operator()() {
        if (i >= TP_SIZE) {
            std::shuffle(pieces.begin(), pieces.end(), gen);
            i = 0;
        }
        return pieces[i++];
    }


} randgen;

class Player {
private:
    std::array<Point, 4> points = {};
    Point pos;
    int rotation;
    Piece piece;
public:
    Player(Piece p) {
        Reset(p);
    }
    Player() : Player(randgen()) {}
    constexpr const std::array<Point, 4>& GetPoints() const {
        return points;
    }
    constexpr Piece GetPiece() const {
        return piece;
    }
    void Move(int x, int y) {
        pos += Point(x, y);
        UpdatePoints();
    }

    void RotateRight() {
        rotation++;
        FixRotation();
        UpdatePoints();
    }
    void RotateLeft() {
        rotation--;
        FixRotation();
        UpdatePoints();
    }
    void PlaceOnBoard(Piece pc) {
        for (auto& pt : points)
            board(pt) = piece + 1;
        Reset(pc);
    }
    void PlaceOnBoard() {
        PlaceOnBoard(randgen());
    }
private:
    void Reset(Piece p) {
        pos = Point(3, 17);
        rotation = 0;
        piece = p;
        UpdatePoints();
    }

private:
    void FixRotation() {
        if (rotation < 0) rotation = 3;
        else if (rotation > 3) rotation = 0;
    }
    bool Contains(Point point) {
        for (auto& p : points) {
            if (p == point) return true;
        }
        return false;
    }
    void UpdatePoints() {
        const auto& rot = PieceRotations[(int)piece];
        int i = 0;
        for (int x = 0; x < 4; ++x) {
            for (int y = 0; y < 4; ++y) {
                if (rot[(16 * y) + (rotation * 4) + x] == 'x') {
                    points[i++] = pos + Point(x, 3 - y);
                    if (i == 4)
                        return;
                }
            }
        }
        throw std::runtime_error("expected 4 points");
    }
} player;

void finish() {
    endwin();
    exit(0);
}

void coladdstr(short col, const char* str) {
    attron(COLOR_PAIR(col));
    addstr(str);
}

void input() {
    int c = getch(); /* refresh, accept single keystroke of input */
    switch (c) {
        //case ERR: continue;
    case KEY_BACKSPACE:
    case 'q':
    case 27:
        finish();
        break;
    case KEY_UP:
    case 'w':
    case 'z':
        player.RotateLeft();
        break;
    case 'x':
        player.RotateRight();
        break;
    case 'p':
        //pause
        break;
    case ' ':
        player.PlaceOnBoard();
        //hard-drop
        break;
    case KEY_DOWN:
    case 's':
        player.Move(0, -1);
        break;
    case KEY_LEFT:
    case 'a':
        player.Move(-1, 0);
        break;
    case KEY_RIGHT:
    case 'd':
        player.Move(1, 0);
        break;
    default:
        //printf("%x ", c);
        break;
    }
}

int main() {
    signal(SIGINT, [] (int) { finish(); }); /* arrange interrupts to terminate */

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
        finish();
    }
    if (has_colors()) {
        start_color();
        constexpr short bgColor = COLOR_BASE00;
        auto addPiece = [](Piece p, short fg) { init_pair(p+1, fg, fg); };
        auto addColor = [](int i, short col) { init_pair(i, col, col); };
        addPiece(TP_I, COLOR_CYAN);
        addPiece(TP_L, COLOR_BLUE);
        addPiece(TP_J, COLOR_ORANGE);
        addPiece(TP_O, COLOR_YELLOW);
        addPiece(TP_S, COLOR_GREEN);
        addPiece(TP_T, COLOR_MAGENTA);
        addPiece(TP_Z, COLOR_RED);
        addColor(PAIR_BG, bgColor);
        addColor(PAIR_BOARD0, COLOR_BASE00);
        addColor(PAIR_BOARD1, COLOR_BASE01);
        addColor(PAIR_BORDER, COLOR_BASE03);
        init_pair(PAIR_TEXT, COLOR_WHITE, bgColor);
    }

    const std::string verticalBorder = std::string(Width * 2 + 4, ' ');
    for (;;) {
        //Draw
        move(0, LeftPad);
        coladdstr(PAIR_BORDER, verticalBorder.c_str()); // move outside update
        for (int y = 0; y < Height; ++y) {
            move(Height - y - 1, LeftPad);
            coladdstr(PAIR_BORDER, "  ");
            for (int x = 0; x < Width; ++x) {
                auto col = board(x, y);
                coladdstr(col ? col : (x % 2 ? PAIR_BOARD0 : PAIR_BOARD1), "  ");
            }
            coladdstr(PAIR_BORDER, "  ");
        }

        move(Height, LeftPad);
        coladdstr(PAIR_BORDER, verticalBorder.c_str());

        attron(COLOR_PAIR(PAIR_TEXT));
        mvprintw(2, LeftPad + (Width * 2) + 5, "Score:");
        mvprintw(3, LeftPad + (Width * 2) + 5, "%d", score);
        //Draw player
        attron(COLOR_PAIR(player.GetPiece() + 1));
        for (auto& p : player.GetPoints()) {
            mvaddstr(Height - p.y - 1, p.x * 2 + LeftPad + 2, "  ");
        }
        input();
    }

    finish();
}

