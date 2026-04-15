#include "../include/gamestate.h"
#include "../include/board.h"
#include "../include/piece.h"
#include <iostream>
#include <cmath>
#include <stdexcept>
#include <array>
#include <cstdint>

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

    castling_rights = 1 | 2 | 4 | 8;
    en_passant = Position{-1, -1};
    halfmove_clock = 0;
    fullmove_number = 1;

    hash = compute_hash();
    position_hashes.clear();
    position_hashes.push_back(hash);
}

// Custom boards, mostly for testing 
GameState::GameState(const Board& custom_board) {
    board = custom_board;
    white_to_move = true;

    white_king_pos = find_king(board, WHITEKING);
    black_king_pos = find_king(board, BLACKKING);

    castling_rights = 0;
    en_passant = Position{-1, -1};
    halfmove_clock = 0;
    fullmove_number = 1;

    hash = compute_hash();
    position_hashes.clear();
    position_hashes.push_back(hash);
}

GameState::GameState(const Board& custom_board, Position whiteKingPos, Position blackKingPos, bool whiteToMove) {
    board = custom_board;
    white_to_move = whiteToMove;
    white_king_pos = whiteKingPos;
    black_king_pos = blackKingPos;

    castling_rights = 0;
    en_passant = Position{-1, -1};
    halfmove_clock = 0;
    fullmove_number = 1;

    hash = compute_hash();
    position_hashes.clear();
    position_hashes.push_back(hash);
}

