#include "profanity.h"
#include "misc.h"
#include "rect.h"
#include "point.h"
#include "full_size_cards.h"
#include "game.h"

using namespace azbyn::profanity;
using azbyn::string_format;
using azbyn::Rect;
using azbyn::Point;


#include <curses.h>

#include <vector>
#include <array>
#include <stdexcept>
#include <string>
#include <iostream>
#include <algorithm>
#include <thread>
#include <functional>
#include <chrono>

std::function<void()> draw;

struct Player {
    Game& game;
    Point curr = Point(0, -1);
    Point selected = Point(-1, -1);
    
    Player(Game& game) : game(game) {}

    void checkVertBounds(int y = 0) {
        if (curr.y <= -1) {
            curr.y = -1;
            return;
        }

        auto& pile = game.curr.piles[curr.x];
        if (curr.y < (int)pile.shownPoint && y < 0) {
            curr.y = -1;
            return;
        }
        auto sz = pile.cards.size();
        if (sz == 0) {
            if (curr.y > 0) curr.y = 0;
        }
        else {
            if (curr.y < (int)pile.shownPoint) curr.y = pile.shownPoint;
            if (curr.y >= (int)sz) curr.y = sz - 1;
        }
    }
    void moveHor(int x) {
        curr.x += x;
        if (curr.x < 0) curr.x = 0;
        if (curr.x > 6) curr.x = 6;
        checkVertBounds();
    }
    void moveVert(int y) {
        curr.y += y;
        checkVertBounds(y);
    }
    //assumes something is selected
    bool selAutoMove() {
        Card c = getSingleCard();
        if (c.type != 0) {
            int i = 0;
            for (auto& topPile : game.curr.topPiles) {
                if (c.type == topPile.type+1 && (c.type == 1 || c.suit == topPile.suit)) {
                    popSingleCard();
                    topPile = c;
                    curr.y = -1;
                    curr.x = 3 + i;
                    deselect();
                    return true;
                }
                ++i;
            }
            i = 0;
            for (auto& pile : game.curr.piles) {
                auto state = game.curr;
                if (pile.cards.size() == 0 && selected.y == 0) {
                    //don't be a debil and move king constantly
                    continue;
                }
                if (pile.append(c)) {
                    popSingleCard();
                    game.prev = state;
                    curr.x = i;
                    moveVert(25);
                    deselect();
                    return true;
                }
                ++i;
            }
        } else if (selected.y == -1) {//must be fundation
            auto& topPile = game.curr.topPiles[selected.x - 3];
            int i = 0;
            for (auto& pile : game.curr.piles) {
                auto state = game.curr;
                if (pile.append(topPile)) {
                    --topPile.type;
                    game.prev = state;
                    curr.x = i;
                    moveVert(25);
                    deselect();
                    return true;
                }
                ++i;
            }

        } else {
            auto& selectedPile = game.curr.piles[selected.x];
            auto end = selectedPile.end();
            auto sel = selectedPile.begin() + selected.y;
            auto state = game.curr;
            for (int i = 0; i < 7; ++i) {
                if (i == selected.x) continue;
                auto& pile = game.curr.piles[i];
                if (pile.cards.size() == 0 && selected.y == 0) {
                    //don't be a debil and move king constantly
                    continue;
                }
                if (pile.append(sel, end)) {
                    game.prev = state;
                    selectedPile.erase(sel, end);
                    curr.x = i;
                    moveVert(25);
                    deselect();
                    return true;
                }
            }
        }
        deselect();
        return false;
    }
    bool autoMove() {
        for (int i = 0; i < 7; ++i) {
            auto& selectedPile = game.curr.piles[i];
            auto sz = selectedPile.cards.size();
            if (sz == 0) continue;
            selected.x = i;
            selected.y = selectedPile.shownPoint;
            if (selAutoMove()) return true;
        }
        for (int i = 0; i < 7; ++i) {
            auto& selectedPile = game.curr.piles[i];
            auto sz = selectedPile.cards.size();
            if (sz == 0) continue;
            selected.x = i;
            selected.y = sz - 1;
            moveVert(25);
            auto c = selectedPile.cards.back();
            for (int j = 0; j < 4; ++j) {
                auto& topPile = game.curr.topPiles[j];
                if (c.type == topPile.type+1 && (c.type == 1 || c.suit == topPile.suit)) {
                    popSingleCard();
                    topPile = c;
                    curr.y = -1;
                    curr.x = 3 + j;
                    deselect();
                    return true;
                }
            }
        }

        selected = Point(1, -1);//waste
        if (selAutoMove()) return true;

        game.incrementStock();
        curr = Point(0, -1); //stock
        deselect();
        return false;
    }

