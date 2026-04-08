#ifndef BOARD_H
#define BOARD_H

#include "types.h"

Board create_board();
bool validate_move_helper(const Move& move, const Board& board);

#endif
