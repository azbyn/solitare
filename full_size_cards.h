#include <array>

namespace full_size_cards {
using T = std::array<const char*, 7>;
template<const T& s, size_t i>
constexpr size_t newStrSize() {
    size_t cnt = 1;
    const char* p = s[i];
    while (*p) {
        if (*p++ == 'x') cnt += 3;
        else ++cnt;
    }
    return cnt;
}
template<const T& s, size_t index, char lastByte>
constexpr auto replaceX() {
    std::array<char, newStrSize<s, index>()> res = {};
    size_t i = 0;
    const char* p = s[index];

    while (*p) {
        if (*p == 'x') {
            res[i+0] = 0xE2;
            res[i+1] = 0x99;
            res[i+2] = lastByte;
            i += 3;
        }
        else res[i++] = *p;
        ++p;
    }
    res.back() = 0;
    return res;
}


constexpr T arr2 = {
    //23456789AB
    "2          ",//1
    "x    x     ",//2
    "           ",//3
    "           ",//4
    "           ",//5
    "     x    x",//6
    "          2",//7
};
constexpr T arr3 = {
    //23456789AB
    "3          ",//1
    "x    x     ",//2
    "           ",//3
    "     x     ",//4
    "           ",//5
    "     x    x",//6
    "          3",//7
};
constexpr T arr4 = {
    //23456789AB
    "4          ",//1
    "x  x   x   ",//2
    "           ",//3
    "           ",//4
    "           ",//5
    "   x   x  x",//6
    "          4",//7
};
constexpr T arr5 = {
    //23456789AB
    "5          ",//1
    "x  x   x   ",//2
    "           ",//3
    "     x     ",//4
    "           ",//5
    "   x   x  x",//6
    "          5",//7
};
constexpr T arr6 = {
    //23456789AB
    "6          ",//1
    "x  x   x   ",//2
    "           ",//3
    "   x   x   ",//4
    "           ",//5
    "   x   x  x",//6
    "          6",//7
};
constexpr T arr7 = {
    //23456789AB
    "7          ",//1
    "x  x   x   ",//2
    "           ",//3
    "  x  x  x  ",//4
    "           ",//5
    "   x   x  x",//6
    "          7",//7
};
constexpr T arr8 = {
    //23456789AB
    "8          ",//1
    "x  x   x   ",//2
    "     x     ",//3
    "   x   x   ",//4
    "     x     ",//5
    "   x   x  x",//6
    "          8",//7
};
static constexpr T arr9 = {
    //23456789AB
    "9          ",//1
    "x x  x  x  ",//2
    "           ",//3
    "  x  x  x  ",//4
    "           ",//5
    "  x  x  x x",//6
    "          9",//7
};
constexpr T arr10 = {
    //23456789AB
    "10         ",//1
    "x x     x  ",//2
    "    x x    ",//3
    "   x   x   ",//4
    "    x x    ",//5
    "  x     x x",//6
    "         10",//7
};

constexpr T arrJ = {
    //23456789AB
    "J......... ",//1
    "x.       . ",//2
    " .  /-\\  . ",//3
    " . (•_•) . ",//4
    " .  d'b  . ",//5
    " .       .x",//6
    " .........J",//7
};
constexpr T arrQ = {
    //23456789AB
    "Q......... ",//1
    "x.       . ",//2
    " .  mMm  . ",//3
    " . {o‿o} . ",//4
    " .  u-u  . ",//5
    " .       .x",//6
    " .........Q",//7
};

constexpr T arrK = {
    //23456789AB
    "K......... ",//1
    "x.       . ",//2
    " .  MWM  . ",//3
    " . (o.o) . ",//4
    " .  vTv  . ",//5
    " .       .x",//6
    " .........K",//7
};
constexpr T arrAH = {
    //23456789AB
    "A......... ",//1
    "x.  _ _  . ",//3
    " . ( V ) . ",//4
    " .  \\ /  . ",//5
    " .   v   . ",//6
    " .       .x",//2
    " .........A",//7
};
constexpr T arrAC = {
    //23456789AB
    "A......... ",//1
    "x.   _   . ",//3
    " .  ( )  . ",//4
    " . (‿┬‿) . ",//5
    " .   ╵   . ",//6
    " .       .x",//2
    " .........A",//7
};
static constexpr T arrAD = {
    // 123456789AB
    R"(A......... )",//1
    R"(x.   ʌ   . )",//2
    R"( .  / \  . )",//3
    R"( . ⟨   ⟩ . )",//4
    R"( .  \ /  . )",//5
    R"( .   v   .x)",//6
    R"( .........A)",//7
};
constexpr T arrAS = {
    //23456789AB
    "A......... ",//1
    "x.       . ",//2
    " .   ʌ   . ",//3
    " .  / \\  . ",//4
    " . (‿┬‿) . ",//5
    " .   ╵   .x",//6
" .........A",//7
};
}
