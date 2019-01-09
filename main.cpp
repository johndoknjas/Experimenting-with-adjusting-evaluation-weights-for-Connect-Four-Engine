#include "EngineOne.h"
#include "EngineTwo.h"
#include "CommonNonClassData.h"
#include <iostream>
#include <vector>
#include <time.h>
#include <cstdlib>
#include <fstream>
#include <ctime>
#include <ratio>
#include <chrono>

using namespace std;

using namespace std::chrono;

const int MAX_ROW_INDEX = 5;
const int MAX_COL_INDEX = 6;

struct RunningGameVariables // the variables that need to be recorded and updated as a game plays out in play_game()
{
    vector<vector<char>> board;
    coordinate last_move;
    vector<treasure_spot> squares_amplifying_comp_2;
    vector<treasure_spot> squares_amplifying_comp_3;
    vector<treasure_spot> squares_amplifying_user_2;
    vector<treasure_spot> squares_amplifying_user_3;
};

struct ReadyForFile // represents the info I'll need to print a position to a file.
{
    RunningGameVariables all_data;
    int EngineOneEvaluation;
    int EngineTwoEvaluation;
};

struct SimStats
{
    int num_EngineOne_wins;
    int num_EngineTwo_wins;
    int num_draws;

    int num_starting_positions_where_EngineOne_wins_both;
    int num_starting_positions_where_EngineOne_wins_and_draw;
    int num_starting_positions_where_each_Engine_wins;
    int num_starting_positions_where_two_draws;
    int num_starting_positions_where_EngineTwo_wins_and_draw;
    int num_starting_positions_where_EngineTwo_wins_both;
};

struct mini_match_summary
{
    double ratio_of_points; // from EngineTwo to EngineOne
    double value_of_adjusted_parameter; // from EngineTwo's position class
};

void display_board(const vector<vector<char>>& board) // This function is just for testing purposes, but it works in any case.
{
    cout << "\n    A   B   C   D   E   F   G\n\n";

    for (int row = 0; row <= 5; row++)
    {
        cout << (6 - row) << " | "; // Since I want row numbers to be displayed increasing from bottom up, not top down.

        for (int col = 0; col <= 6; col++)
        {
            cout << board[row][col] << " | ";
        }

        cout << "\n" << "  |---|---|---|---|---|---|---|\n";
    }
}

void print_amplifying_vector(const vector<treasure_spot>& vec) // function is just for testing purposes
{
    for (const treasure_spot& spot: vec)
    {
        cout << "Current square: " << "(" << char('A' + spot.current_square.col) << (6 - spot.current_square.row) << "),  ";
        cout << "Next square: " << "(" << char('A' + spot.next_square.col) << (6 - spot.next_square.row) << ")";
        cout << "\n";
    }

    cout << "\n";
}

void flip_pieces_in_board(vector<vector<char>>& board)
{
    // Function replaces all 'C' with 'U' in board, and 'U' with 'C' in board.

    for (int row = 0; row <= MAX_ROW_INDEX; row++)
    {
        for (int col = 0; col <= MAX_COL_INDEX; col++)
        {
            if (board[row][col] == 'C')
            {
                board[row][col] = 'U';
            }

            else if (board[row][col] == 'U') // else if needed, or this would always be true if above if statement was true.
            {
                board[row][col] = 'C';
            }
        }
    }
}

