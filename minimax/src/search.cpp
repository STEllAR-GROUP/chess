/*
 *  SEARCH.C
 */


#include "search.hpp"

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
    BOOL f;

    /* if we are a leaf node, return the value from the eval() function */
    if (!depth)
    {
      return eval(board, chosen_evaluator);
    }
    /* if this isn't the root of the search tree (where we have
       to pick a move and can't simply return 0) then check to
       see if the position is a repeat. if so, we can assume that
       this line is a draw and return 0. */
    if (board.ply && reps(board))
        return 0;

    std::vector<gen_t> workq;
    gen(workq, board); // Generate the moves
    f = FALSE; // Set this to false, if we have at least one legal move
               // we will set it to true (this is used for detecting 
               // stalemates and checkmates)

    max = -10000; // Set the max score to -infinity

    /* loop through the moves */
    while (workq.size() != 0) {
        gen_t g = workq.back(); // Get the move at the end
        workq.pop_back();       // Remove it from the work queue

        if (!makemove(board, g.m.b)) { // Make the move, if it isn't 
            continue;                  // legal, then go to the next one
        }
        
        f = TRUE;  // We have at least one legal move
        val = -search(board, depth - 1); // Recursively search this new board
                                         // position for its score
        takeback(board);  // Lets go back to our original board so we can
                          // make another move


        if (val > max)  // Is this value our maximum?
        {
          max = val;

          if (board.ply == 0) {  // If we are at the root level, need to set 
            move_to_make = g.m;    // the move to make as this one
          }
        }
    }

    /* no legal moves? then we're in checkmate or stalemate */
    if (!f) {
        if (in_check(board, board.side))
            return -10000 + board.ply;
        else
            return 0;
    }

    /* fifty move draw rule */
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


