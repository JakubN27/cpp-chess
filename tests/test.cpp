#include "../include/gamestate.h"
#include "../include/types.h"
#include <cassert>
#include <iostream>

static Board empty_board_with_kings() {
    Board b{};
    for (auto &row : b) row.fill(EMPTY);
    b[7][4] = WHITEKING;
    b[0][4] = BLACKKING;
    return b;
}

void test_all_piece_moves() {
    {
        Board b = empty_board_with_kings();
        b[6][4] = WHITEPAWN;
        GameState gs(b);

        assert(gs.validate_move({6, 5, 4, 4}) == true);
        assert(gs.validate_move({6, 4, 4, 4}) == true);
        assert(gs.validate_move({6, 3, 4, 4}) == false);
        assert(gs.validate_move({6, 5, 4, 5}) == false);
        assert(gs.validate_move({6, 7, 4, 4}) == false);
    }

    {
        Board b = empty_board_with_kings();
        b[1][4] = BLACKPAWN;
        GameState gs(b);

        assert(gs.validate_move({1, 2, 4, 4}) == true);
        assert(gs.validate_move({1, 3, 4, 4}) == true);
        assert(gs.validate_move({1, 4, 4, 4}) == false);
        assert(gs.validate_move({1, 2, 4, 5}) == false);
        assert(gs.validate_move({1, 0, 4, 4}) == false);
    }

    {
        Board b = empty_board_with_kings();
        b[4][4] = WHITEKNIGHT;
        GameState gs(b);

        assert(gs.validate_move({4, 6, 4, 5}) == true);
        assert(gs.validate_move({4, 5, 4, 6}) == true);
        assert(gs.validate_move({4, 2, 4, 3}) == true);
        assert(gs.validate_move({4, 3, 4, 2}) == true);
        assert(gs.validate_move({4, 5, 4, 5}) == false);
        assert(gs.validate_move({4, 4, 4, 6}) == false);
    }

    {
        Board b = empty_board_with_kings();
        b[4][4] = WHITEBISHOP;
        GameState gs(b);

        assert(gs.validate_move({4, 6, 4, 6}) == true);
        assert(gs.validate_move({4, 2, 4, 2}) == true);
        assert(gs.validate_move({4, 4, 4, 7}) == false);
        assert(gs.validate_move({4, 5, 4, 6}) == false);
    }

    {
        Board b = empty_board_with_kings();
        b[4][4] = WHITEBISHOP;
        b[5][5] = WHITEPAWN;
        GameState gs(b);

        assert(gs.validate_move({4, 6, 4, 6}) == false);
    }

    {
        Board b = empty_board_with_kings();
        b[4][4] = WHITEROOK;
        GameState gs(b);

        assert(gs.validate_move({4, 4, 4, 7}) == true);
        assert(gs.validate_move({4, 4, 4, 1}) == true);
        assert(gs.validate_move({4, 0, 4, 4}) == true);
        assert(gs.validate_move({4, 6, 4, 6}) == false);
    }

    {
        Board b = empty_board_with_kings();
        b[4][4] = WHITEROOK;
        b[4][6] = WHITEPAWN;
        GameState gs(b);

        assert(gs.validate_move({4, 4, 4, 7}) == false);
    }

    {
        Board b = empty_board_with_kings();
        b[4][4] = WHITEQUEEN;
        GameState gs(b);

        assert(gs.validate_move({4, 4, 4, 7}) == true);
        assert(gs.validate_move({4, 1, 4, 1}) == true);
        assert(gs.validate_move({4, 6, 4, 5}) == false);
    }

    {
        Board b = empty_board_with_kings();
        b[4][4] = WHITEQUEEN;
        b[5][5] = WHITEPAWN;
        b[4][2] = WHITEPAWN;
        GameState gs(b);

        assert(gs.validate_move({4, 7, 4, 7}) == false);
        assert(gs.validate_move({4, 4, 4, 0}) == false);
    }

    {
        Board b = empty_board_with_kings();
        b[4][4] = WHITEKING;
        GameState gs(b);

        assert(gs.validate_move({4, 5, 4, 4}) == true);
        assert(gs.validate_move({4, 3, 4, 4}) == true);
        assert(gs.validate_move({4, 4, 4, 5}) == true);
        assert(gs.validate_move({4, 5, 4, 5}) == true);
        assert(gs.validate_move({4, 6, 4, 4}) == false);
        assert(gs.validate_move({4, 4, 4, 6}) == false);
    }

    std::cout << "All regular piece movement tests passed.\n";
}
