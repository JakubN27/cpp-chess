#include "../include/board.h"
#include "../include/piece.h"
#include <iostream>
#include <cmath>

Board create_board() {
    Board board{};

    board[0] = {BLACKROOK, BLACKKNIGHT, BLACKBISHOP, BLACKQUEEN,
                BLACKKING, BLACKBISHOP, BLACKKNIGHT, BLACKROOK};

    board[1] = {BLACKPAWN, BLACKPAWN, BLACKPAWN, BLACKPAWN,
                BLACKPAWN, BLACKPAWN, BLACKPAWN, BLACKPAWN};

    board[2] = {EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY};
    board[3] = {EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY};
    board[4] = {EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY};
    board[5] = {EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY};

    board[6] = {WHITEPAWN, WHITEPAWN, WHITEPAWN, WHITEPAWN,
                WHITEPAWN, WHITEPAWN, WHITEPAWN, WHITEPAWN};

    board[7] = {WHITEROOK, WHITEKNIGHT, WHITEBISHOP, WHITEQUEEN,
                WHITEKING, WHITEBISHOP, WHITEKNIGHT, WHITEROOK};

    return board;
}