void initialize_board_and_last_move(RunningGameVariables& all_data, int num_pieces)
{
    if (num_pieces > 42)
    {
        throw runtime_error("num_pieces is greater than 42 in create_random_board() function in main.cpp\n");
    }

    // Function initializes a 7x6 board, and randomly fills it up with num_pieces pieces. Will alternate between
    // placing 'C' and 'U' chars.

    // Function begins to place a 'C' char first.

    // After filling in the board, all_data.board will be set to board, and all_data.last_move will be set to the
    // coordinate of the last piece put in the board.

    vector<vector<char>> board; // will be filled with pieces, and then returned.

    vector<char> single_row;

    for (int col = 0; col <= MAX_COL_INDEX; col++)
    {
        single_row.push_back(' ');
    }

    for (int row = 0; row <= MAX_ROW_INDEX; row++)
    {
        board.push_back(single_row);
    }

    char current_char = 'C';

    vector<coordinate> possible_moves; // stores all legal moves that can be played at any time on board.

    for (int col = 0; col <= MAX_COL_INDEX; col++)
    {
        coordinate temp;
        temp.row = MAX_ROW_INDEX;
        temp.col = col;
        possible_moves.push_back(temp);
    }

    for (int i = 1; i <= num_pieces; i++)
    {
        int index = rand() % possible_moves.size();
        coordinate chosen_move = possible_moves[index];

        // Before placing current_char in board at chosen_move, decrease row value of possible_moves[index]:

        possible_moves[index].row--;

        if (possible_moves[index].row < 0)
        {
            possible_moves.erase(possible_moves.begin() + index);
        }

        // Now place current_char in board at chosen_move:

        board[chosen_move.row][chosen_move.col] = current_char;

        // Now flip the current_char (C ==> U   OR   C ==> U):

        if (current_char == 'C')
        {
            current_char = 'U';
        }

        else
        {
            current_char = 'C';
        }

        if (i == num_pieces) // so the for-loop is about to be exited...
        {
            all_data.last_move = chosen_move;
        }
    }

    all_data.board = board;
}

bool do_all_squares_equal_piece(coordinate preceding_square, int num_squares, int change_row_value, int change_col_value,
                                char piece, const vector<vector<char>>& board)
{
    // Function returns true if all the square coordinates in all_data.board = piece.

    // The square coordinates are all the squares from immediately after preceding_square to num_squares
    // in the appropriate direction.
    // Appropriate direction is determined by the values of change_row_value and change_col_value params.

    coordinate current_square = preceding_square;
    current_square.row += change_row_value;
    current_square.col += change_col_value;

    for (int i = 1; i <= num_squares; i++)
    {
        int row = current_square.row;
        int col = current_square.col;

        // First, check if row and col are in-bounds:

        if (row < 0 || row > MAX_ROW_INDEX || col < 0 || col > MAX_COL_INDEX) // square is out-of-bounds...
        {
            return false; // If this square is out of bounds, it != piece. Therefore, all squares != piece.
        }

        // Now, check if the square equals piece in board:

        if (board[row][col] != piece)
        {
            return false; // This square != piece; therefore, all squares != piece.
        }

        // Now to increment current_square.row or current_square.col by a certain amount:

        current_square.row += change_row_value;
        current_square.col += change_col_value;
    }

    return true;
}

