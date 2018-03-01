#include "point.h"
using azbyn::Point;
#include "misc.h"
using azbyn::Callback;
using azbyn::string_format;
#include "prophanity.h"
using namespace azbyn::prophanity;
using azbyn::Rect;
using azbyn::Rect4;
using azbyn::RectWH;

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <fstream>
#include <random>
#include <stdexcept>
#include <string>
#include <thread>

enum Piece {
    TP_EMPTY = -1,
    TP_I,
    TP_L,
    TP_J,
    TP_O,
    TP_S,
    TP_T,
    TP_Z,
    TP_LEN
};

//generated using generate-table.py
using PiecePoints = std::array<Point, 4>;
constexpr PiecePoints PieceRotations[TP_LEN][4] = {
    {{{{0, 2}, {1, 2}, {2, 2}, {3, 2}}},
     {{{2, 3}, {2, 2}, {2, 1}, {2, 0}}},
     {{{0, 1}, {1, 1}, {2, 1}, {3, 1}}},
     {{{1, 3}, {1, 2}, {1, 1}, {1, 0}}}},
    {{{{0, 3}, {0, 2}, {1, 2}, {2, 2}}},
     {{{1, 3}, {1, 2}, {1, 1}, {2, 3}}},
     {{{0, 2}, {1, 2}, {2, 2}, {2, 1}}},
     {{{0, 1}, {1, 3}, {1, 2}, {1, 1}}}},
    {{{{0, 2}, {1, 2}, {2, 3}, {2, 2}}},
     {{{1, 3}, {1, 2}, {1, 1}, {2, 1}}},
     {{{0, 2}, {0, 1}, {1, 2}, {2, 2}}},
     {{{0, 3}, {1, 3}, {1, 2}, {1, 1}}}},
    {{{{1, 3}, {1, 2}, {2, 3}, {2, 2}}},
     {{{1, 3}, {1, 2}, {2, 3}, {2, 2}}},
     {{{1, 3}, {1, 2}, {2, 3}, {2, 2}}},
     {{{1, 3}, {1, 2}, {2, 3}, {2, 2}}}},
    {{{{0, 2}, {1, 3}, {1, 2}, {2, 3}}},
     {{{1, 3}, {1, 2}, {2, 2}, {2, 1}}},
     {{{0, 1}, {1, 2}, {1, 1}, {2, 2}}},
     {{{0, 3}, {0, 2}, {1, 2}, {1, 1}}}},
    {{{{0, 2}, {1, 3}, {1, 2}, {2, 2}}},
     {{{1, 3}, {1, 2}, {1, 1}, {2, 2}}},
     {{{0, 2}, {1, 2}, {1, 1}, {2, 2}}},
     {{{0, 2}, {1, 3}, {1, 2}, {1, 1}}}},
    {{{{0, 3}, {1, 3}, {1, 2}, {2, 2}}},
     {{{1, 2}, {1, 1}, {2, 3}, {2, 2}}},
     {{{0, 2}, {1, 2}, {1, 1}, {2, 1}}},
     {{{0, 2}, {0, 1}, {1, 3}, {1, 2}}}},
};
constexpr float _speeds[] = {
    1.0,
    0.793,
    0.618,
    0.473,
    0.355,
    0.262,
    0.190,
    0.135,
    0.094,
    0.064,
    0.043,
    0.028,
    0.018,
    0.011,
    0.007,
};
constexpr float speed(int level) { return level > 14 ? .007 : _speeds[level]; }

constexpr int Width = 10;
constexpr int Height = 20;
constexpr int MatrixSizeY = 30;
constexpr int NextPiecesLen = 3;

void waitAFrame() {
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
}
void keyChoice(int a, Callback cbA, int b, Callback cbB) {
    for (;;) {
        auto c = tolower(getch());
        if (c == a) {
            cbA();
            return;
        }
        if (c == b) {
            cbB();
            return;
        }
        waitAFrame();
    }
}

