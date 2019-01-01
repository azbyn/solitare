#include <array>
#include <vector>
#include <random>
#include <algorithm>

//if last bit is 1 it's a red card
enum class Suit {
    Spades = 0,
    Hearts = 1,
    Clubs = 2,
    Diamonds = 3,

    Size
};

struct Card {
    // Ace = 1
    // Jack = 11
    // Queen = 12
    // King = 13
    int type;
    Suit suit;
    constexpr Card() : type(0), suit(Suit::Spades) {}
    constexpr Card(int type, Suit suit) : type(type), suit(suit) {}
    constexpr char typeChar() const {
        switch (type) {
        case 1:  return 'A';
        case 10: return 'T';
        case 11: return 'J';
        case 12: return 'Q';
        case 13: return 'K';
        default: return '0' + type;
        }
    }
    constexpr bool isRed() const {
        return int(suit) & 1;
    }
};

struct Deck {
    std::random_device rd;
    std::mt19937 gen;

    std::vector<Card> cards;

    Deck() : rd(), gen(rd()), cards(52) {
        for (int s = (int) Suit::Spades; s < (int) Suit::Size; ++s) {
            for (int i = 0; i < 13; ++i) {
                cards[s * 13 + i] = Card(i + 1, (Suit) s);
            }
        }

        std::shuffle(cards.begin(), cards.end(), gen);
    }

    Card deal() {
        if (cards.size() == 0) throw std::runtime_error("Ty Debil (deal())");
        Card res = cards.back();
        cards.pop_back();
        return res;
    }
    std::vector<Card> deal(size_t i) {
        if (cards.size() < i) throw std::runtime_error("Ty Debil (deal(size_t))");
        std::vector<Card> res(i);
        auto newSize = cards.size() - i;

        auto end = cards.end();
        auto start = cards.begin() + newSize;

        //std::cout << "newsz " << newSize << "\n";
        std::copy(start, end, res.begin());
        cards.resize(newSize);

        return res;
    }
};

struct Pile {
    std::vector<Card> cards;

    // cards from this point on are face up
    size_t shownPoint;
    Pile(){}
    Pile(std::vector<Card>&& cards, size_t shownPoint)
        : cards(std::move(cards)), shownPoint(shownPoint) {}

    void popBack() {
        cards.pop_back();
        auto sz = cards.size();
        if (sz == 0) return;
        if (shownPoint == sz) --shownPoint;
    }
    auto end() {
        return cards.end();
    }
    auto begin() {
        return cards.begin();
    }
    template<class It>
    void erase(It start, It end) {
        cards.erase(start, end);
        auto sz = cards.size();
        if (shownPoint >= sz) shownPoint = sz -1;
    }

    // returns if is valid move
    template<class It>
    bool append(It start, It end) {
        if (cards.size() == 0) {
            if (start->type != 13) return false;
            shownPoint = 0;
        }
        else {
            auto back = cards.back();
            if (start->type != back.type - 1 || back.isRed() == start->isRed())
                return false;
        }
        cards.insert(cards.end(), start, end);
        return true;

    }
    bool append(Card c) {
        if (cards.size() == 0) {
            if (c.type != 13) return false;
            shownPoint = 0;
            cards = {c};
            return true;
        }
        else {
            auto back = cards.back();
            if (c.type != back.type -1 || c.isRed() == back.isRed())
                return false;
        }
        cards.push_back(c);
        return true;
    }
};
struct Game {
    struct State {
        std::array<Pile, 7> piles;
        //A card with type 0 (default) is considered empty
        std::array<Card, 4> topPiles;
        std::vector<Card> stock;
        // cards from this point on are face up (including this one)
        int stockIndex = -1;
    } curr, prev;


    Card stockTop() const {
        if (curr.stockIndex == -1) return Card();
        return curr.stock[curr.stockIndex];
    }
    void popStockTop() {
        if (curr.stockIndex == -1) return;
        curr.stock.erase(curr.stock.begin() + curr.stockIndex);
        curr.stockIndex--;
    }
    void undo() {
        std::swap(curr, prev);
    }
    void saveUndo(){
        prev = curr;
    }
    bool isWon() const {
        //it's less work
        for (auto& t : curr.topPiles) {
            if (t.type != 13) return false;
        }
        return true;
        /*
        for (auto& p : curr.piles) {
            if (p.cards.size() != 0) return false;
        }
        return
        */
    }

    Game() {
        Deck deck;
        for (size_t i = 0; i < curr.piles.size(); ++i)
            curr.piles[i] = Pile(deck.deal(i+1), i);
        curr.stock = deck.cards;

        prev = curr;
        //piles[0] = Pile({}, 0);
        //piles[1] = Pile({{13, Suit::Clubs}}, 0);

        /*
        for (size_t i = 1; i < piles.size(); ++i)
            piles[i] = Pile(deck.deal(i+1), i-1);

        topPiles[0] = Card(1, Suit::Clubs);
        topPiles[2] = Card(1, Suit::Diamonds);
        */
    }
    void incrementStock() {
        if (curr.stock.size() == 0) return;
        prev = curr;
        if (++curr.stockIndex >= (int) curr.stock.size()) {
            curr.stockIndex = -1;
        }
    }

};
