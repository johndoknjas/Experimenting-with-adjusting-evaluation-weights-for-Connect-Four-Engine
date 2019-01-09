#pragma once

#include <iostream>
#include <vector>

using namespace std;

struct coordinate
{
    int row;
    int col;
};

struct treasure_spot
{
    // stores the coordinate of a square that, if filled, makes a 3-in-a-row or 4-in-a-row
    // Also stores the coordinates of the two squares that would extend the newly formed hypothetical line of pieces
        // (which is formed after current_square gets filled).

    coordinate current_square;
    coordinate next_square;
    coordinate other_next_square; // Version 19!
};

struct position_info_for_TT // The key info of a position that will be stored in the transposition table.
{
    vector<vector<char>> board; // The position's 2-D vector of char board. Acts as the KEY!
    int evaluation;
    int calculation_depth_from_this_position; // stores how far ahead the computer calculated for getting the position's evaluation.
    vector<coordinate> possible_moves_sorted; // stores the position's possible moves, sorted from probable best to probable worst.
    bool is_evaluation_indisputable; // stores true if there this position's evaluation will not change by calculating deeper...
                                     // someone has a forced win/forced draw. Someone could have just won/drawn in this position too.
    bool is_comp_turn; // stores true if it's the computer's turn in the position.
};

struct coordinate_and_value // Object for storing amplifying squares along with their respective values.
{
    coordinate square; // stores the coordinates of the amplifying square.
    double value; // stores the value of the amplifying square, determined by the heuristics in smart_evaluation() and the function it calls.
};

bool operator==(const coordinate& first, const coordinate& second) // function tests for equality between two coordinate objects
{
    return (first.row == second.row && first.col == second.col);
}

bool operator!=(const coordinate& first, const coordinate& second) // function tests for inequality between two coordinate objects.
{
    return !(first == second);
}

bool operator==(const treasure_spot& first, const treasure_spot& second)
{
    return (first.current_square == second.current_square && first.next_square == second.next_square);
}

/*
ostream& operator<<(ostream& os, const coordinate& square)
{
    os << "(" << square.row << "," << square.col << ")";

    return os;
}
*/
