#ifndef TYPES_H
#define TYPES_H

#include <array>
#include <cstdint>

enum Piece : uint8_t {
    EMPTY,
    WHITEPAWN,
    BLACKPAWN,
    WHITEKNIGHT,
    BLACKKNIGHT,
    WHITEBISHOP,
    BLACKBISHOP,
    WHITEROOK,
    BLACKROOK,
    WHITEQUEEN,
    BLACKQUEEN,
    WHITEKING,
    BLACKKING
};

using Board = std::array<std::array<Piece, 8>, 8>;

struct Move {
    int start_row;
    int end_row;
    int start_col;
    int end_col;
};

struct Position {
    int row;
    int col;
};

#endif
