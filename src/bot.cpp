#include "../include/bot.h"
#include "../include/gamestate.h"
#include <stdexcept>

Move choose_bot_move(GameState&, const std::vector<Move>& legal_moves) {
    if (legal_moves.empty()) {
        throw std::runtime_error("No legal moves");
    }

    return legal_moves.front();
}