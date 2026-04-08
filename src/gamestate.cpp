#include "../include/gamestate.h"
#include "../include/board.h"
#include "../include/piece.h"
#include <iostream>
#include <cmath>
#include <stdexcept>

static Position find_king(const Board& b, Piece king) {
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            if (b[r][c] == king) return Position{r, c};
        }
    }
    throw std::runtime_error("King not found on board");
}

// GameState Constructor
GameState::GameState() {
    board = create_board();
    white_to_move = true;
    white_king_pos = Position{7, 4};  // e1
    black_king_pos = Position{0, 4};  // e8
}

// Custom boards, mostly for testing 
GameState::GameState(const Board& custom_board) {
    board = custom_board;
    white_to_move = true;

    // One-time scan to initialize cached king positions.
    white_king_pos = find_king(board, WHITEKING);
    black_king_pos = find_king(board, BLACKKING);
}

GameState::GameState(const Board& custom_board, Position whiteKingPos, Position blackKingPos, bool whiteToMove) {
    board = custom_board;
    white_to_move = whiteToMove;
    white_king_pos = whiteKingPos;
    black_king_pos = blackKingPos;
}

void GameState::update_cached_king_pos_after_move(const Move& move) {
    Piece moved = board[move.end_row][move.end_col];
    if (moved == WHITEKING) {
        white_king_pos = Position{move.end_row, move.end_col};
    } else if (moved == BLACKKING) {
        black_king_pos = Position{move.end_row, move.end_col};
    }
}

void GameState::print_board() {
    for (size_t i = 0; i < board.size(); i++) {
        for (size_t j = 0; j < board[i].size(); j++) {
            std::cout << piece_to_char(board[i][j]) << " ";
        }
        std::cout << '\n';
    }
}

bool GameState::make_move(const Move& move) {
    // NOTE: Keep this function fast; avoid any I/O here (engine will call it a lot).

    board[move.end_row][move.end_col] = board[move.start_row][move.start_col];
    board[move.start_row][move.start_col] = EMPTY;

    update_cached_king_pos_after_move(move);
    white_to_move = !white_to_move;

    return true;
}

bool GameState::validate_move(const Move& move) {
    if (move.start_row < 0 || move.start_row >= 8 ||
        move.start_col < 0 || move.start_col >= 8 ||
        move.end_row   < 0 || move.end_row   >= 8 ||
        move.end_col   < 0 || move.end_col   >= 8) {
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

bool GameState::is_square_attacked(int row, int col, bool by_white) {
    if (by_white){
        if (in_bounds(row + 1, col + 1) && board[row + 1][col + 1] == WHITEPAWN ||
            in_bounds(row + 1, col - 1) && board[row + 1][col - 1] == WHITEPAWN){
            return true;
        }
    } else{
        if (in_bounds(row - 1, col + 1) && board[row - 1][col + 1] == BLACKPAWN ||
            in_bounds(row - 1, col - 1) && board[row - 1][col - 1] == BLACKPAWN){
            return true;
        }
    }

    const std::array<std::pair<int,int>, 8> knight_squares = {{
        {1, 2}, {1, -2}, {-1, 2}, {-1, -2}, {2, 1}, {2, -1}, {-2, 1}, {-2, -1}
    }};
    for (const auto& [row_diff, col_diff]: knight_squares){
        int new_row = row + row_diff;
        int new_col = col + col_diff;

        if (in_bounds(new_row, new_col)){
            if (by_white && board[new_row][new_col] == WHITEKNIGHT){
                return true;
            } else if (not by_white && board[new_row][new_col] == BLACKKNIGHT){
                return true;
            }
        }
    }

    const std::array<std::pair<int,int>, 4> bishop_dirs = {{
        {1,1}, {1, -1}, {-1, -1}, {-1, 1}
    }};
    for (const auto& [row_dir, col_dir]: bishop_dirs){
        int new_row = row + row_dir;
        int new_col = col + col_dir;
        while (in_bounds(new_row, new_col)){
            if (board[new_row][new_col] == EMPTY){
                new_row += row_dir;
                new_col += col_dir;
                continue;
            }
            else if (by_white &&
                (board[new_row][new_col] == WHITEBISHOP ||
                board[new_row][new_col] == WHITEQUEEN)) {
                return true;
            }
            else if (not by_white &&
                (board[new_row][new_col] == BLACKBISHOP ||
                board[new_row][new_col] == BLACKQUEEN)) {
                return true;
            }
            break;
        }
    }

    const std::array<std::pair<int,int>, 4> rook_dirs = {{
        {0,1}, {0,-1}, {1,0}, {-1, 0}
    }};
    for (const auto& [row_dir, col_dir]: rook_dirs){
        int new_row = row + row_dir;
        int new_col = col + col_dir;
        while (in_bounds(new_row, new_col)){
            if (board[new_row][new_col] == EMPTY){
                new_row += row_dir;
                new_col += col_dir;
                continue;
            }
            else if (by_white &&
                (board[new_row][new_col] == WHITEROOK ||
                board[new_row][new_col] == WHITEQUEEN)) {
                return true;
            }
            else if (not by_white &&
                (board[new_row][new_col] == BLACKROOK ||
                board[new_row][new_col] == BLACKQUEEN)) {
                return true;
            }
            break;
        }
    }

    const std::array<std::pair<int,int>, 8> king_dirs = {{
        {0,1}, {0,-1}, {1,1}, {1, -1}, {1,0}, {-1,0}, {-1, 1}, {-1,-1}
    }};
    for (const auto& [row_dir, col_dir] : king_dirs){
        int new_row = row + row_dir;
        int new_col = col + col_dir;
        if (!in_bounds(new_row, new_col)){
            continue;
        }
        else if(by_white && board[new_row][new_col] == WHITEKING){
            return true;
        }
        else if (not by_white && board[new_row][new_col] == BLACKKING){
            return true;
        }
    }
    return false;
}

bool GameState::is_in_check(int row, int col, bool white) {
    return is_square_attacked(row, col, !white);
}

void GameState::capture_piece(const Move& move) {
    // Implementation can be added later
}
