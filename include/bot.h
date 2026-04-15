#ifndef BOT_H
#define BOT_H

#include "types.h"
#include <vector>

class GameState;

Move choose_bot_move(GameState& gs, const std::vector<Move>& legal_moves);

#endif