    void allAutoMoves() {
        auto state = game.curr;
        while(autoMove()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            draw();
        }
        game.prev = state;
    }

    void deselect() {
        selected = Point(-1, -1);
    }
    // returns emptyCard if selected more than 1 card
    // we already know something's selected so no need to check
    Card getSingleCard() const {
        if (selected.y == -1) {
            if (selected.x == 1) { // stockShown
                return game.stockTop();
            }
            //althrough the fundation selects one card, it's not valid for our purpose (adding to the fundation)
            return Card();
        }
        else {
            auto& pileCards = game.curr.piles[selected.x].cards;
            if (pileCards.size() - selected.y == 1) return pileCards.back();
            return Card();
        }
    }
    void popSingleCard() {
        if (selected.y == -1 && selected.x == 1) { // stockShown
            game.saveUndo();
            game.popStockTop();
        }
        else {
            game.saveUndo();
            game.curr.piles[selected.x].popBack();
        }
    }
    void action() {
        auto x = curr.x;
        if (curr.y == -1) {
            if (selected.x == -1) { //aka nothing selected
                if (x == 0) { // stock
                    game.incrementStock();
                }
                else if (x == 1) { // waste
                    if (game.stockTop().type != 0)
                        selected = curr;
                }
                else if (x >= 3) {
                    auto& topPile = game.curr.topPiles[curr.x - 3];
                    if (topPile.type != 0) selected = curr;
                }
            } else {
                if (x >= 3) {
                    auto& topPile = game.curr.topPiles[curr.x - 3];
                    auto card = getSingleCard();
                    if (card.type == topPile.type+1 && (card.type == 1 || card.suit == topPile.suit)) {
                        popSingleCard();
                        topPile = card;
                    } else return;
                } else if (x == 2) return;

                deselect();
            }
        } else {
            auto& pile = game.curr.piles[x];
            if (selected.x == -1) {
                if (pile.cards.size() != 0)
                    selected = curr;

            } else {
                if (selected.y == -1) {
                    if (selected.x == 1) {// waste
                        auto card = game.stockTop();
                        auto state = game.curr;
                        if (pile.append(card)) {
                            game.popStockTop();
                            game.prev = state;
                        }
                       else return;
                    } else { //must be fundation
                        auto& topPile = game.curr.topPiles[selected.x-3];
                        auto state = game.curr;
                        if (pile.append(topPile)) {
                            game.prev = state;
                            --topPile.type;
                        }
                        else return;
                    }
                } else {
                    if (selected.x == x) {
                        if (selected.y == curr.y) deselect();
                        else selected = curr;
                        return;
                    }
                    auto& selectedPile = game.curr.piles[selected.x];
                    auto end = selectedPile.end();
                    auto sel = selectedPile.begin() + selected.y;
                    auto state = game.curr;
                    if (pile.append(sel, end)) {
                        game.prev = state;
                        selectedPile.erase(sel, end);
                    }
                    else return;
                }
                deselect();
            }
        }
    }

