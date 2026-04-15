#ifndef GAMESTATE_H
#define GAMESTATE_H

#include "types.h"
#include <vector>
#include <cstdint>

class GameState {
private:
    Board board; 
    bool white_to_move;
    Position white_king_pos;
    Position black_king_pos;
    std::vector<Undo> history;

    // Full chess state
    uint8_t castling_rights; // bitmask: 1=WK,2=WQ,4=BK,8=BQ
    Position en_passant;     // {-1,-1} means none (square that can be captured onto)
    int halfmove_clock;
    int fullmove_number;

    // Repetition / hashing
    uint64_t hash;
    std::vector<uint64_t> position_hashes;

    void update_king(const Move& move);
    uint64_t compute_hash() const;

public:
    // Constructor
    GameState();

    // Construct from a custom board.
    // This will scan the board once to locate kings and initialize caches.
    explicit GameState(const Board& custom_board);

    // Construct from a custom board with explicit king caches.
    GameState(const Board& custom_board, Position whiteKingPos, Position blackKingPos, bool whiteToMove = true);
    
    // Core gameplay functions
    bool validate_move(const Move& move);

    // NOTE: For engine/search use, make/unmake must be fully reversible.
    Undo make_move(const Move& move);
    void undo_move(const Move& move, const Undo& undo);

    bool is_square_attacked(int row, int col, bool by_white) const;
    bool is_in_check(int row, int col, bool white) const;

    // Move generation
    void generate_pseudolegal_moves(std::vector<Move>& moves) const;
    void generate_legal_moves(std::vector<Move>& moves);
    
    // Utility functions
    void print_board();
    void capture_piece(const Move& move);

    // Game status helpers
    bool is_checkmate();
    bool is_stalemate();
    bool is_draw_insufficient_material() const;
    bool is_draw_fifty_move_rule() const;
    bool is_draw_threefold_repetition() const;

    // GUI/debug access
    const Board& get_board() const { return board; }
    bool is_white_to_move() const { return white_to_move; }
    uint8_t get_castling_rights() const { return castling_rights; }
    Position get_en_passant() const { return en_passant; }
    int get_halfmove_clock() const { return halfmove_clock; }
    int get_fullmove_number() const { return fullmove_number; }
    uint64_t get_hash() const { return hash; }

    Position get_white_king_pos() const { return white_king_pos; }
    Position get_black_king_pos() const { return black_king_pos; }
};

#endif
