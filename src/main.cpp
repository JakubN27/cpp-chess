#include "../include/gamestate.h"
#include "../include/bot.h"
#include "../include/cli.h"
#include "../include/algebraic.h"
#include "../include/piece.h"
#include "../include/lichess.h"
#include <iostream>
#include <string>
#include <vector>

static void print_board_ascii(const GameState& gs) {
    const Board& b = gs.get_board();
    std::cout << "\n";
    for (int r = 0; r < 8; ++r) {
        std::cout << (8 - r) << " ";
        for (int c = 0; c < 8; ++c) {
            std::cout << piece_to_char(b[r][c]) << " ";
        }
        std::cout << "\n";
    }
    std::cout << "  a b c d e f g h\n";
    std::cout << (gs.is_white_to_move() ? "White" : "Black") << " to move\n";
}

static std::string result_text(GameState& gs){
    if (gs.is_checkmate()){
        return gs.is_white_to_move() ? "Checkmate - Black wins" : "Checkmate - White wins";
    }
    if (gs.is_stalemate()){
        return "Stalemate - Draw";
    }
    if (gs.is_draw_threefold_repetition()){
        return "Draw - Threefold repetition";
    }
    if (gs.is_draw_fifty_move_rule()){
        return "Draw - 50-move rule";
    }
    if (gs.is_draw_insufficient_material()){
        return "Draw - Insufficient material";
    }
    return "";
}

int main(int argc, char** argv) {
    BotConfig cfg = parse_bot_config(argc, argv);

    if (cfg.lichess) {
        const char* token = std::getenv("LICHESS_API_KEY");
        if (!token || std::string(token).empty()) {
            std::cerr << "LICHESS_API_KEY not set in environment.\n";
            std::cerr << "Tip: export LICHESS_API_KEY=... before running, or use a tool to load .env.\n";
            return 1;
        }
        LichessConfig lc;
        lc.api_key = token;
        return run_lichess_bot(lc);
    }

    GameState gs;
    std::vector<Move> legal_moves;

    while (true){
        std::string res = result_text(gs);
        if (!res.empty()){
            print_board_ascii(gs);
            std::cout << res << "\n";
            break;
        }

        gs.generate_legal_moves(legal_moves);
        if (legal_moves.empty()){
            print_board_ascii(gs);
            std::cout << "No legal moves.\n";
            break;
        }
        
        int eval = evaluate_position(gs);
        std::cout << "Bot evluation : " << eval;
        print_board_ascii(gs);

        bool bot_to_move = (gs.is_white_to_move() && cfg.white_bot) || (!gs.is_white_to_move() && cfg.black_bot);
        if (bot_to_move){
            Move bm = choose_bot_move(gs, legal_moves);
            std::cout << "Bot plays: " << move_to_san(gs, bm) << "\n";
            gs.make_move(bm);
            continue;
        }

        std::cout << "Enter move (SAN like Nxe4+, O-O, e4, exd5, e8=Q; or 'quit'): ";
        std::string input;
        if (!std::getline(std::cin, input)){
            break;
        }
        if (input == "quit" || input == "exit"){
            break;
        }

        Move chosen{};
        if (!parse_san_move(gs, input, legal_moves, chosen)){
            std::cout << "Invalid or ambiguous move.\n";
            continue;
        }

        gs.make_move(chosen);
    }

    return 0;
}
