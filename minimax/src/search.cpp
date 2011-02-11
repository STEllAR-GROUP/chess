/*
 *  search.cpp
 */


#include "search.hpp"
#include <algorithm>

// think() calls search() 

int think(node_t& board)
{
    board.ply = 0;

    search(board, depth[board.side]);

    return 1;
}


int search(node_t board, int depth)
{
    int val, max;

    // if we are a leaf node, return the value from the eval() function
    if (!depth)
    {
      evaluator ev;
      return ev.eval(board, chosen_evaluator);
    }
    /* if this isn't the root of the search tree (where we have
       to pick a move and can't simply return 0) then check to
       see if the position is a repeat. if so, we can assume that
       this line is a draw and return 0. */
    if (board.ply && reps(board))
        return 0;

    std::vector<gen_t> workq;
    
    std::vector<move> max_moves; // This is a vector of the moves that all have the same score and are the highest. To be sorted by the hash value.
    gen(workq, board); // Generate the moves

    max = -10000; // Set the max score to -infinity

    // loop through the moves
    
#pragma omp parallel for shared (max, workq) private(val)
    for (int i = 0; i < workq.size(); i++) {
        node_t p_board = board;

        gen_t g = workq[i];

        if (!makemove(p_board, g.m.b)) { // Make the move, if it isn't 
            continue;                  // legal, then go to the next one
        }

        val = -search(p_board, depth - 1); // Recursively search this new board
                                         // position for its score
        takeback(p_board);  // Lets go back to our original board so we can
                          // make another move


#pragma omp critical
{
        if (val > max)  // Is this value our maximum?
        {
          max = val;
          
          max_moves.clear();
          max_moves.push_back(g.m);

          /*if (p_board.ply == 0) {  // If we are at the root level, need to set 
            move_to_make = g.m;    // the move to make as this one
          }*/
        }
        else if (val == max)
        {
            max_moves.push_back(g.m);
        }
}
    }

    // no legal moves? then we're in checkmate or stalemate
    if (max_moves.size() == 0) {
        if (in_check(board, board.side))
            return -10000 + board.ply;
        else
            return 0;
    }
    
    if (board.ply == 0) {
        sort(max_moves.begin(), max_moves.end(), compare_moves);
        move_to_make = *(max_moves.begin());
    }

    // fifty move draw rule
    if (board.fifty >= 100)
        return 0;

    return max;
}



/* reps() returns the number of times the current position
   has been repeated. It compares the current value of hash
   to previous values. */

int reps(node_t& board)
{
    int i;
    int r = 0;

    for (i = board.hply - board.fifty; i < board.hply; ++i)
        if (board.hist_dat[i].hash == board.hash)
            ++r;
    return r;
}

bool compare_moves(move a, move b)
{
    if (a.b.from == b.b.from) 
    {
        if (a.b.to == b.b.to)
          std::cerr << "Error, comparing similar moves";
        return (a.b.to) > (b.b.to);
    }
    else
        return (a.b.from) > (b.b.from);
}

