#include "../include/piece.h"

char piece_to_char(Piece p) {
    switch (p) {
        case EMPTY: return '.';
        case WHITEPAWN: return 'P';
        case BLACKPAWN: return 'p';
        case WHITEKNIGHT: return 'N';
        case BLACKKNIGHT: return 'n';
        case WHITEBISHOP: return 'B';
        case BLACKBISHOP: return 'b';
        case WHITEROOK: return 'R';
        case BLACKROOK: return 'r';
        case WHITEQUEEN: return 'Q';
        case BLACKQUEEN: return 'q';
        case WHITEKING: return 'K';
        case BLACKKING: return 'k';
    }
    return '?';
}

bool is_white(Piece p) {
    if (p == WHITEPAWN ||
        p == WHITEKNIGHT ||
        p == WHITEBISHOP ||
        p == WHITEROOK ||
        p == WHITEQUEEN ||
        p == WHITEKING){
        return true;
    }
    return false;
}

bool in_bounds(int row, int col) {
    return row < 8 && row >= 0 && col < 8 && col >= 0;
}