void initialize_amplifying_vector(vector<treasure_spot>& amplifying_vector, char piece,
                                  int num_pieces, const vector<vector<char>>& board)
{
    // Function fills in the amplifying_vector param (passed by reference) to store treasure_spots that amplify
    // num_pieces-in-a-row for char piece in board ('C' or 'U').

    amplifying_vector.clear();

    for (int row = 0; row <= MAX_ROW_INDEX; row++)
    {
        for (int col = 0; col <= MAX_COL_INDEX; col++)
        {
            if (board[row][col] != ' ') // not an amplifying square, since it's not empty:
            {
                continue; // skip to next iteration of inner for-loop to look at next square in board.
            }

            // So, at this point board[row][col] is guaranteed to = ' '. Now test if there is a num_pieces-in-a-row
            // around it in any direction...

            treasure_spot spot; // possibly add to the amplifying vector...
            spot.current_square.row = row;
            spot.current_square.col = col;

            if (do_all_squares_equal_piece(spot.current_square, num_pieces, 0, -1, piece, board)) /* Horizontal Left */
            {
                spot.next_square = spot.current_square;
                spot.next_square.col ++;
                amplifying_vector.push_back(spot);
            }

            if (do_all_squares_equal_piece(spot.current_square, num_pieces, 0, 1, piece, board)) /* Horizontal Right */
            {
                spot.next_square = spot.current_square;
                spot.next_square.col --;
                amplifying_vector.push_back(spot);
            }

            if (do_all_squares_equal_piece(spot.current_square, num_pieces, 1, 0, piece, board)) /* Vertical Down */
            {
                spot.next_square = spot.current_square;
                spot.next_square.row --;
                amplifying_vector.push_back(spot);
            }

            if (do_all_squares_equal_piece(spot.current_square, num_pieces, -1, -1, piece, board)) /* Up-Left */
            {
                spot.next_square = spot.current_square;
                spot.next_square.row ++;
                spot.next_square.col ++;
                amplifying_vector.push_back(spot);
            }

            if (do_all_squares_equal_piece(spot.current_square, num_pieces, -1, 1, piece, board)) /* Up-Right */
            {
                spot.next_square = spot.current_square;
                spot.next_square.row ++;
                spot.next_square.col --;
                amplifying_vector.push_back(spot);
            }

            if (do_all_squares_equal_piece(spot.current_square, num_pieces, 1, -1, piece, board)) /* Down-Left */
            {
                spot.next_square = spot.current_square;
                spot.next_square.row --;
                spot.next_square.col ++;
                amplifying_vector.push_back(spot);
            }

            if (do_all_squares_equal_piece(spot.current_square, num_pieces, 1, 1, piece, board)) /* Down-Right */
            {
                spot.next_square = spot.current_square;
                spot.next_square.row --;
                spot.next_square.col --;
                amplifying_vector.push_back(spot);
            }
        }
    }
}

void initialize_all_data(int num_pieces, RunningGameVariables& all_data)
{
    initialize_board_and_last_move(all_data, num_pieces);

    // Each amplifying vector attribute of all_data is passed by reference in the function calls below.

    initialize_amplifying_vector(all_data.squares_amplifying_comp_2, 'C', 2, all_data.board);

    initialize_amplifying_vector(all_data.squares_amplifying_comp_3, 'C', 3, all_data.board);

    initialize_amplifying_vector(all_data.squares_amplifying_user_2, 'U', 2, all_data.board);

    initialize_amplifying_vector(all_data.squares_amplifying_user_3, 'U', 3, all_data.board);
}

unique_ptr<EngineOne::position> get_EngineOne_move(const unique_ptr<EngineOne::position>& p1)
{
    // Function gets EngineOne (p1) to find the best move, and then return the future_position that this best move reaches.

    // Note that even though the POINTER (p1) is const, the position object it points to will change (due to
    // p1->get_future_positions() a few lines down from here).

    // A constant pointer just means the memory address it stores remains constant (not what's AT the memory address!).

    coordinate best_move = p1->find_best_move_for_comp();

    vector<unique_ptr<EngineOne::position>> future_positions = p1->get_future_positions();

    for (int i = 0; i < future_positions.size(); i++)
    {
        if (future_positions[i]->get_last_move() == best_move)
        {
            return move(future_positions[i]);
        }
    }

    throw runtime_error("Nothing returned in get_EngineOne_move() function in main.cpp\n");
}

unique_ptr<EngineTwo::position> get_EngineTwo_move(const unique_ptr<EngineTwo::position>& p2)
{
    // Function gets EngineTwo (p2) to find the best move, and then return the future_position that this best move reaches.

    // Note that even though the POINTER (p2) is const, the position object it points to will change (due to
    // p2->get_future_positions() a few lines down from here).

    // A constant pointer just means the memory address it stores remains constant (not what's AT the memory address!).

    coordinate best_move = p2->find_best_move_for_comp();

    vector<unique_ptr<EngineTwo::position>> future_positions = p2->get_future_positions();

    for (int i = 0; i < future_positions.size(); i++)
    {
        if (future_positions[i]->get_last_move() == best_move)
        {
            return move(future_positions[i]);
        }
    }

    throw runtime_error("Nothing returned in get_EngineTwo_move() function in main.cpp\n");
}

