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

enum MoveFlag : uint8_t {
    MOVE_NONE = 0,
    MOVE_CAPTURE = 1 << 0,
    MOVE_DOUBLE_PAWN_PUSH = 1 << 1,
    MOVE_EN_PASSANT = 1 << 2,
    MOVE_CASTLE = 1 << 3,
    MOVE_PROMOTION = 1 << 4
};

struct Move {
    int start_row;
    int end_row;
    int start_col;
    int end_col;

    uint8_t flags = MOVE_NONE;
    Piece promotion = EMPTY; // only used when MOVE_PROMOTION is set
};

struct Position {
    int row;
    int col;
};

struct Undo {
    Piece moved_piece;
    Piece captured_piece;

    bool prev_white_to_move;

    // state needed for full rules
    uint8_t prev_castling_rights;
    Position prev_en_passant; // {-1,-1} means none
    int prev_halfmove_clock;
};

#endif
