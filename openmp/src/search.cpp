/*
 *  search.cpp
 */


#include "search.hpp"
#include <algorithm>

// think() calls search() 

int think(node_t& board)
{
    board.ply = 0;
    
    if (search_method == MINIMAX) {
        search(board, depth[board.side]);
    } else if (search_method == ALPHABETA) {
        // Initially alpha is -infinity, beta is infinity
        search_ab(board, depth[board.side], -10000, 10000, board.side);
    }
    
    return 1;
}


int search(const node_t& board, int depth)
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

    std::vector<move> workq;
    
    std::vector<move> max_moves; // This is a vector of the moves that all have the same score and are the highest. To be sorted by the hash value.
    gen(workq, board); // Generate the moves

    max = -10000; // Set the max score to -infinity

    // loop through the moves
    
#ifdef OPENMP_SUPPORT
#pragma omp parallel for shared (max, workq) private(val)
#endif
    for (int i = 0; i < workq.size(); i++) {
        node_t p_board = board;

        move g = workq[i];

        if (!makemove(p_board, g.b)) { // Make the move, if it isn't 
            continue;                  // legal, then go to the next one
        }

        val = -search(p_board, depth - 1); // Recursively search this new board
                                         // position for its score


#ifdef OPENMP_SUPPORT
#pragma omp critical
#endif
{
        if (val > max)  // Is this value our maximum?
        {
          max = val;
          
          max_moves.clear();
          max_moves.push_back(g);

        }
        else if (val == max)
        {
            max_moves.push_back(g);
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

/*
    Alpha Beta search function. Uses OpenMP parallelization by the 
    'Young Brothers Wait' algorithm which searches the eldest brother (i.e. move)
    serially to determine alpha-beta bounds and then searches the rest of the
    brothers in parallel.
    
    In order for this to be effective, move ordering is crucial. In theory,
    if the best move is ordered first, then it will produce a cutoff which leads
    to smaller search spaces which can be searched faster than standard minimax.
    However we don't know what a "good move" is until we have searched it, which
    is what iterative deepening is for.
    
    The algorithm maintains two values, alpha and beta, which represent the minimum 
    score that the maximizing player is assured of and the maximum score that the minimizing 
    player is assured of respectively. Initially alpha is negative infinity and beta is 
    positive infinity. As the recursion progresses the "window" becomes smaller. 
    When beta becomes less than alpha, it means that the current position cannot 
    be the result of best play by both players and hence need not be explored further.
    
    As a first cut, there is no move ordering (and no iterative deepening) 
    in this alpha-beta algorithm.
*/

int search_ab(const node_t& board, int depth, int alpha, int beta, int max_side)
{
    int val;

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
        
    // fifty move draw rule
    if (board.fifty >= 100)
        return 0;

    std::vector<move> workq;

    gen(workq, board); // Generate the moves

    // loop through the moves
    
    if (board.side == max_side)
    {
        for (int i = 0; i < workq.size(); i++) {
            node_t p_board = board;

            move g = workq[i];

            if (!makemove(p_board, g.b)) { // Make the move, if it isn't 
                continue;                  // legal, then go to the next one
            }
                
            val = search_ab(p_board, depth-1, alpha, beta, max_side);
            
            if (val > alpha)
            {
                alpha = val;
                if (board.ply == 0)
                    move_to_make = g;
            }
                
            if (beta <= alpha)
                break; //beta cutoff
        }
        return alpha;
    } else { // we are the minimizing side
        for (int i = 0; i < workq.size(); i++) {
            node_t p_board = board;

            move g = workq[i];

            if (!makemove(p_board, g.b)) { // Make the move, if it isn't 
                continue;                  // legal, then go to the next one
            }
                
            val = search_ab(p_board, depth-1, alpha, beta, max_side);
            
            if (val < beta)
                beta = val;
                
            if (beta <= alpha)
                break; //beta cutoff
        }
        return beta;
    }
}


/* reps() returns the number of times the current position
   has been repeated. It compares the current value of hash
   to previous values. */

int reps(const node_t& board)
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