bool in_vector(const vector<vector<char>>& board, const vector<RunningGameVariables>& vec)
{
    // Returns true if board (a 2-D vector of chars) is equal to a board in one of the RunningGameVariables objects in vec.

    for (const RunningGameVariables& temp: vec)
    {
        if (board == temp.board)
        {
            return true;
        }
    }

    return false;
}

RunningGameVariables agree_on_starting_position(vector<RunningGameVariables>& starting_positions_already_used)
{
    // Function gets the engines to agree on a starting position, and then returns a RunningGameVariables object
    // that represents this starting position.

    RunningGameVariables all_data; // object stores all the data needed for this game (must be constantly updated).

    unique_ptr<EngineOne::position> p1; // The pointer representing EngineOne.
    unique_ptr<EngineTwo::position> p2; // The pointer representing EngineTwo.

    steady_clock::time_point start_time = steady_clock::now();

    do
    {
        do
        {
            int num_pieces = rand() % 6 + 1; // I want to make sure num_pieces > 0.
                                             // The statement also ensures num_pieces is never greater than 6. This means
                                             // the initialize_board() function won't have to check if someone won.

            if (num_pieces % 2 != 0) // is odd, no good...
            {
                num_pieces ++;
            }

            initialize_all_data(num_pieces, all_data);

        } while (in_vector(all_data.board, starting_positions_already_used));

        // Get both engines to think about the position represented by all_data:

        p1 = EngineOne::position::think_on_game_position(all_data.board, true, all_data.last_move,
                                                         all_data.squares_amplifying_comp_2, all_data.squares_amplifying_comp_3,
                                                         all_data.squares_amplifying_user_2, all_data.squares_amplifying_user_3,
                                                         true);

        p2 = EngineTwo::position::think_on_game_position(all_data.board, true, all_data.last_move,
                                                         all_data.squares_amplifying_comp_2, all_data.squares_amplifying_comp_3,
                                                         all_data.squares_amplifying_user_2, all_data.squares_amplifying_user_3,
                                                         true);

    } while(p1->get_evaluation() >= 10 || p1->get_evaluation() <= -10 ||
            p2->get_evaluation() >= 10 || p2->get_evaluation() <= -10);

    // Now add the position that was just played to the starting_positions_already_used vector:

    starting_positions_already_used.push_back(all_data);

    duration<double> time_span = duration_cast<duration<double>>(steady_clock::now() - start_time);

    cout << "\n\ntime was: " << (time_span.count()) << " seconds.\n";

    cout << "Starting position the engines \"agreed\" to play:\n";
    display_board(all_data.board);
    cout << "EngineOne evaluation of the position: " << p1->get_evaluation() << "\n";
    cout << "EngineTwo evaluation of the position: " << p2->get_evaluation() << "\n";

    return all_data;
}

