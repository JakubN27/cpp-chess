#include "../include/board.h"
#include <cassert>
#include <iostream>

void test_all_piece_moves() {
    {
        Board board{};
        board[6][4] = WHITEPAWN;

        assert(validate_move_helper({6, 5, 4, 4}, board) == true);
        assert(validate_move_helper({6, 4, 4, 4}, board) == true);
        assert(validate_move_helper({6, 3, 4, 4}, board) == false);
        assert(validate_move_helper({6, 5, 4, 5}, board) == false);
        assert(validate_move_helper({6, 7, 4, 4}, board) == false);
    }

    {
        Board board{};
        board[1][4] = BLACKPAWN;

        assert(validate_move_helper({1, 2, 4, 4}, board) == true);
        assert(validate_move_helper({1, 3, 4, 4}, board) == true);
        assert(validate_move_helper({1, 4, 4, 4}, board) == false);
        assert(validate_move_helper({1, 2, 4, 5}, board) == false);
        assert(validate_move_helper({1, 0, 4, 4}, board) == false);
    }

    {
        Board board{};
        board[4][4] = WHITEKNIGHT;

        assert(validate_move_helper({4, 6, 4, 5}, board) == true);
        assert(validate_move_helper({4, 5, 4, 6}, board) == true);
        assert(validate_move_helper({4, 2, 4, 3}, board) == true);
        assert(validate_move_helper({4, 3, 4, 2}, board) == true);
        assert(validate_move_helper({4, 5, 4, 5}, board) == false);
        assert(validate_move_helper({4, 4, 4, 6}, board) == false);
    }

    {
        Board board{};
        board[4][4] = WHITEBISHOP;

        assert(validate_move_helper({4, 6, 4, 6}, board) == true);
        assert(validate_move_helper({4, 2, 4, 2}, board) == true);
        assert(validate_move_helper({4, 4, 4, 7}, board) == false);
        assert(validate_move_helper({4, 5, 4, 6}, board) == false);
    }

    {
        Board board{};
        board[4][4] = WHITEBISHOP;
        board[5][5] = WHITEPAWN;

        assert(validate_move_helper({4, 6, 4, 6}, board) == false);
    }

    {
        Board board{};
        board[4][4] = WHITEROOK;

        assert(validate_move_helper({4, 7, 4, 4}, board) == true);
        assert(validate_move_helper({4, 1, 4, 4}, board) == true);
        assert(validate_move_helper({4, 4, 4, 0}, board) == true);
        assert(validate_move_helper({4, 6, 4, 6}, board) == false);
    }

    {
        Board board{};
        board[4][4] = WHITEROOK;
        board[4][6] = WHITEPAWN;

        assert(validate_move_helper({4, 4, 4, 7}, board) == false);
    }

    {
        Board board{};
        board[4][4] = WHITEQUEEN;

        assert(validate_move_helper({4, 7, 4, 4}, board) == true);
        assert(validate_move_helper({4, 1, 4, 1}, board) == true);
        assert(validate_move_helper({4, 6, 4, 5}, board) == false);
    }

    {
        Board board{};
        board[4][4] = WHITEQUEEN;
        board[5][5] = WHITEPAWN;
        board[4][2] = WHITEPAWN;

        assert(validate_move_helper({4, 7, 4, 7}, board) == false);
        assert(validate_move_helper({4, 4, 4, 0}, board) == false);
    }

    {
        Board board{};
        board[4][4] = WHITEKING;

        assert(validate_move_helper({4, 5, 4, 4}, board) == true);
        assert(validate_move_helper({4, 3, 4, 4}, board) == true);
        assert(validate_move_helper({4, 4, 4, 5}, board) == true);
        assert(validate_move_helper({4, 5, 4, 5}, board) == true);
        assert(validate_move_helper({4, 6, 4, 4}, board) == false);
        assert(validate_move_helper({4, 4, 4, 6}, board) == false);
    }

    std::cout << "All regular piece movement tests passed.\n";
}
