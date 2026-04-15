#include "../include/bot.h"
#include "../include/gamestate.h"
#include <stdexcept>
#include <array>
#include <limits>

// Material values indexed by `Piece` enum values (see `include/types.h`).
static constexpr std::array<int, 13> kMaterial = {
    /* 0  EMPTY */ 0,
    /* 1  WHITEPAWN */ 100,
    /* 2  BLACKPAWN */ -100,
    /* 3  WHITEKNIGHT */ 320,
    /* 4  BLACKKNIGHT */ -320,
    /* 5  WHITEBISHOP */ 330,
    /* 6  BLACKBISHOP */ -330,
    /* 7  WHITEROOK */ 500,
    /* 8  BLACKROOK */ -500,
    /* 9  WHITEQUEEN */ 900,
    /* 10 BLACKQUEEN */ -900,
    /* 11 WHITEKING */ 999,
    /* 12 BLACKKING */ -999
};

Move choose_bot_move(GameState& gs, const std::vector<Move>& legal_moves) {
    if (legal_moves.empty()) {
        throw std::runtime_error("No legal moves");
    }

    const int root_colour = gs.is_white_to_move() ? 1 : -1;

    Move best_move = legal_moves.front();
    int best_score = INT_MIN;

    for (const Move& m : legal_moves) {
        Undo u = gs.make_move(m);
        // After we make a move, it's the opponent's turn, so the node colour flips.
        const int score = -negamax(gs, 3, -root_colour);
        gs.undo_move(m, u);

        if (score > best_score) {
            best_score = score;
            best_move = m;
        }
    }

    return best_move;
}

int evaluate_position(const GameState& gs) {
    int score = 0;
    const Board& b = gs.get_board();

    for (const auto& row : b) {
        for (Piece piece : row) {
            score += kMaterial[static_cast<size_t>(piece)];
        }
    }
    return score;
}

//negamax returns an int, but we will decide which move to make 
// by running it to evaluate all immediate possible moves
int negamax(GameState& gs, int depth, int colour) {
    std::vector<Move> move_list;
    gs.generate_legal_moves(move_list);

    if (depth == 0 || move_list.empty()) {
        return colour * evaluate_position(gs);
    }

    int max_value = std::numeric_limits<int>::min();
    for (const Move& m : move_list) {
        Undo u = gs.make_move(m);
        const int value = -negamax(gs, depth - 1, -colour);
        gs.undo_move(m, u);
        max_value = std::max(max_value, value);
    }

    return max_value;
}