void playout_game(bool EngineOneGoesFirst, RunningGameVariables all_data, SimStats& results,
                  double& time_EngineOne_spent_thinking, double& time_EngineTwo_spent_thinking,
                  int& num_moves_played_by_EngineOne, int& num_moves_played_by_EngineTwo)
{
    /*

    // Gets the two computers in EngineOne.h and EngineTwo.h to play a game.

    // In this function, all_data keeps a record of important variables/data that both Engines will need
    // to use as the game goes on. These data (which are constantly updated throughout the game in this function) are:

        // vector<vector<char>> board. The board must store each Engine's move when they choose their best move,
           and then all 'C' and 'U' must be flipped in preparation for the other Engine to think.
           all_data.board is already initialized to an "agreed", fair position for the Engines to play.
        // coordinate last_move must be updated with the best move chosen by either Engine when they choose their move.
        // The 4 amplifying vectors must be updated when either Engine moves, i.e., set equal to the 4 amplifying vectors
           of the new position. Then, the 2 user vectors and 2 comp vectors should be swapped, in preparation for the
           other Engine to think.
    */

    bool EngineOneTurn = false; // This variable determines which Engine gets to move in each turn of the simulation.

    if (EngineOneGoesFirst)
    {
        EngineOneTurn = true;
    }

    unique_ptr<EngineOne::position> p1; // The pointer representing EngineOne.
    unique_ptr<EngineTwo::position> p2; // The pointer representing EngineTwo.

    bool is_new_game_starting_for_Engine_Two = true; // need to send this variable to EngineTwo's "think function"
                                                      // so that it resets the TT when a new game starts.

    bool is_new_game_starting_for_Engine_One = true;

    while (true)
    {
        if (EngineOneTurn)
        {
            steady_clock::time_point start_time = steady_clock::now();

            p1 = EngineOne::position::think_on_game_position(all_data.board, true, all_data.last_move,
                                                             all_data.squares_amplifying_comp_2, all_data.squares_amplifying_comp_3,
                                                             all_data.squares_amplifying_user_2, all_data.squares_amplifying_user_3,
                                                             is_new_game_starting_for_Engine_One);

            duration<double> time_span = duration_cast<duration<double>>(steady_clock::now() - start_time);

            time_EngineOne_spent_thinking += (time_span.count());

            is_new_game_starting_for_Engine_One = false;

            p1 = get_EngineOne_move(p1); // finds the best future_position for comp (according to EngineOne) and gets p1 to
                                         // point to it.
            num_moves_played_by_EngineOne ++;

            if (p1->did_computer_win())
            {
                // EngineOne is playing with the 'C' pieces, so it has just won the game...

                results.num_EngineOne_wins ++;
                cout << "Engine One won.\n";
                return;
            }

            if (p1->is_game_drawn())
            {
                results.num_draws ++;
                cout << "The game is drawn.\n";
                return;
            }

            all_data.board = p1->get_board();
            flip_pieces_in_board(all_data.board);

            all_data.last_move = p1->get_last_move();

            all_data.squares_amplifying_comp_2 = p1->get_squares_amplifying_user_2();
            all_data.squares_amplifying_comp_3 = p1->get_squares_amplifying_user_3();
            all_data.squares_amplifying_user_2 = p1->get_squares_amplifying_comp_2();
            all_data.squares_amplifying_user_3 = p1->get_squares_amplifying_comp_3();
        }

        else // EngineTwo's turn:
        {
            steady_clock::time_point start_time = steady_clock::now();

            p2 = EngineTwo::position::think_on_game_position(all_data.board, true, all_data.last_move,
                                                             all_data.squares_amplifying_comp_2, all_data.squares_amplifying_comp_3,
                                                             all_data.squares_amplifying_user_2, all_data.squares_amplifying_user_3,
                                                             is_new_game_starting_for_Engine_Two);

            duration<double> time_span = duration_cast<duration<double>>(steady_clock::now() - start_time);

            time_EngineTwo_spent_thinking += (time_span.count());

            is_new_game_starting_for_Engine_Two = false; // Since a new game cannot be starting no matter what now, since it
                                                         // just thought.


            p2 = get_EngineTwo_move(p2); // finds the best future_position for comp (according to EngineTwo) and gets p2 to
                                         // point to it.

            num_moves_played_by_EngineTwo ++;

            if (p2->did_computer_win())
            {
                // EngineTwo is playing with the 'C' pieces, so it has just won the game...

                results.num_EngineTwo_wins ++;
                cout << "Engine Two won.\n";
                return;
            }

            if (p2->is_game_drawn())
            {
                results.num_draws ++;
                cout << "The game is drawn.\n";
                return;
            }

            all_data.board = p2->get_board();
            flip_pieces_in_board(all_data.board);

            all_data.last_move = p2->get_last_move();

            all_data.squares_amplifying_comp_2 = p2->get_squares_amplifying_user_2();
            all_data.squares_amplifying_comp_3 = p2->get_squares_amplifying_user_3();
            all_data.squares_amplifying_user_2 = p2->get_squares_amplifying_comp_2();
            all_data.squares_amplifying_user_3 = p2->get_squares_amplifying_comp_3();
        }

        EngineOneTurn = !EngineOneTurn;
    }
}