class RandomGenerator {
    std::array<Piece, TP_LEN> bags[2] = {};
    int bagIndex = 0;
    std::random_device rd;
    std::mt19937 gen;
    int index = 0;

public:
    RandomGenerator() : rd(), gen(rd()) {
        for (int i = 0; i < TP_LEN; ++i) {
            bags[0][i] = (Piece)i;
            bags[1][i] = (Piece)i;
        }
        Restart();
    }

    Piece operator()() {
        if (index >= TP_LEN) {
            std::shuffle(bags[bagIndex].begin(), bags[bagIndex].end(), gen);
            bagIndex = !bagIndex;
            index = 0;
        }
        return bags[bagIndex][index++];
    }
    Piece NextPiece(int i) const {
        int ix = index + i;
        if (ix >= TP_LEN)
            return bags[!bagIndex][ix - TP_LEN];
        return bags[bagIndex][ix];
    }
    void Restart() {
        bagIndex = 0;
        index = 0;
        std::shuffle(bags[0].begin(), bags[0].end(), gen);
        std::shuffle(bags[1].begin(), bags[1].end(), gen);
    }

} randgen;

constexpr char HighscoresFile[] = "highscores";

class Game {
    int highscore = 0;
    int score = 0;
    int clearedLinesThisLevel = 0;
    int totalClearedLines = 0;
    int level = 0;
    uint8_t matrix[MatrixSizeY * Width];
    bool running = true;
    Callback pause;
    int RequiredLines() const { return (level + 1) * 5; }

    void ClearLine(int y) {
        ++totalClearedLines;
        if (++clearedLinesThisLevel >= RequiredLines()) {
            ++level;
            clearedLinesThisLevel = 0;
        }
        memcpy(matrix + y * Width,
               matrix + (y + 1) * Width,
               sizeof(*matrix) * Width * (MatrixSizeY - y - 1));
        memset(matrix + (MatrixSizeY - 1) * Width, 0, Width);
    }
    void ReadHighscore() {
        std::ifstream f(HighscoresFile);
        f >> highscore;
    }
    void WriteHighscore() {
        std::ofstream f(HighscoresFile);
        f << highscore << "\n";
    }

public:
    Game() {
        ReadHighscore();
        Restart();
    }
    ~Game() { WriteHighscore(); }
    void Restart() {
        score = 0;
        clearedLinesThisLevel = 0;
        totalClearedLines = 0;
        level = 0;
        memset(matrix, 0, sizeof(matrix));
        running = true;
    }

    void IncreaseScore(int x) {
        score += x;
        if (highscore < score) {
            highscore = score;
        }
    }
    inline void Init(Callback pause) { this->pause = pause; }
    inline void End() { running = false; }
    inline void Pause() { pause(); }

    inline uint8_t& Matrix(Point p) { return matrix[p.y * Width + p.x]; }
    inline uint8_t& Matrix(int x, int y) { return matrix[y * Width + x]; }
    inline float Speed() const { return speed(level); }
    inline int ClearedLines() const { return totalClearedLines; }
    inline int GoalLines() const { return RequiredLines() - clearedLinesThisLevel; }
    inline int Score() const { return score; }
    inline int Highscore() const { return highscore; }
    inline bool HasHighscore() const { return score == highscore; }
    inline int Level() const { return level + 1; }
    inline bool Running() const { return running; }
    void CheckClearLines() {
        unsigned y = 0;
        int clearedLines = 0;
        while (y < MatrixSizeY) {
            for (unsigned x = 0; x < Width; ++x) {
                if (!Matrix(x, y))
                    goto nextLine;
            }
            ClearLine(y);
            ++clearedLines;
            continue; //redo line
        nextLine:
            ++y;
        }
        switch (clearedLines) {
        case 1: IncreaseScore(Level() * 40); break;
        case 2: IncreaseScore(Level() * 100); break;
        case 3: IncreaseScore(Level() * 300); break;
        case 4: IncreaseScore(Level() * 1200); break;
        }
    }
} game;

class Player {
    PiecePoints points;
    PiecePoints ghostPoints;
    Point pos;
    int rotation;
    Piece pieceType;
    std::chrono::time_point<std::chrono::system_clock> lastDrop;
    Piece holdPiece = TP_EMPTY;
    bool lastWasHold = false;

