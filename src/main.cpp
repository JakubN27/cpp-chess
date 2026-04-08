#include "../include/gamestate.h"

void test_all_piece_moves();

int main() {
    GameState gs;

    gs.print_board();

    test_all_piece_moves();

    return 0;
}