int getEngineOneEvaluation(const RunningGameVariables& all_data)
{
    unique_ptr<EngineOne::position> p1;

    p1 = EngineOne::position::think_on_game_position(all_data.board, true, all_data.last_move,
                                                     all_data.squares_amplifying_comp_2, all_data.squares_amplifying_comp_3,
                                                     all_data.squares_amplifying_user_2, all_data.squares_amplifying_user_3,
                                                     true);

    return p1->get_evaluation();
}

int getEngineTwoEvaluation(const RunningGameVariables& all_data)
{
    unique_ptr<EngineTwo::position> p2;

    p2 = EngineTwo::position::think_on_game_position(all_data.board, true, all_data.last_move,
                                                     all_data.squares_amplifying_comp_2, all_data.squares_amplifying_comp_3,
                                                     all_data.squares_amplifying_user_2, all_data.squares_amplifying_user_3,
                                                     true);

    return p2->get_evaluation();
}

bool ReadyForFileComparison(const ReadyForFile& first, const ReadyForFile& second)
{
    // Return true if first should GO BEFORE second in the sorted list.
    // first should go before if its EngineOneEvaluation is smaller than second's.
    // If they are equal, then compare their EngineTwoEvaluations.

    if (first.EngineOneEvaluation < second.EngineOneEvaluation)
    {
        return true;
    }

    if (second.EngineOneEvaluation < first.EngineOneEvaluation)
    {
        return false;
    }

    // EngineOneEvaluations are equal, so compare EngineTwoEvaluations:

    return (first.EngineTwoEvaluation < second.EngineTwoEvaluation);
}

void print_position_to_file(const ReadyForFile& temp, ofstream& fout)
{
    fout << "\n    A   B   C   D   E   F   G\n\n";

    for (int row = 0; row <= 5; row++)
    {
        fout << (6 - row) << " | "; // Since I want row numbers to be displayed increasing from bottom up, not top down.

        for (int col = 0; col <= 6; col++)
        {
            fout << temp.all_data.board[row][col] << " | ";
        }

        fout << "\n" << "  |---|---|---|---|---|---|---|\n";
    }

    fout << "\n";

    // Now to print EngineOne and EngineTwo's evaluations:

    fout << "EngineOne's evaluation: " << temp.EngineOneEvaluation << "\n";
    fout << "EngineTwo's evaluation: " << temp.EngineTwoEvaluation << "\n\n\n";
}

void write_all_played_positions_to_file(const vector<RunningGameVariables>& starting_positions_already_used)
{
    // Get EngineOne & EngineTwo's evaluations of each position, and print to a file.

    vector<ReadyForFile> revised_vec;

    for (const RunningGameVariables& temp: starting_positions_already_used)
    {
        ReadyForFile new_object;

        new_object.all_data = temp;

        new_object.EngineOneEvaluation = getEngineOneEvaluation(new_object.all_data);

        new_object.EngineTwoEvaluation = getEngineTwoEvaluation(new_object.all_data);

        revised_vec.push_back(new_object);
    }

    sort(revised_vec.begin(), revised_vec.end(), ReadyForFileComparison); // last param is a comparison function I wrote.

    // Now print to a file:

    ofstream fout("PositionsPlayedInSimulation.txt");

    if (fout.fail())
    {
        cout << "fstream failed in write_all_played_positions_to_file()!\n";
    }

    for (const ReadyForFile& temp: revised_vec)
    {
        print_position_to_file(temp, fout);
    }
}