void GameState::update_king(const Move& move) {
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

Undo GameState::make_move(const Move& move) {
    Undo undo;
    undo.moved_piece = board[move.start_row][move.start_col];
    undo.captured_piece = board[move.end_row][move.end_col];
    undo.prev_white_to_move = white_to_move;
    undo.prev_castling_rights = castling_rights;
    undo.prev_en_passant = en_passant;
    undo.prev_halfmove_clock = halfmove_clock;
    undo.prev_hash = hash;

    // reset en-passant by default
    en_passant = Position{-1, -1};

    // halfmove clock
    if (undo.moved_piece == WHITEPAWN || undo.moved_piece == BLACKPAWN || undo.captured_piece != EMPTY || (move.flags & MOVE_EN_PASSANT)){
        halfmove_clock = 0;
    } else {
        halfmove_clock += 1;
    }

    // handle en-passant capture (captured pawn is not on end square)
    if (move.flags & MOVE_EN_PASSANT){
        if (undo.moved_piece == WHITEPAWN){
            undo.captured_piece = board[move.end_row + 1][move.end_col];
            board[move.end_row + 1][move.end_col] = EMPTY;
        } else {
            undo.captured_piece = board[move.end_row - 1][move.end_col];
            board[move.end_row - 1][move.end_col] = EMPTY;
        }
    }

    // move piece (and optionally promote)
    board[move.end_row][move.end_col] = undo.moved_piece;
    board[move.start_row][move.start_col] = EMPTY;

    if (move.flags & MOVE_PROMOTION){
        board[move.end_row][move.end_col] = move.promotion;
    }

    // castling rook move
    if (move.flags & MOVE_CASTLE){
        // white
        if (undo.moved_piece == WHITEKING){
            // king goes from e1 (7,4)
            if (move.end_col == 6){
                // king side: rook h1 -> f1
                board[7][5] = WHITEROOK;
                board[7][7] = EMPTY;
            } else if (move.end_col == 2){
                // queen side: rook a1 -> d1
                board[7][3] = WHITEROOK;
                board[7][0] = EMPTY;
            }
        }
        // black
        else if (undo.moved_piece == BLACKKING){
            if (move.end_col == 6){
                board[0][5] = BLACKROOK;
                board[0][7] = EMPTY;
            } else if (move.end_col == 2){
                board[0][3] = BLACKROOK;
                board[0][0] = EMPTY;
            }
        }
    }

    // update king caches
    update_king(move);

    // update castling rights on king/rook move or rook capture
    if (undo.moved_piece == WHITEKING){
        castling_rights &= ~(1 | 2);
    }
    else if (undo.moved_piece == BLACKKING){
        castling_rights &= ~(4 | 8);
    }
    else if (undo.moved_piece == WHITEROOK){
        if (move.start_row == 7 && move.start_col == 0) castling_rights &= ~2;
        else if (move.start_row == 7 && move.start_col == 7) castling_rights &= ~1;
    }
    else if (undo.moved_piece == BLACKROOK){
        if (move.start_row == 0 && move.start_col == 0) castling_rights &= ~8;
        else if (move.start_row == 0 && move.start_col == 7) castling_rights &= ~4;
    }

    // rook captured affects castling rights
    if (undo.captured_piece == WHITEROOK){
        if (move.end_row == 7 && move.end_col == 0) castling_rights &= ~2;
        else if (move.end_row == 7 && move.end_col == 7) castling_rights &= ~1;
    }
    else if (undo.captured_piece == BLACKROOK){
        if (move.end_row == 0 && move.end_col == 0) castling_rights &= ~8;
        else if (move.end_row == 0 && move.end_col == 7) castling_rights &= ~4;
    }

    // set en-passant square after double pawn push
    if (move.flags & MOVE_DOUBLE_PAWN_PUSH){
        if (undo.moved_piece == WHITEPAWN){
            en_passant = Position{move.start_row - 1, move.start_col};
        } else {
            en_passant = Position{move.start_row + 1, move.start_col};
        }
    }

    // flip side
    white_to_move = !white_to_move;

    if (!white_to_move){
        fullmove_number += 1;
    }

    history.push_back(undo);

    hash = compute_hash();
    position_hashes.push_back(hash);

    return undo;
}

void GameState::undo_move(const Move& move, const Undo& undo) {
    // restore side/state first
    white_to_move = undo.prev_white_to_move;
    castling_rights = undo.prev_castling_rights;
    en_passant = undo.prev_en_passant;
    halfmove_clock = undo.prev_halfmove_clock;

    // revert fullmove counter
    if (!white_to_move){
        // we are undoing a black move
        fullmove_number -= 1;
    }

    // undo castling rook move
    if (move.flags & MOVE_CASTLE){
        if (undo.moved_piece == WHITEKING){
            if (move.end_col == 6){
                board[7][7] = WHITEROOK;
                board[7][5] = EMPTY;
            } else if (move.end_col == 2){
                board[7][0] = WHITEROOK;
                board[7][3] = EMPTY;
            }
        } else if (undo.moved_piece == BLACKKING){
            if (move.end_col == 6){
                board[0][7] = BLACKROOK;
                board[0][5] = EMPTY;
            } else if (move.end_col == 2){
                board[0][0] = BLACKROOK;
                board[0][3] = EMPTY;
            }
        }
    }

    // undo moved piece (promotion restores pawn)
    board[move.start_row][move.start_col] = undo.moved_piece;

    // restore capture
    board[move.end_row][move.end_col] = undo.captured_piece;

    // undo en-passant capture (captured pawn returns behind end square, and end square becomes empty)
    if (move.flags & MOVE_EN_PASSANT){
        board[move.end_row][move.end_col] = EMPTY;
        if (undo.moved_piece == WHITEPAWN){
            board[move.end_row + 1][move.end_col] = undo.captured_piece;
        } else {
            board[move.end_row - 1][move.end_col] = undo.captured_piece;
        }
    }

    // restore king caches if king moved
    if (undo.moved_piece == WHITEKING){
        white_king_pos = Position{move.start_row, move.start_col};
    }
    else if (undo.moved_piece == BLACKKING){
        black_king_pos = Position{move.start_row, move.start_col};
    }

    // restore hash and repetition stack
    hash = undo.prev_hash;
    if (!position_hashes.empty()) {
        position_hashes.pop_back();
    }

    if (!history.empty()){
        history.pop_back();
    }
}

bool GameState::validate_move(const Move& move) {
    // Validation for player entered moves
    //Move is defined as {start_row, end_row, start_col, end_col} in `types.h`.

    const int start_row = move.start_row;
    const int end_row   = move.end_row;
    const int start_col = move.start_col;
    const int end_col   = move.end_col;

    if (start_row < 0 || start_row >= 8 ||
        start_col < 0 || start_col >= 8 ||
        end_row   < 0 || end_row   >= 8 ||
        end_col   < 0 || end_col   >= 8) {
        return false;
    }

    Piece PIECE_MOVED = board[start_row][start_col];
    Piece TARGET = board[end_row][end_col];

    if (start_col == end_col &&
        start_row == end_row){
        return false;
    }

    int row_diff = end_row - start_row;
    int col_diff = end_col - start_col;
    int row_dir = (row_diff > 0) - (row_diff < 0);
    int col_dir = (col_diff > 0) - (col_diff < 0);

    switch (PIECE_MOVED){
        case EMPTY:
            std::cout << "ERROR: No piece is being moved" << std::endl;
            return false;

        case BLACKPAWN:
            if (start_col == end_col && TARGET == EMPTY){
                if (row_diff == 1){
                    return true;
                }
                if (start_row == 1 && row_diff == 2 &&
                    board[start_row + 1][start_col] == EMPTY){
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
            if (start_col == end_col && TARGET == EMPTY){
                if (row_diff == -1){
                    return true;
                }
                if (start_row == 6 && row_diff == -2 &&
                    board[start_row - 1][start_col] == EMPTY){
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
                int new_row = start_row + i * row_dir;
                int new_col = start_col + i * col_dir;

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
                    int new_row = start_row;
                    int new_col = start_col + i * col_dir;

                    if (board[new_row][new_col] != EMPTY) {
                        return false;
                    }
                }
            } else {
                for (int i = 1; i < abs(row_diff); ++i) {
                    int new_row = start_row + i * row_dir;
                    int new_col = start_col;

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
                int new_row = start_row + i * row_dir;
                int new_col = start_col + i * col_dir;

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

bool GameState::is_square_attacked(int row, int col, bool by_white) const {
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

bool GameState::is_in_check(int row, int col, bool white) const {
    return is_square_attacked(row, col, !white);
}

void GameState::capture_piece(const Move& move) {
    // Implementation can be added later
}

void GameState::generate_pseudolegal_moves(std::vector<Move>& moves) const {
    moves.clear();

    for (int start_row = 0; start_row < 8; ++start_row){
        for (int start_col = 0; start_col < 8; ++start_col){
            Piece PIECE_MOVED = board[start_row][start_col];

            if (PIECE_MOVED == EMPTY){
                continue;
            }

            if (white_to_move != is_white(PIECE_MOVED)){
                continue;
            }

            switch (PIECE_MOVED){
                case WHITEPAWN: {
                    // forward
                    if (in_bounds(start_row - 1, start_col) && board[start_row - 1][start_col] == EMPTY){
                        if (start_row - 1 == 0){
                            moves.push_back({start_row, start_row - 1, start_col, start_col, MOVE_PROMOTION, WHITEQUEEN});
                            moves.push_back({start_row, start_row - 1, start_col, start_col, MOVE_PROMOTION, WHITEROOK});
                            moves.push_back({start_row, start_row - 1, start_col, start_col, MOVE_PROMOTION, WHITEBISHOP});
                            moves.push_back({start_row, start_row - 1, start_col, start_col, MOVE_PROMOTION, WHITEKNIGHT});
                        } else {
                            moves.push_back({start_row, start_row - 1, start_col, start_col});
                        }

                        // double push
                        if (start_row == 6 && board[start_row - 2][start_col] == EMPTY){
                            Move m{start_row, start_row - 2, start_col, start_col};
                            m.flags = MOVE_DOUBLE_PAWN_PUSH;
                            moves.push_back(m);
                        }
                    }

                    // captures
                    if (in_bounds(start_row - 1, start_col - 1) &&
                        board[start_row - 1][start_col - 1] != EMPTY &&
                        is_white(board[start_row - 1][start_col - 1]) == false){
                        if (start_row - 1 == 0){
                            moves.push_back({start_row, start_row - 1, start_col, start_col - 1, (uint8_t)(MOVE_PROMOTION | MOVE_CAPTURE), WHITEQUEEN});
                            moves.push_back({start_row, start_row - 1, start_col, start_col - 1, (uint8_t)(MOVE_PROMOTION | MOVE_CAPTURE), WHITEROOK});
                            moves.push_back({start_row, start_row - 1, start_col, start_col - 1, (uint8_t)(MOVE_PROMOTION | MOVE_CAPTURE), WHITEBISHOP});
                            moves.push_back({start_row, start_row - 1, start_col, start_col - 1, (uint8_t)(MOVE_PROMOTION | MOVE_CAPTURE), WHITEKNIGHT});
                        } else {
                            Move m{start_row, start_row - 1, start_col, start_col - 1};
                            m.flags = MOVE_CAPTURE;
                            moves.push_back(m);
                        }
                    }

                    if (in_bounds(start_row - 1, start_col + 1) &&
                        board[start_row - 1][start_col + 1] != EMPTY &&
                        is_white(board[start_row - 1][start_col + 1]) == false){
                        if (start_row - 1 == 0){
                            moves.push_back({start_row, start_row - 1, start_col, start_col + 1, (uint8_t)(MOVE_PROMOTION | MOVE_CAPTURE), WHITEQUEEN});
                            moves.push_back({start_row, start_row - 1, start_col, start_col + 1, (uint8_t)(MOVE_PROMOTION | MOVE_CAPTURE), WHITEROOK});
                            moves.push_back({start_row, start_row - 1, start_col, start_col + 1, (uint8_t)(MOVE_PROMOTION | MOVE_CAPTURE), WHITEBISHOP});
                            moves.push_back({start_row, start_row - 1, start_col, start_col + 1, (uint8_t)(MOVE_PROMOTION | MOVE_CAPTURE), WHITEKNIGHT});
                        } else {
                            Move m{start_row, start_row - 1, start_col, start_col + 1};
                            m.flags = MOVE_CAPTURE;
                            moves.push_back(m);
                        }
                    }

                    // en-passant
                    if (en_passant.row != -1){
                        if (start_row == 3 && abs(en_passant.col - start_col) == 1 && en_passant.row == 2){
                            Move m{start_row, 2, start_col, en_passant.col};
                            m.flags = (uint8_t)(MOVE_EN_PASSANT | MOVE_CAPTURE);
                            moves.push_back(m);
                        }
                    }

                    break;
                }

                case BLACKPAWN: {
                    if (in_bounds(start_row + 1, start_col) && board[start_row + 1][start_col] == EMPTY){
                        if (start_row + 1 == 7){
                            moves.push_back({start_row, start_row + 1, start_col, start_col, MOVE_PROMOTION, BLACKQUEEN});
                            moves.push_back({start_row, start_row + 1, start_col, start_col, MOVE_PROMOTION, BLACKROOK});
                            moves.push_back({start_row, start_row + 1, start_col, start_col, MOVE_PROMOTION, BLACKBISHOP});
                            moves.push_back({start_row, start_row + 1, start_col, start_col, MOVE_PROMOTION, BLACKKNIGHT});
                        } else {
                            moves.push_back({start_row, start_row + 1, start_col, start_col});
                        }

                        if (start_row == 1 && board[start_row + 2][start_col] == EMPTY){
                            Move m{start_row, start_row + 2, start_col, start_col};
                            m.flags = MOVE_DOUBLE_PAWN_PUSH;
                            moves.push_back(m);
                        }
                    }

                    if (in_bounds(start_row + 1, start_col - 1) &&
                        board[start_row + 1][start_col - 1] != EMPTY &&
                        is_white(board[start_row + 1][start_col - 1]) == true){
                        if (start_row + 1 == 7){
                            moves.push_back({start_row, start_row + 1, start_col, start_col - 1, (uint8_t)(MOVE_PROMOTION | MOVE_CAPTURE), BLACKQUEEN});
                            moves.push_back({start_row, start_row + 1, start_col, start_col - 1, (uint8_t)(MOVE_PROMOTION | MOVE_CAPTURE), BLACKROOK});
                            moves.push_back({start_row, start_row + 1, start_col, start_col - 1, (uint8_t)(MOVE_PROMOTION | MOVE_CAPTURE), BLACKBISHOP});
                            moves.push_back({start_row, start_row + 1, start_col, start_col - 1, (uint8_t)(MOVE_PROMOTION | MOVE_CAPTURE), BLACKKNIGHT});
                        } else {
                            Move m{start_row, start_row + 1, start_col, start_col - 1};
                            m.flags = MOVE_CAPTURE;
                            moves.push_back(m);
                        }
                    }

                    if (in_bounds(start_row + 1, start_col + 1) &&
                        board[start_row + 1][start_col + 1] != EMPTY &&
                        is_white(board[start_row + 1][start_col + 1]) == true){
                        if (start_row + 1 == 7){
                            moves.push_back({start_row, start_row + 1, start_col, start_col + 1, (uint8_t)(MOVE_PROMOTION | MOVE_CAPTURE), BLACKQUEEN});
                            moves.push_back({start_row, start_row + 1, start_col, start_col + 1, (uint8_t)(MOVE_PROMOTION | MOVE_CAPTURE), BLACKROOK});
                            moves.push_back({start_row, start_row + 1, start_col, start_col + 1, (uint8_t)(MOVE_PROMOTION | MOVE_CAPTURE), BLACKBISHOP});
                            moves.push_back({start_row, start_row + 1, start_col, start_col + 1, (uint8_t)(MOVE_PROMOTION | MOVE_CAPTURE), BLACKKNIGHT});
                        } else {
                            Move m{start_row, start_row + 1, start_col, start_col + 1};
                            m.flags = MOVE_CAPTURE;
                            moves.push_back(m);
                        }
                    }

                    // en-passant
                    if (en_passant.row != -1){
                        if (start_row == 4 && abs(en_passant.col - start_col) == 1 && en_passant.row == 5){
                            Move m{start_row, 5, start_col, en_passant.col};
                            m.flags = (uint8_t)(MOVE_EN_PASSANT | MOVE_CAPTURE);
                            moves.push_back(m);
                        }
                    }

                    break;
                }

                case WHITEKNIGHT:
                case BLACKKNIGHT: {
                    const std::array<std::pair<int,int>, 8> knight_squares = {{
                        {1, 2}, {1, -2}, {-1, 2}, {-1, -2}, {2, 1}, {2, -1}, {-2, 1}, {-2, -1}
                    }};
                    for (const auto& [row_diff, col_diff]: knight_squares){
                        int end_row = start_row + row_diff;
                        int end_col = start_col + col_diff;

                        if (!in_bounds(end_row, end_col)){
                            continue;
                        }

                        Piece TARGET = board[end_row][end_col];
                        if (TARGET == EMPTY || is_white(TARGET) != is_white(PIECE_MOVED)){
                            moves.push_back({start_row, end_row, start_col, end_col});
                        }
                    }
                    break;
                }

                case WHITEBISHOP:
                case BLACKBISHOP: {
                    const std::array<std::pair<int,int>, 4> bishop_dirs = {{
                        {1,1}, {1,-1}, {-1,-1}, {-1,1}
                    }};
                    for (const auto& [row_dir, col_dir]: bishop_dirs){
                        int end_row = start_row + row_dir;
                        int end_col = start_col + col_dir;

                        while (in_bounds(end_row, end_col)){
                            Piece TARGET = board[end_row][end_col];

                            if (TARGET == EMPTY){
                                moves.push_back({start_row, end_row, start_col, end_col});
                            }
                            else if (is_white(TARGET) != is_white(PIECE_MOVED)){
                                moves.push_back({start_row, end_row, start_col, end_col});
                                break;
                            }
                            else{
                                break;
                            }

                            end_row += row_dir;
                            end_col += col_dir;
                        }
                    }
                    break;
                }

                case WHITEROOK:
                case BLACKROOK: {
                    const std::array<std::pair<int,int>, 4> rook_dirs = {{
                        {0,1}, {0,-1}, {1,0}, {-1,0}
                    }};
                    for (const auto& [row_dir, col_dir]: rook_dirs){
                        int end_row = start_row + row_dir;
                        int end_col = start_col + col_dir;

                        while (in_bounds(end_row, end_col)){
                            Piece TARGET = board[end_row][end_col];

                            if (TARGET == EMPTY){
                                moves.push_back({start_row, end_row, start_col, end_col});
                            }
                            else if (is_white(TARGET) != is_white(PIECE_MOVED)){
                                moves.push_back({start_row, end_row, start_col, end_col});
                                break;
                            }
                            else{
                                break;
                            }

                            end_row += row_dir;
                            end_col += col_dir;
                        }
                    }
                    break;
                }

                case WHITEQUEEN:
                case BLACKQUEEN: {
                    const std::array<std::pair<int,int>, 8> queen_dirs = {{
                        {1,1}, {1,-1}, {-1,-1}, {-1,1},
                        {0,1}, {0,-1}, {1,0}, {-1,0}
                    }};
                    for (const auto& [row_dir, col_dir]: queen_dirs){
                        int end_row = start_row + row_dir;
                        int end_col = start_col + col_dir;

                        while (in_bounds(end_row, end_col)){
                            Piece TARGET = board[end_row][end_col];

                            if (TARGET == EMPTY){
                                moves.push_back({start_row, end_row, start_col, end_col});
                            }
                            else if (is_white(TARGET) != is_white(PIECE_MOVED)){
                                moves.push_back({start_row, end_row, start_col, end_col});
                                break;
                            }
                            else{
                                break;
                            }

                            end_row += row_dir;
                            end_col += col_dir;
                        }
                    }
                    break;
                }

                case WHITEKING:
                case BLACKKING: {
                    const std::array<std::pair<int,int>, 8> king_dirs = {{
                        {0,1}, {0,-1}, {1,1}, {1,-1}, {1,0}, {-1,0}, {-1,1}, {-1,-1}
                    }};
                    for (const auto& [row_dir, col_dir] : king_dirs){
                        int end_row = start_row + row_dir;
                        int end_col = start_col + col_dir;

                        if (!in_bounds(end_row, end_col)){
                            continue;
                        }

                        Piece TARGET = board[end_row][end_col];
                        if (TARGET == EMPTY || is_white(TARGET) != is_white(PIECE_MOVED)){
                            moves.push_back({start_row, end_row, start_col, end_col});
                        }
                    }

                    // castling (pseudo-legal; legality filtered by generate_legal_moves + attacked-square checks here)
                    if (PIECE_MOVED == WHITEKING && start_row == 7 && start_col == 4){
                        // king side
                        if ((castling_rights & 1) &&
                            board[7][5] == EMPTY && board[7][6] == EMPTY &&
                            !is_square_attacked(7,4,false) && !is_square_attacked(7,5,false) && !is_square_attacked(7,6,false)){
                            Move m{7,7,4,6};
                            m.flags = MOVE_CASTLE;
                            moves.push_back(m);
                        }
                        // queen side
                        if ((castling_rights & 2) &&
                            board[7][1] == EMPTY && board[7][2] == EMPTY && board[7][3] == EMPTY &&
                            !is_square_attacked(7,4,false) && !is_square_attacked(7,3,false) && !is_square_attacked(7,2,false)){
                            Move m{7,7,4,2};
                            m.flags = MOVE_CASTLE;
                            moves.push_back(m);
                        }
                    }
                    else if (PIECE_MOVED == BLACKKING && start_row == 0 && start_col == 4){
                        if ((castling_rights & 4) &&
                            board[0][5] == EMPTY && board[0][6] == EMPTY &&
                            !is_square_attacked(0,4,true) && !is_square_attacked(0,5,true) && !is_square_attacked(0,6,true)){
                            Move m{0,0,4,6};
                            m.flags = MOVE_CASTLE;
                            moves.push_back(m);
                        }
                        if ((castling_rights & 8) &&
                            board[0][1] == EMPTY && board[0][2] == EMPTY && board[0][3] == EMPTY &&
                            !is_square_attacked(0,4,true) && !is_square_attacked(0,3,true) && !is_square_attacked(0,2,true)){
                            Move m{0,0,4,2};
                            m.flags = MOVE_CASTLE;
                            moves.push_back(m);
                        }
                    }

                    break;
                }

                default:
                    break;
            }
        }
    }
}

bool GameState::is_draw_insufficient_material() const {
    int white_minor = 0;
    int black_minor = 0;
    int white_other = 0;
    int black_other = 0;

    for (int r = 0; r < 8; ++r){
        for (int c = 0; c < 8; ++c){
            Piece p = board[r][c];
            if (p == EMPTY || p == WHITEKING || p == BLACKKING) continue;

            bool w = is_white(p);
            if (p == WHITEBISHOP || p == WHITEKNIGHT || p == BLACKBISHOP || p == BLACKKNIGHT){
                if (w) white_minor++; else black_minor++;
            } else {
                if (w) white_other++; else black_other++;
            }
        }
    }

    // K vs K
    if (white_minor == 0 && black_minor == 0 && white_other == 0 && black_other == 0) return true;
    // K+minor vs K
    if (white_other == 0 && black_other == 0){
        if ((white_minor == 1 && black_minor == 0) || (white_minor == 0 && black_minor == 1)) return true;
    }

    return false;
}

bool GameState::is_checkmate() {
    std::vector<Move> moves;
    generate_legal_moves(moves);

    Position king_pos = white_to_move ? white_king_pos : black_king_pos;
    if (is_in_check(king_pos.row, king_pos.col, white_to_move) && moves.empty()) return true;
    return false;
}

bool GameState::is_stalemate() {
    std::vector<Move> moves;
    generate_legal_moves(moves);

    Position king_pos = white_to_move ? white_king_pos : black_king_pos;
    if (!is_in_check(king_pos.row, king_pos.col, white_to_move) && moves.empty()) return true;
    return false;
}

void GameState::generate_legal_moves(std::vector<Move>& moves) {
    std::vector<Move> pseudo;
    generate_pseudolegal_moves(pseudo);

    moves.clear();
    moves.reserve(pseudo.size());

    for (const auto& m : pseudo) {
        Undo u = make_move(m);

        // after make_move side-to-move flips; the mover is the opposite
        bool mover_is_white = !white_to_move;
        Position king_pos = mover_is_white ? white_king_pos : black_king_pos;

        bool illegal = is_in_check(king_pos.row, king_pos.col, mover_is_white);

        undo_move(m, u);

        if (!illegal) {
            moves.push_back(m);
        }
    }
}

namespace {
    constexpr int BOARD_SIZE = 8;
    constexpr int MAX_HALF_MOVES_FOR_FIFTY_MOVE_RULE = 100;

    // splitmix64 constants (public-domain mixer)
    constexpr uint64_t SM64_GAMMA = 0x9e3779b97f4a7c15ULL;
    constexpr uint64_t SM64_MUL1  = 0xbf58476d1ce4e5b9ULL;
    constexpr uint64_t SM64_MUL2  = 0x94d049bb133111ebULL;

    // position-hash domain separators
    constexpr uint64_t HASH_PIECE_SEED   = 0xA5A5A5A5A5A5A5A5ULL;
    constexpr uint64_t HASH_WHITE_TO_MOVE = 0xABCDEF01ULL;
    constexpr uint64_t HASH_BLACK_TO_MOVE = 0x10FEDCBAULL;
    constexpr uint64_t HASH_CASTLING_SEED = 0xC0FFEEULL;
    constexpr uint64_t HASH_EPFILE_SEED   = 0xE11EULL;
}

static inline uint64_t splitmix64(uint64_t x) {
    x += SM64_GAMMA;
    x = (x ^ (x >> 30)) * SM64_MUL1;
    x = (x ^ (x >> 27)) * SM64_MUL2;
    return x ^ (x >> 31);
}

uint64_t GameState::compute_hash() const {
    uint64_t h = 0;

    // board
    for (int r = 0; r < BOARD_SIZE; ++r) {
        for (int c = 0; c < BOARD_SIZE; ++c) {
            Piece p = board[r][c];
            if (p == EMPTY) continue;
            uint64_t key = (uint64_t)p;
            uint64_t sq = (uint64_t)(r * BOARD_SIZE + c);
            h ^= splitmix64((key << 6) ^ sq ^ HASH_PIECE_SEED);
        }
    }

    // side to move
    h ^= splitmix64(white_to_move ? HASH_WHITE_TO_MOVE : HASH_BLACK_TO_MOVE);

    // castling rights
    h ^= splitmix64(HASH_CASTLING_SEED ^ (uint64_t)castling_rights);

    // en-passant file (only file matters for repetition)
    if (en_passant.row != -1) {
        h ^= splitmix64(HASH_EPFILE_SEED ^ (uint64_t)en_passant.col);
    }

    return h;
}

bool GameState::is_draw_fifty_move_rule() const {
    return halfmove_clock >= MAX_HALF_MOVES_FOR_FIFTY_MOVE_RULE;
}

bool GameState::is_draw_threefold_repetition() const {
    uint64_t cur = hash;
    int count = 0;
    for (uint64_t h : position_hashes) {
        if (h == cur) count++;
    }
    return count >= 3;
}


