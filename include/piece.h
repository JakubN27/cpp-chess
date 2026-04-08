#ifndef PIECE_H
#define PIECE_H

#include "types.h"

char piece_to_char(Piece p);
bool is_white(Piece p);
bool in_bounds(int row, int col);

#endif