    void UpdatePtsBase(PiecePoints& pts, Point p) const {
        for (int i = 0; i < 4; ++i) {
            pts[i] = PieceRotations[pieceType][rotation][i] + p;
        }
    }
    void UpdatePoints() { UpdatePtsBase(points, pos); }
    void UpdateGhostPoints() {
        Point p = pos;
        for (;;) {
            for (int i = 0; i < 4; ++i) {
                ghostPoints[i] = PieceRotations[pieceType][rotation][i] + p;
                if (game.Matrix(ghostPoints[i]) || ghostPoints[i].y < 0) {
                    ++p.y;
                    UpdatePtsBase(ghostPoints, p);
                    return;
                }
            }
            --p.y;
        }
    }
    void Reset(Piece p) {
        if (p == TP_I && game.Matrix(4, Height - 1)) game.End();
        pos = Point(3, Height - 3);
        rotation = 0;
        pieceType = p;
        UpdatePoints();
        for (auto pt : points) {
            if (game.Matrix(pt)) {
                for (auto p : points) {
                    if (p.y >= Height) {
                        ++pos.y; //just visual
                        UpdatePoints();
                        game.End();
                        return;
                    }
                }
            }
        }
        UpdateGhostPoints();
        ResetDrop();
    }

public:
    Player(Piece p) { Reset(p); }
    Player() : Player(randgen()) {}
    void Restart() {
        Reset(randgen());
        holdPiece = TP_EMPTY;
        lastWasHold = false;
    }
    inline const PiecePoints& GetPoints() const { return points; }
    inline const PiecePoints& GetGhostPoints() const { return ghostPoints; }
    inline Piece GetPieceType() const { return pieceType; }
    inline Piece GetHoldPiece() const { return holdPiece; }
    void Hold() {
        if (holdPiece == TP_EMPTY) {
            holdPiece = pieceType;
            Reset(randgen());
        }
        else if (!lastWasHold) {
            Piece tmp = pieceType;
            Reset(holdPiece);
            holdPiece = tmp;
        }
        lastWasHold = true;
    }

    void Move(int x) {
        pos.x += x;
        PiecePoints bk = points;
        UpdatePoints();
        for (auto& pt : points) {
            if (game.Matrix(pt) != 0 || pt.x < 0 || pt.x >= Width) {
                pos.x -= x;
                points = bk;
                return;
            }
        }
        UpdateGhostPoints();
    }
    void Fall() {
        --pos.y;
        UpdatePoints();
        for (auto pt : points) {
            if (game.Matrix(pt) || pt.y < 0) {
                ++pos.y;
                UpdatePoints();
                PlaceOnBoard();
                return;
            }
        }
    }
    void SoftDrop() {
        game.IncreaseScore(1);
        ResetDrop();
        Fall();
    }
    void HardDrop() {
        //if (pos.y == Height - 2) return;
        for (;;) {
            --pos.y;
            UpdatePoints();
            for (auto pt : points) {
                if (game.Matrix(pt) || pt.y < 0) {
                    ++pos.y;
                    UpdatePoints();
                    PlaceOnBoard();
                    return;
                }
            }
            game.IncreaseScore(1);
        }
    }
    void Rotate(int i) {
        auto tmprot = rotation;
        rotation += i;
        if (rotation < 0)
            rotation = 3;
        else if (rotation > 3)
            rotation = 0;
        if (CheckRotation())
            return;
        // wall kick right
        auto tmpx = pos.x;
        ++pos.x;
        if (CheckRotation())
            return;
        // wall kick left
        pos.x = tmpx - 1;
        if (CheckRotation())
            return;
        // special cases for I
        if (pieceType == TP_I) {
            pos.x = tmpx + 2;
            if (CheckRotation())
                return;
            pos.x = tmpx - 2;
            if (CheckRotation())
                return;
        }
        // can't wall kick
        pos.x = tmpx;
        rotation = tmprot;
        UpdateGhostPoints();
        UpdatePoints();
    }

private:
    bool CheckRotation() {
        UpdatePoints();
        for (auto pt : points) {
            if (game.Matrix(pt) || pt.x < 0 || pt.x >= Width) {
                return false;
            }
        }
        UpdateGhostPoints();
        return true;
    }

public:
    void Input() {
        switch (tolower(getch())) {
            // case ERR: continue;
        case 'z':
        case 'a':
            Rotate(-1);
            break;
        case KEY_UP:
        case 's':
        case 'x':
            Rotate(1);
            break;
        case 'c':
        case 'd':
            Hold();
            break;
        case KEY_F(1):
        case 'q':
        case 27:
        case 'p':
            game.Pause();
            break;
        case ' ':
            HardDrop();
            break;
        case KEY_DOWN:
            SoftDrop();
            break;
        case KEY_LEFT:
            Move(-1);
            break;
        case KEY_RIGHT:
            Move(1);
            break;
        }
    }

