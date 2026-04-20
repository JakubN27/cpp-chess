#ifndef BOT_H
#define BOT_H

#include "types.h"
#include <vector>

class GameState;

Move choose_bot_move(GameState& gs, std::vector<Move>& legal_moves);
int evaluate_heuristics(const GameState & gs);
int evaluate_material(const GameState& gs);
int evaluate_position(const GameState& gs);
int negamax(GameState& gs, int depth, int colour, int alpha, int beta);
void order_moves(GameState& gs, std::vector<Move>& moves);
bool check_transposition_table(const GameState& gs, int depth, int& alpha, int& beta, int& eval);

#endif
