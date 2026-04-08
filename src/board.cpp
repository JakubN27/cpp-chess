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

// Standalone helper function for testing
bool validate_move_helper(const Move& move, const Board& board) {
    if (move.start_row < 0 || move.start_row >= 8 ||
        move.start_col < 0 || move.start_col >= 8 ||
        move.end_row   < 0 || move.end_row   >= 8 ||
        move.end_col   < 0 || move.end_col   >= 8) {
        std::cout << "ERROR: Move out of bounds\n";
        return false;
    }

    Piece PIECE_MOVED = board[move.start_row][move.start_col];
    Piece TARGET = board[move.end_row][move.end_col];

    if (move.start_col == move.end_col &&
        move.start_row == move.end_row){
        return false;
    }

    int row_diff = move.end_row - move.start_row;
    int col_diff = move.end_col - move.start_col;
    int row_dir = (row_diff > 0) - (row_diff < 0);
    int col_dir = (col_diff > 0) - (col_diff < 0);

    switch (PIECE_MOVED){
        case EMPTY:
            std::cout << "ERROR: No piece is being moved" << std::endl;
            return false;

        case BLACKPAWN:
            if (move.start_col == move.end_col && TARGET == EMPTY){
                if (row_diff == 1){
                    return true;
                }
                if (move.start_row == 1 && row_diff == 2 &&
                    board[move.start_row + 1][move.start_col] == EMPTY){
                    return true;
                }
            }
            else if (abs(col_diff) == 1 && row_diff == 1 &&
                     TARGET != EMPTY &&
                     is_white(PIECE_MOVED) != is_white(TARGET)){
                return true;
            }
            return false;

        case WHITEPAWN:
            if (move.start_col == move.end_col && TARGET == EMPTY){
                if (row_diff == -1){
                    return true;
                }
                if (move.start_row == 6 && row_diff == -2 &&
                    board[move.start_row - 1][move.start_col] == EMPTY){
                    return true;
                }
            }
            else if (abs(col_diff) == 1 && row_diff == -1 &&
                     TARGET != EMPTY &&
                     is_white(PIECE_MOVED) != is_white(TARGET)){
                return true;
            }
            return false;

        case BLACKKNIGHT:
        case WHITEKNIGHT:
            if ((abs(row_diff) == 1 && abs(col_diff) == 2) ||
                (abs(row_diff) == 2 && abs(col_diff) == 1)) {
                if (TARGET == EMPTY){
                    return true;
                }
                else if (is_white(PIECE_MOVED) != is_white(TARGET)){
                    return true;
                }
            }
            return false;

        case BLACKBISHOP:
        case WHITEBISHOP:
            if (abs(row_diff) != abs(col_diff)) {
                return false;
            }
            for (int i = 1; i < abs(row_diff); ++i) {
                int new_row = move.start_row + i * row_dir;
                int new_col = move.start_col + i * col_dir;

                if (board[new_row][new_col] != EMPTY) {
                    return false;
                }
            }
            if (TARGET == EMPTY){
                return true;
            }
            else if (is_white(PIECE_MOVED) != is_white(TARGET)){
                return true;
            }
            return false;

        case BLACKROOK:
        case WHITEROOK:
            if (!(row_diff == 0 || col_diff == 0)) {
                return false;
            }
            if (row_diff == 0) {
                for (int i = 1; i < abs(col_diff); ++i) {
                    int new_row = move.start_row;
                    int new_col = move.start_col + i * col_dir;

                    if (board[new_row][new_col] != EMPTY) {
                        return false;
                    }
                }
            } else {
                for (int i = 1; i < abs(row_diff); ++i) {
                    int new_row = move.start_row + i * row_dir;
                    int new_col = move.start_col;

                    if (board[new_row][new_col] != EMPTY) {
                        return false;
                    }
                }
            }
            if (TARGET == EMPTY){
                return true;
            }
            else if (is_white(PIECE_MOVED) != is_white(TARGET)){
                return true;
            }
            return false;

        case BLACKQUEEN:
        case WHITEQUEEN:
            if (!(abs(row_diff) == abs(col_diff) ||
                row_diff == 0 ||
                col_diff == 0)) {
                return false;
            }

            int distance;
            if (row_diff == 0) {
                distance = abs(col_diff);
            } else {
                distance = abs(row_diff);
            }

            for (int i = 1; i < distance; ++i) {
                int new_row = move.start_row + i * row_dir;
                int new_col = move.start_col + i * col_dir;

                if (board[new_row][new_col] != EMPTY) {
                    return false;
                }
            }

            if (TARGET == EMPTY){
                return true;
            }
            else if (is_white(PIECE_MOVED) != is_white(TARGET)){
                return true;
            }
            return false;

        case BLACKKING:
        case WHITEKING:
            if (abs(col_diff) <= 1 && abs(row_diff) <= 1){
                if (TARGET == EMPTY){
                    return true;
                }
                else if (is_white(PIECE_MOVED) != is_white(TARGET)){
                    return true;
                }
            }
            return false;

        default:
            std::cout << "Invalid move" << std::endl;
            return false;
    }
}
