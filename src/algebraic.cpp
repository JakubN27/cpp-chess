#include "../include/algebraic.h"
#include "../include/gamestate.h"
#include "../include/piece.h"
#include <cctype>
#include <algorithm>

static char file_char(int col) { return (char)('a' + col); }
static char rank_char(int row) { return (char)('8' - row); }

std::string move_to_uci(const Move& m) {
    std::string s;
    s += file_char(m.start_col);
    s += rank_char(m.start_row);
    s += file_char(m.end_col);
    s += rank_char(m.end_row);
    if (m.flags & MOVE_PROMOTION) {
        char pc = std::tolower(piece_to_char(m.promotion));
        s += pc;
    }
    return s;
}

static bool is_piece_of_type(Piece p, char sanPiece) {
    switch (sanPiece) {
        case 'N': return p == WHITEKNIGHT || p == BLACKKNIGHT;
        case 'B': return p == WHITEBISHOP || p == BLACKBISHOP;
        case 'R': return p == WHITEROOK || p == BLACKROOK;
        case 'Q': return p == WHITEQUEEN || p == BLACKQUEEN;
        case 'K': return p == WHITEKING || p == BLACKKING;
        default: return false;
    }
}

static Piece promotion_piece_for(bool white, char c) {
    c = (char)std::toupper((unsigned char)c);
    if (white) {
        if (c == 'Q') return WHITEQUEEN;
        if (c == 'R') return WHITEROOK;
        if (c == 'B') return WHITEBISHOP;
        if (c == 'N') return WHITEKNIGHT;
    } else {
        if (c == 'Q') return BLACKQUEEN;
        if (c == 'R') return BLACKROOK;
        if (c == 'B') return BLACKBISHOP;
        if (c == 'N') return BLACKKNIGHT;
    }
    return EMPTY;
}

static std::string square_name(int row, int col){
    std::string s;
    s += file_char(col);
    s += rank_char(row);
    return s;
}

static char piece_letter(Piece p){
    switch (p){
        case WHITEKNIGHT: case BLACKKNIGHT: return 'N';
        case WHITEBISHOP: case BLACKBISHOP: return 'B';
        case WHITEROOK:   case BLACKROOK:   return 'R';
        case WHITEQUEEN:  case BLACKQUEEN:  return 'Q';
        case WHITEKING:   case BLACKKING:   return 'K';
        default: return 0;
    }
}

static bool is_same_piece_type(Piece a, Piece b){
    if (a == EMPTY || b == EMPTY) return false;
    return piece_letter(a) == piece_letter(b);
}

static bool is_capture(const GameState& gs, const Move& m){
    const Board& b = gs.get_board();
    if (m.flags & MOVE_EN_PASSANT) return true;
    return b[m.end_row][m.end_col] != EMPTY;
}

static bool would_give_check_or_mate(GameState& gs_after_move, bool& mate_out){
    std::vector<Move> replies;
    gs_after_move.generate_legal_moves(replies);

    bool side_to_move_white = gs_after_move.is_white_to_move();
    Position king = side_to_move_white ? gs_after_move.get_white_king_pos() : gs_after_move.get_black_king_pos();

    bool in_check = gs_after_move.is_in_check(king.row, king.col, side_to_move_white);
    mate_out = in_check && replies.empty();
    return in_check;
}

std::string move_to_san(GameState& gs, const Move& m) {
    const Board& b = gs.get_board();
    Piece moved = b[m.start_row][m.start_col];

    // Castling
    if (m.flags & MOVE_CASTLE){
        std::string s = (m.end_col == 6) ? "O-O" : "O-O-O";

        GameState tmp = gs;
        tmp.make_move(m);
        bool mate = false;
        bool check = would_give_check_or_mate(tmp, mate);
        if (mate) s += '#';
        else if (check) s += '+';
        return s;
    }

    bool cap = is_capture(gs, m);

    std::string san;

    char pl = piece_letter(moved);
    if (pl){
        san += pl;

        // Disambiguation
        std::vector<Move> legal;
        gs.generate_legal_moves(legal);
        bool needFile = false;
        bool needRank = false;

        for (const auto& other : legal){
            if (other.start_row == m.start_row && other.start_col == m.start_col) continue;
            if (other.end_row != m.end_row || other.end_col != m.end_col) continue;

            Piece om = b[other.start_row][other.start_col];
            if (!is_same_piece_type(om, moved)) continue;

            if (other.start_col != m.start_col) needFile = true;
            if (other.start_row != m.start_row) needRank = true;
        }

        if (needFile) san += file_char(m.start_col);
        if (needRank) san += rank_char(m.start_row);

        if (cap) san += 'x';

        san += square_name(m.end_row, m.end_col);
    } else {
        // Pawn
        if (cap){
            san += file_char(m.start_col);
            san += 'x';
        }
        san += square_name(m.end_row, m.end_col);

        if (m.flags & MOVE_PROMOTION){
            san += '=';
            san += (char)std::toupper((unsigned char)piece_to_char(m.promotion));
        }
    }

    GameState tmp = gs;
    tmp.make_move(m);
    bool mate = false;
    bool check = would_give_check_or_mate(tmp, mate);
    if (mate) san += '#';
    else if (check) san += '+';

    return san;
}

static std::string trim_spaces(const std::string& s){
    std::string out;
    out.reserve(s.size());
    for (char ch : s){
        if (!std::isspace((unsigned char)ch)) out.push_back(ch);
    }
    return out;
}

bool parse_san_move(GameState& gs, const std::string& sanIn, const std::vector<Move>& legal_moves, Move& out) {
    std::string san = trim_spaces(sanIn);
    if (san.empty()) return false;

    // accept common alternative: 0-0 / 0-0-0
    if (san == "0-0") san = "O-O";
    if (san == "0-0-0") san = "O-O-O";

    // First try exact SAN match against generated SAN strings.
    std::vector<Move> candidates;
    for (const auto& m : legal_moves){
        std::string msan = move_to_san(gs, m);
        if (msan == san){
            candidates.push_back(m);
        }
    }
    if (candidates.size() == 1){
        out = candidates.front();
        return true;
    }

    // Fallback: allow users to omit +/# and still match.
    auto strip_suffix = [](std::string x){
        while (!x.empty() && (x.back() == '+' || x.back() == '#')) x.pop_back();
        return x;
    };
    std::string base = strip_suffix(san);
    candidates.clear();
    for (const auto& m : legal_moves){
        std::string msan = strip_suffix(move_to_san(gs, m));
        if (msan == base){
            candidates.push_back(m);
        }
    }

    if (candidates.size() != 1) return false;
    out = candidates.front();
    return true;
}
