#ifndef ALGEBRAIC_H
#define ALGEBRAIC_H

#include "types.h"
#include <string>
#include <vector>

class GameState;

std::string move_to_uci(const Move& m);
std::string move_to_san(GameState& gs, const Move& m);

bool parse_san_move(GameState& gs, const std::string& san, const std::vector<Move>& legal_moves, Move& out);

#endif