    void PlaceOnBoard(Piece pc) {
        for (auto pt : points) {
            game.Matrix(pt) = pieceType + 1;
        }
        game.CheckClearLines();
        Reset(pc);
        lastWasHold = false;
    }
    inline void PlaceOnBoard() { PlaceOnBoard(randgen()); }

    inline void ResetDrop() {
        lastDrop = std::chrono::system_clock::now();
    }
    void Gravity() {
        auto now = std::chrono::system_clock::now();
        std::chrono::duration<float> d = now - lastDrop;
        if (d.count() >= game.Speed()) {
            lastDrop = now;
            Fall();
        }
    }
} player;

class Graphics {
    enum Pairs {
        PAIR_BG = TP_LEN + 1,
        PAIR_BOARD,
        PAIR_BORDER,
        PAIR_TEXT,
        PAIR_GHOST_PIECE,
    };

    void InitColors() {
        start_color();
        constexpr short bgColor = COL_BLACK;
        auto addPiece = [](Piece p, short fg) { init_pair(p + 1, fg, fg); };
        auto addColor = [](int i, short col) { init_pair(i, col, col); };
        addPiece(TP_I, COL_CYAN);
        addPiece(TP_L, COL_BLUE);
        addPiece(TP_J, COL_ORANGE);
        addPiece(TP_O, COL_YELLOW);
        addPiece(TP_S, COL_GREEN);
        addPiece(TP_T, COL_MAGENTA);
        addPiece(TP_Z, COL_RED);
        addColor(PAIR_BG, bgColor);
        addColor(PAIR_BOARD, COL_BLACK);

        bool isTerm256 = std::string(getenv("TERM")).find("256") != std::string::npos;

        addColor(PAIR_GHOST_PIECE, BASE16 ? 18 : (isTerm256 ? 236 : COL_DARK_GRAY));
        addColor(PAIR_BORDER, isTerm256 ? COL_DARK_GRAY : COL_LIGHT_GRAY);
        init_pair(PAIR_TEXT, COL_DARK_WHITE, bgColor);
    }

public:
    Graphics() {
        initscr(); /* initialize the curses library */
        keypad(stdscr, true); /* enable keyboard mapping */
        nonl(); /* tell curses not to do NL->CR/NL on output */
        cbreak(); /* take input chars one at a time, no wait for \n */
        noecho();
        nodelay(stdscr, true);
        meta(stdscr, true);
        curs_set(0);
        if (COLS < 2 * Width + 15 + LeftPad || LINES < Height + 1) {
            throw std::runtime_error(
                string_format("terminal too small %dx%d", COLS, LINES));
        }
        if (has_colors())
            InitColors();
        DrawBegin();
    }
    void Restart() {
        clear();
        DrawBegin();
    }