    void input() {
        switch (getch()) {
        case ' ':
            action();
            break;
        case 'e':
            if (selected.x != -1)
                selAutoMove();
            break;
        case 'f':
            autoMove();
            break;
        case 'F':
            allAutoMoves();
            break;
        case 'c':
            deselect();
            break;
        case 'u':
        case 'z':
            game.undo();
            checkVertBounds();
            break;
        case 's':
            moveVert(25);//a big number so it moves to the bottom
            break;
        case 'a':
            moveHor(-6);//a big number so it moves to the left
            break;
        case 'd':
            moveHor(6);//a big number so it moves to the right
            break;
        case 'w': {
            if (curr.y <= 0) {
                //throw 1;
                //moveVert(-15);//a big number so it moves to the top
                curr.y = -1;
            } else {
                auto& pile = game.curr.piles[curr.x];
                if (curr.y == (int)pile.shownPoint) {
                    curr.y = -1;
                } else {
                    curr.y = -1;
                    moveVert(1);
                }
            }
        } break;
            //case 27:
        case 'p':
            //if (tolower(getch()) != 'q') return;
            endwin();
            std::cout << "exited normally\n";
            exit(0);
            break;
        case KEY_UP:
            moveVert(-1);
            break;
        case KEY_DOWN:
            moveVert(1);
            break;
        case KEY_LEFT:
            moveHor(-1);
            break;
        case KEY_RIGHT:
            moveHor(1);
            break;
        }
    }
};

struct Graphics {
    enum Pairs {
        PAIR_BG = 1,
        PAIR_CARD_RED,
        PAIR_CARD_BLACK,
        PAIR_CARD_BACK,
        PAIR_CURSOR,
        PAIR_SELECTED,
    };

    void initColors() {
        //std::cout <<"INIT COLS\n";
        start_color();
        auto cardFront = COL_WHITE;//COL_DARK_WHITE;
        auto boardBg = COL_GREEN;

        init_pair(PAIR_BG, COL_BLACK, boardBg);
        init_pair(PAIR_CARD_RED, COL_RED, cardFront);
        init_pair(PAIR_CARD_BLACK, COL_BLACK, cardFront);
        init_pair(PAIR_CARD_BACK, COL_LIGHT_GRAY, COL_BLUE);
        init_pair(PAIR_CURSOR, COL_WHITE, boardBg);
        init_pair(PAIR_SELECTED, COL_DARK_WHITE, boardBg);
    }

    constexpr static int Width = 100;
    constexpr static int Height = 40;
    int leftPad = 0;


    bool smallMode;
    Point cardSize = smallMode ? Point(9, 6) : Point(11, 7);
    Graphics(bool smallMode) : smallMode(smallMode) {
        setlocale(LC_ALL, "");
        initscr(); // initialize the curses library
        keypad(stdscr, true); // enable keyboard mapping
        nonl(); // tell curses not to do NL->CR/NL on output
        cbreak(); // take input chars one at a time, no wait for \n
        noecho();
        //nodelay(stdscr, true);
        meta(stdscr, true);
        curs_set(0);
        /*
        if (COLS < Width || LINES < Height) {
            throw std::runtime_error(
                string_format("terminal too small %dx%d", COLS, LINES));
                }*/
        if (has_colors())
            initColors();
    }
    ~Graphics() {
        endwin();
    }