int main()
{
    srand(time(NULL));

    bool first_mini_match_done = false;

    vector<RunningGameVariables> starting_positions_already_used; // will store all starting positions
                                                                  // that get picked for the Engines to play from.
                                                                  // It is to ensure no starting position is used more than once.

    int num_trials = 0; // How many trials the engines play against each other in each mini-match.

    cout << "Enter how many trials (starting positions) you want run for each mini-match: ";

    cin >> num_trials;

    vector<mini_match_summary> results_for_mini_matches;

    // The current for loop header below tests values from 6.0 to 2.0, at decreasing intervals of 0.2
    for (EngineTwo::position::parameter_being_adjusted = 6.0; EngineTwo::position::parameter_being_adjusted >= 1.99;
         EngineTwo::position::parameter_being_adjusted -= 0.2)
    {

        SimStats results; // object stores number of games EngineOne and EngineTwo win, and number of draws.

        // initializing members of results:

        results.num_draws = 0;
        results.num_EngineOne_wins = 0;
        results.num_EngineTwo_wins = 0;

        results.num_starting_positions_where_EngineOne_wins_both = 0;
        results.num_starting_positions_where_EngineOne_wins_and_draw = 0;
        results.num_starting_positions_where_each_Engine_wins = 0;
        results.num_starting_positions_where_two_draws = 0;
        results.num_starting_positions_where_EngineTwo_wins_and_draw = 0;
        results.num_starting_positions_where_EngineTwo_wins_both = 0;

        bool EngineOneGoesFirst = false; // will be sent to playout_game() function, to determine which Engine gets the first move.

        double time_EngineOne_spent_thinking = 0.0; // will store total time EngineOne thought in gameplay VS EngineTwo overall in sim.
        double time_EngineTwo_spent_thinking = 0.0; // will store total time EngineTwo thought in gameplay VS EngineOne overall in sim.
        int num_moves_played_by_EngineOne = 0; // stores the total number of moves played by EngineOne in games in this simulation.
        int num_moves_played_by_EngineTwo = 0; // stores the total number of moves played by EngineTwo in games in this simulation.

        for (int i = 1; i <= num_trials; i++)
        {
            EngineOneGoesFirst = !EngineOneGoesFirst; // Not really a necessary statement, but it's just to ensure absolute
                                                      // fairness between engines just in case whoever gets the first turn
                                                      // going first in the starting position.

            RunningGameVariables all_data;

            if (!first_mini_match_done) // so on the first_mini_match... no full vector of playable positions yet.
            {
                all_data = agree_on_starting_position(starting_positions_already_used);
            }

            else // Not on the first match. Use the corresponding starting position used in the previous mini-match(es):
            {
                all_data = starting_positions_already_used[i-1];
            }

            int num_EngineOne_wins_old = results.num_EngineOne_wins;
            int num_EngineTwo_wins_old = results.num_EngineTwo_wins;
            int num_draws_old = results.num_draws;

            playout_game(EngineOneGoesFirst, all_data, results, time_EngineOne_spent_thinking, time_EngineTwo_spent_thinking,
                         num_moves_played_by_EngineOne, num_moves_played_by_EngineTwo);
                         // all_data sent by value (original NOT changed, so it can be used again when the other engine goes first).
                         // results is passed by reference, and updated depending on who wins (or if it's a draw).
                         // The two "time" variables are also passed by reference and record how much time Engine spend calculating.

            // OTHER ENGINE GOES FIRST:

            playout_game(!EngineOneGoesFirst, all_data, results, time_EngineOne_spent_thinking, time_EngineTwo_spent_thinking,
                         num_moves_played_by_EngineOne, num_moves_played_by_EngineTwo);

            // Now to update one of the members of results dealing starting_positions:

            if (results.num_draws - num_draws_old == 2) // were two draws for this position...
            {
                results.num_starting_positions_where_two_draws ++;
            }

            else if (results.num_EngineOne_wins - num_EngineOne_wins_old == 2) // EngineOne won twice for this position...
            {
                results.num_starting_positions_where_EngineOne_wins_both ++;
            }

            else if (results.num_EngineTwo_wins - num_EngineTwo_wins_old == 2) // EngineTwo won twice for this position...
            {
                results.num_starting_positions_where_EngineTwo_wins_both ++;
            }

            else if (results.num_draws - num_draws_old == 1 && results.num_EngineOne_wins - num_EngineOne_wins_old == 1)
            {
                results.num_starting_positions_where_EngineOne_wins_and_draw ++;
            }

            else if (results.num_draws - num_draws_old == 1 && results.num_EngineTwo_wins - num_EngineTwo_wins_old == 1)
            {
                results.num_starting_positions_where_EngineTwo_wins_and_draw ++;
            }

            else // Both engines won a game each:
            {
                results.num_starting_positions_where_each_Engine_wins ++;
            }

            cout << "Current mini-match score for adjusted parameter equaling " << EngineTwo::position::parameter_being_adjusted;
            cout << ":\n";
            cout << "EngineOne has " << results.num_EngineOne_wins << "\n";
            cout << "EngineTwo has " << results.num_EngineTwo_wins << "\n";
            cout << "Number of draws are " << results.num_draws << "\n\n";

          /*  cout << "EngineOne has spent " << time_EngineOne_spent_thinking << " seconds thinking overall in mini-match, so far.\n";
            cout << "EngineTwo has spent " << time_EngineTwo_spent_thinking << " seconds thinking overall in mini-match, so far.\n";
            cout << "EngineOne is thinking at a rate of " << time_EngineOne_spent_thinking / double(num_moves_played_by_EngineOne);
            cout << " seconds/move, currently in the simulation.\n";
            cout << "EngineTwo is thinking at a rate of " << time_EngineTwo_spent_thinking / double(num_moves_played_by_EngineTwo);
            cout << " seconds/move, currently in the simulation.\n"; */
        }


        cout << "Mini-match with parameter_being_adjusted = " << EngineTwo::position::parameter_being_adjusted << " is done:\n";

        double EngineTwoPoints = static_cast<double>(results.num_EngineTwo_wins) + 0.5 * static_cast<double>(results.num_draws);
        double EngineOnePoints = static_cast<double>(results.num_EngineOne_wins) + 0.5 * static_cast<double>(results.num_draws);

        mini_match_summary temp;
        temp.ratio_of_points = EngineTwoPoints / EngineOnePoints;
        temp.value_of_adjusted_parameter = EngineTwo::position::parameter_being_adjusted;

        results_for_mini_matches.push_back(temp);

        cout << "EngineTwo to EngineOne results ratio of their matchpoints: " << (EngineTwoPoints / EngineOnePoints) << "\n";

     /*   cout << "EngineOne won " << results.num_EngineOne_wins << " games.\n";
        cout << "EngineTwo won " << results.num_EngineTwo_wins << " games.\n";
        cout << "There were " << results.num_draws << " draws.\n";
        cout << "A total of " << num_trials << " starting positions were tested.\n";
        cout << "A total of " << (num_trials * 2) << " games were played.\n";

        cout << "\n";
        cout << results.num_starting_positions_where_EngineOne_wins_both << " starting positions where EngineOne won both.\n";
        cout << results.num_starting_positions_where_EngineOne_wins_and_draw << " starting positions where EngineOne won & drew.\n";
        cout << results.num_starting_positions_where_each_Engine_wins << " starting positions where each Engine won a game.\n";
        cout << results.num_starting_positions_where_two_draws << " starting positions where both games were drawn.\n";
        cout << results.num_starting_positions_where_EngineTwo_wins_and_draw << " starting positions where EngineTwo won & drew.\n";
        cout << results.num_starting_positions_where_EngineTwo_wins_both << " starting positions where EngineTwo won both.\n";  */

      /*  cout << "\nAll the positions that were played are now being written to a file, with both Engines' evaluations of them...\n";

        write_all_played_positions_to_file(starting_positions_already_used);

        cout << "...all done!\n"; */


        first_mini_match_done = true;
    } // End of big outer for loop

    cout << "\n\n\n";

    // Run through results for all mini-matches:

    for (const mini_match_summary& current: results_for_mini_matches)
    {
        cout << "For adjusted parameter = " << current.value_of_adjusted_parameter << ", the ratio of EngineTwo's points to ";
        cout << "EngineOne's points was " << current.ratio_of_points << "\n";
    }


    cout << "\nTo exit the simulation, enter a char: ";

    char ending_input;

    cin >> ending_input;
}