    ~Graphics() {
        endwin();
    }
    static constexpr int LeftPad = 10;
    static constexpr int MatrixStartX = LeftPad + 2;
    static constexpr int MatrixEndX = MatrixStartX + 2 * Width;
    void DrawBegin() {
        setcol(PAIR_TEXT);
        mvaddstr(0, 2, "Hold:");
        mvaddstr(0, MatrixEndX + 4, "Next:");

        setcol(PAIR_BORDER);
        addborder(RectWH(2, 2, 4 * 2, 3));
        addborder(RectWH(MatrixEndX + 2, 2,
                         4 * 2, 3 * NextPiecesLen));
        addline(Height, LeftPad, Width * 2 + 4);
        addvline(0, LeftPad, Height);
        addvline(0, MatrixEndX, Height);
    }
    void DrawMatrix() {
        move(0, LeftPad);
        for (int y = 0; y < Height; ++y) {
            move(Height - y - 1, MatrixStartX);
            for (int x = 0; x < Width; ++x) {
                auto col = game.Matrix(x, y);
                coladdstr(col ? col : (uint8_t)PAIR_BOARD, "  ");
            }
        }
    }
    void DrawVal(int y, int x, const char* str, int num) {
        mvprintw(y, x, str);
        mvprintw(y + 1, x, "  %d", num);
    }

    void DrawInfo() {
        setcol(PAIR_TEXT);
        constexpr int x = MatrixEndX + 3;
        DrawVal(Height - 4, 1, "Level:", game.Level());
        DrawVal(Height - 2, 1, "Goal:", game.GoalLines());
        DrawVal(Height - 7, x, "Highscore:", game.Highscore());
        DrawVal(Height - 4, x, "Score:", game.Score());
        DrawVal(Height - 2, x, "Cleared Lines:", game.ClearedLines());
        if (player.GetHoldPiece() != -1)
            DrawPiece(2, 2, player.GetHoldPiece());
        for (int i = 0; i < NextPiecesLen; ++i) {
            DrawPiece(2 + i * 3, MatrixEndX + 2, randgen.NextPiece(i));
        }
    }
    void DrawPiece(int y, int x, Piece p) {
        colfill(PAIR_BG, RectWH(x, y, 8, 3));
        setcol(p + 1);
        for (int i = 0; i < 4; ++i) {
            auto pt = PieceRotations[p][0][i];
            addblock(3 - pt.y + y, pt.x * 2 + x);
        }
    }
    inline void DrawBlock(Point pt) {
        addblock(Height - pt.y - 1, pt.x * 2 + LeftPad + 2);
    }
    void DrawPlayer() {
        setcol(player.GetPieceType() + 1);
        for (auto pt : player.GetPoints()) {
            DrawBlock(pt);
        }
    }

    void DrawGhostPiece() {
        setcol(PAIR_GHOST_PIECE);
        for (auto pt : player.GetGhostPoints()) {
            DrawBlock(pt);
        }
    }

public:
    void Draw() {
        DrawInfo();
        DrawMatrix();
        DrawGhostPiece();
        DrawPlayer();
    }
    inline void DrawPause() {
        DrawScreenBase("Paused", true);
    }

    inline void DrawEndScreen() {
        DrawScreenBase(game.HasHighscore() ?
                           "HIGH SCORE" :
                           "GAME OVER",
                       false);
    }

private:
    void DrawScreenBase(std::string title, bool isPause) {
        addbox(PAIR_BORDER, PAIR_TEXT,
               RectWH(MatrixStartX, 4, Width * 2, 6));
        setcol(PAIR_TEXT);
        DrawAtMiddle(5, title);
        DrawAtMiddle(7, isPause ?
                            "Quit      Resume" :
                            "Quit      Replay");
        DrawAtMiddle(8, "  Q          R  ");
    }

    inline void DrawAtMiddle(int y, std::string s) {
        mvaddstr(y, MatrixStartX + Width - (s.size() / 2), s.c_str());
    }
} graphics;

int main() {
    signal(SIGINT, [](int) { game.End(); exit(0); });
    atexit([] { game.End(); });
    game.Init([] {
        graphics.DrawPause();
        keyChoice('q', [] { exit(0); },
                  'r', [] { /* don't do anything */ });
    });

    for (;;) {
        while (game.Running()) {
            player.Input();
            player.Gravity();
            graphics.Draw();
            waitAFrame();
        }
        graphics.DrawEndScreen();
        keyChoice('q', [] { exit(0); },
                  'r', [] {
                      randgen.Restart();
                      game.Restart();
                      player.Restart();
                      graphics.Restart(); });
    }
    return 0;
}
