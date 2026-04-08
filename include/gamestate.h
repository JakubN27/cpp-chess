#ifndef GAMESTATE_H
#define GAMESTATE_H

#include "types.h"

class GameState {
private:
    Board board; 
    bool white_to_move;
    Position white_king_pos;
    Position black_king_pos;
    
public:
    // Constructor
    GameState();
    
    // Core gameplay functions
    bool validate_move(const Move& move);
    bool make_move(const Move& move);
    bool is_square_attacked(int row, int col, bool by_white);
    bool is_in_check(int row, int col, bool white);
    
    // Utility functions
    void print_board();
    void capture_piece(const Move& move);
};

#endif