    template<const full_size_cards::T& s, char lastByte>
    static void printCard(int y, int x) {
        using namespace full_size_cards;
        mvaddstr(y + 0, x, (replaceX<s, 0, lastByte>().data()));
        mvaddstr(y + 1, x, (replaceX<s, 1, lastByte>().data()));
        mvaddstr(y + 2, x, (replaceX<s, 2, lastByte>().data()));
        mvaddstr(y + 3, x, (replaceX<s, 3, lastByte>().data()));
        mvaddstr(y + 4, x, (replaceX<s, 4, lastByte>().data()));
        mvaddstr(y + 5, x, (replaceX<s, 5, lastByte>().data()));
        mvaddstr(y + 6, x, (replaceX<s, 6, lastByte>().data()));
        setcol(PAIR_CARD_BLACK);
        constexpr auto top = replaceX<s, 0, lastByte>();
        // I hate the 10 card
        if (top[1] == '0') {
            mvaddstr(y+0, x+2, "───────┐");
            mvaddstr(y+6, x+1, "└───────");
        } else {
            mvaddstr(y+0, x+1, "┌───────┐");
            mvaddstr(y+6, x+1, "└───────┘");
        }
        for (int i = 1; i < 6; ++i) {
            mvaddstr(y+i, x+1, "│");
            mvaddstr(y+i, x+9, "│");
        }
    }
    template<const full_size_cards::T& s>
    static void printCard(int y, int x, Suit suit) {
        switch (suit) {
        case Suit::Spades:   printCard<s, (char) 0xA0>(y, x); break;
        case Suit::Hearts:   printCard<s, (char) 0xA5>(y, x); break;
        case Suit::Clubs:    printCard<s, (char) 0xA3>(y, x); break;
        case Suit::Diamonds: printCard<s, (char) 0xA6>(y, x); break;
        default: break;
        }
    }
    static void printCard(int y, int x, Card c) {
        using namespace full_size_cards;
        switch (c.type) {
        case 1: {
            switch (c.suit) {
            case Suit::Spades: printCard<arrAS>(y, x, c.suit); break;
            case Suit::Hearts: printCard<arrAH>(y, x, c.suit); break;
            case Suit::Clubs: printCard<arrAC>(y, x, c.suit); break;
            case Suit::Diamonds: printCard<arrAD>(y, x, c.suit); break;
            default:
                throw std::logic_error(string_format("invalid suit %d", (int)c.suit));
            }
        } break;
        case 2: printCard<arr2>(y, x, c.suit); break;
        case 3: printCard<arr3>(y, x, c.suit); break;
        case 4: printCard<arr4>(y, x, c.suit); break;
        case 5: printCard<arr5>(y, x, c.suit); break;
        case 6: printCard<arr6>(y, x, c.suit); break;
        case 7: printCard<arr7>(y, x, c.suit); break;
        case 8: printCard<arr8>(y, x, c.suit); break;
        case 9: printCard<arr9>(y, x, c.suit); break;
        case 10: printCard<arr10>(y, x, c.suit); break;
        case 11: printCard<arrJ>(y, x, c.suit); break;
        case 12: printCard<arrQ>(y, x, c.suit); break;
        case 13: printCard<arrK>(y, x, c.suit); break;
        default:
            throw std::logic_error(string_format("invalid type %d", (int)c.type));
        }
    }
    //TODO better you won
    //TODO some sort of menu?

    void drawCardBack(int y, int x) {
        drawCardBackImpl(y, x, PAIR_CARD_BACK, "░");
    }
    void drawCardEmpty(int y, int x) {
        drawCardBackImpl(y, x, PAIR_BG, " ");
    }

    void drawCardBackImpl(int y, int x, short col, const char* filling) {
        colfill(col, Rect(x, y, cardSize.x, cardSize.y));

        move(y, x);
        addstr("┌");
        for (int i = 1; i < cardSize.x-1; ++i)
            addstr("─");
        addstr("┐");

        for (int j = 1; j < cardSize.y-1;++j) {
            move(y+j, x);
            addstr("│");
            for (int i = 1; i < cardSize.x-1; ++i) {
                addstr(filling);//▚ ╬
            }
            addstr("│");
        }
        move(y+cardSize.y-1, x);
        addstr("└");
        for (int i = 1; i < cardSize.x-1; ++i)
            addstr("─");
        addstr("┘");

    }
    void drawCard(int y, int x, Card c) {
        auto col = c.isRed() ? PAIR_CARD_RED : PAIR_CARD_BLACK;
        colfill(col, Rect(x, y, cardSize.x, cardSize.y));

        if (smallMode) {
            std::string suit;
            switch (c.suit) {
            case Suit::Spades: suit = "♠"; break;
            case Suit::Hearts: suit = "♥"; break;
            case Suit::Clubs: suit = "♣"; break;
            case Suit::Diamonds: suit = "♦"; break;
            default:
                throw std::logic_error(string_format("invalid suit %d", (int)c.suit));
            }
            char typeStr[3] = {c.typeChar(), 0, 0 };
            bool is10 = c.type==10;
            if (is10) {
                typeStr[0] = '1';
                typeStr[1] = '0';
            }
            mvprintw(y, x+ is10 + 1, suit.c_str());
            mvprintw(y, x, typeStr);
            mvprintw(y+cardSize.y-1, x+cardSize.x - 1 - is10, typeStr);
            mvprintw(y+cardSize.y-1, x+cardSize.x - 2 - is10, suit.c_str());
            return;
        }
        printCard(y,x,c);
    }
    void drawCardOrEmpty(int y, int x, Card c) {
        if (c.type == 0) drawCardEmpty(y,x);
        else drawCard(y, x, c);
    }
    void drawPile(int y, int x, const Pile& p, int selectedIndex) {
        auto sz = p.cards.size();
        drawCardEmpty(y, x);
        if (sz == 0) {
            return;
        }
        size_t increment = 2 - smallMode;
        size_t i = 0;
        size_t lastY = y;
        for (; i < p.shownPoint; ++i)
            drawCardBack(lastY++, x);
        if (selectedIndex >= (int)p.shownPoint) {
            for (; (int) i < selectedIndex; ++i) {
                drawCard(lastY, x, p.cards[i]);
                 lastY += increment;
            }
            ++lastY;
        }
        for (; i < sz; ++i) {
            drawCard(lastY, x, p.cards[i]);
            lastY += increment;
        }
    }

