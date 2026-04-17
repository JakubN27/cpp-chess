#include "../include/bot.h"
#include "../include/gamestate.h"
#include "../include/algebraic.h"
#include "../include/piece.h"
#include <stdexcept>
#include <array>
#include <unordered_map>
#include <algorithm>
#include <cstdlib>
#include <iostream>

// global counters for logging
int g_nodes_visited = 0;
int g_prunes = 0;

enum TTFlag : uint8_t{
    EXACT,
    LOWER,
    UPPER
};

//entry for transposition table
struct PositionInfo{
    int depth;
    int eval;
    TTFlag flag;
};

std::unordered_map<uint64_t, PositionInfo> TranspositionTable{};

// material values indexed by `Piece` enum values (see `include/types.h`)
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

static constexpr int INF = 1000000;

static int piece_value_abs(Piece p) {
    return std::abs(kMaterial[static_cast<size_t>(p)]);
}

static int move_priority_score(const GameState& gs, const Move& m) {
    const Board& b = gs.get_board();
    const Piece mover = b[m.start_row][m.start_col];

    Piece captured = b[m.end_row][m.end_col];
    if ((m.flags & MOVE_EN_PASSANT) != 0) {
        captured = is_white(mover) ? BLACKPAWN : WHITEPAWN;
    }

    int score = 0;

    // captures first
    if (captured != EMPTY) {
        score += 10000 + (10 * piece_value_abs(captured)) - piece_value_abs(mover);
    }

    // promotions next
    if ((m.flags & MOVE_PROMOTION) != 0) {
        score += 8000 + piece_value_abs(m.promotion);
    }

    return score;
}

Move choose_bot_move(GameState& gs, std::vector<Move>& legal_moves) {
    g_nodes_visited = 0;
    g_prunes = 0;
    order_moves(gs, legal_moves);
    if (legal_moves.empty()) {
        throw std::runtime_error("No legal moves");
    }

    const int root_colour = gs.is_white_to_move() ? 1 : -1;

    Move best_move = legal_moves.front();
    int best_score = -INF;
    int alpha = -INF;
    std::vector<int> root_scores;
    root_scores.reserve(legal_moves.size());

    for (const Move& m : legal_moves) {
        Undo u = gs.make_move(m);
        const int score = -negamax(gs, 5, -root_colour, -INF, -alpha);
        gs.undo_move(m, u);
        root_scores.push_back(score);

        if (score > best_score) {
            best_score = score;
            best_move = m;
        }
        alpha = std::max(alpha, score);
    }
    std::cout << "Nodes visited: " << g_nodes_visited << ", Prunes: " << g_prunes << std::endl;
    // Print root move order and scores
    std::cout << "Root move order and scores (" << (gs.is_white_to_move() ? "White" : "Black") << " to move):\n";
    for (size_t i = 0; i < legal_moves.size(); ++i) {
        const Move& m = legal_moves[i];
        std::string san = move_to_san(gs, m);
        std::cout << "  " << san << " | Score: " << root_scores[i] << std::endl;
    }
    return best_move;
}

void order_moves(GameState& gs, std::vector<Move>& moves){
    std::sort(moves.begin(), moves.end(), [&](const Move& a, const Move& b) {
        return move_priority_score(gs, a) > move_priority_score(gs, b);
    });
}

//Check TT for an evaluation, if its not there or less depth, evaluate
bool check_transposition_table(const GameState& gs, int depth, int& alpha, int& beta, int& eval){
    uint64_t key = gs.get_hash();

    if (!TranspositionTable.contains(key)){
        return false;
    }

    const PositionInfo& entry = TranspositionTable.at(key);

    if (entry.depth < depth){
        return false;
    }

    eval = entry.eval;

    switch (entry.flag){
        case EXACT:
            return true;
        case LOWER:
            alpha = std::max(alpha, entry.eval);
            break;
        case UPPER:
            beta = std::min(beta, entry.eval);
            break;
    }

    return alpha >= beta;
}




int evaluate_material(const GameState& gs) {
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

//Alpha beta pruning, keep track of our best (alpha) and opponents best (beta)
//Prune branch if position better than beta (assume opponent makes best move)
//Or if worse than alpha (pointless). We swap and negate them between iterations for negamaxos 
int negamax(GameState& gs, int depth, int colour, int alpha, int beta) {
    ++g_nodes_visited;

    const int alpha_entry = alpha;
    const int beta_entry = beta;

    std::vector<Move> move_list;
    int eval = -INF;

    // memoisation
    if (check_transposition_table(gs, depth, alpha, beta, eval)) {
        return eval;
    }

    gs.generate_legal_moves(move_list);

    if (depth == 0 || move_list.empty()) {
        return colour * evaluate_material(gs);
    }

    order_moves(gs, move_list);

    for (const Move& m : move_list) {
        Undo u = gs.make_move(m);
        int value = -negamax(gs, depth - 1, -colour, -beta, -alpha);
        gs.undo_move(m, u);

        eval = std::max(eval, value);
        alpha = std::max(alpha, value);
        if (alpha >= beta) {
            ++g_prunes;
            break;
        }
    }

    TTFlag flag = EXACT;
    if (eval <= alpha_entry) {
        flag = UPPER;
    } else if (eval >= beta_entry) {
        flag = LOWER;
    } else {
        flag = EXACT;
    }

    TranspositionTable.insert_or_assign(gs.get_hash(), PositionInfo{depth, eval, flag});

    return eval;
}