    size_t cardPaddingX = cardSize.x + 3;
    size_t cardPaddingY = cardSize.y + 1;
    void drawCursor(Point c, short col, const char* str, const std::array<Pile, 7>& piles, int selectedIndex) {
        if (c.x == -1) return;
        setcol(col);
        if (c.y == -1) {
            mvaddstr(cardPaddingY / 2, leftPad+ 2 + cardSize.x + cardPaddingX * c.x, str);
        } else {
            int shownPoint = piles[c.x].shownPoint;
            if (c.y < shownPoint)
                mvaddstr(1+cardPaddingY + c.y, leftPad+ 2 + cardSize.x + cardPaddingX * c.x, str);
            else {
                //casting to size_t removes the need to compare selectedIndex with -1 since -1 > any y value
                mvaddstr(1 + cardPaddingY + shownPoint + (c.y - shownPoint) * (2-smallMode) + ((size_t) selectedIndex <= (size_t) c.y),
                         2 + cardSize.x + cardPaddingX * c.x + leftPad, str);
            }
        }
    }
    void draw(const Game& game, const Player& player) {
        colfill(PAIR_BG, Rect(0, 0, COLS, LINES));
        
        leftPad = (COLS - Width) / 2;
        if (leftPad < 0) leftPad = 0;
        /*
          std::array<Pile, 7> piles;
          //A card with type 0 (default) is considered empty
          std::array<Card, 4> topPiles;
          std::vector<Card> stock;
          // cards from this point on are face up
          size_t stockIndex = 0;
         */
        if ((size_t)game.curr.stockIndex == game.curr.stock.size()-1) {
            drawCardEmpty(1, 2+leftPad);
        } else {
            drawCardBack(1, 2+leftPad);
        }
        //TODO: show 3?
        drawCardOrEmpty(1, 2 + cardPaddingX +leftPad, game.stockTop());

        for (int i = 0; i < 4; ++i) {
            drawCardOrEmpty(1, 2 + cardPaddingX * (i+3) +leftPad, game.curr.topPiles[i]);
        }

        if (player.selected == player.curr) {
            drawCursor(player.selected, PAIR_CURSOR, "<<", game.curr.piles, player.selected.y);
        } else {
            drawCursor(player.selected, PAIR_SELECTED, "<=", game.curr.piles, player.selected.y);
            drawCursor(player.curr, PAIR_CURSOR, "<", game.curr.piles, player.selected.x == player.curr.x ? player.selected.y : -1);
        }

        for (int i = 0; i < 7; ++i) {
            drawPile(1+ cardPaddingY, 2 + cardPaddingX *i + leftPad, game.curr.piles[i], player.selected.x == i ? player.selected.y : -1);
        }

        refresh();
    }
};

int main(int argc, char**) {
    {
        Game game;
        Graphics g(argc > 1);
        Player player(game);
        draw = [&](){ g.draw(game, player); };
        refresh();
        while (!game.isWon()) {
            g.draw(game, player);
            player.input();
        }
    }
    std::cout << "YOU WON!! (Better message is WIP) \n";

    return 0;
}

