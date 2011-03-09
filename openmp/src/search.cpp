/*
 *  search.cpp
 */


#include "search.hpp"
#include <algorithm>

// think() calls search() 

std::vector<move> pv;  // Principle Variation, used in iterative deepening


int think(node_t& board)
{
  board.ply = 0;

  if (search_method == MINIMAX) {
    search(board, depth[board.side]);
  } else if (search_method == ALPHABETA) {
    // Initially alpha is -infinity, beta is infinity
    pv.resize(depth[board.side]);
    int alpha = -10000;
    int beta = 10000;
    bool brk = false;  /* Indicates whether we broke away from iterative deepening 
                          and need to call search on the actual ply */

    for (int i = 1; i <= depth[board.side]; i++) // Iterative deepening
    {
      search_ab(board, i, alpha, beta);

      if (i >= iter_depth)
      {
        brk = true;
        break;
      }
    }

    if (brk)
      search_ab(board, depth[board.side], alpha, beta);
    pv.clear();
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

  std::vector<move> max_moves; /* This is a vector of the moves that all have the same score and are the highest. 
                                  To be sorted by the hash value. */
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
      continue;                    // legal, then go to the next one
    }

    val = -search(p_board, depth - 1); /* Recursively search this new board
                                          position for its score */


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
 */

int search_ab(const node_t& board, int depth, int alpha, int beta)
{
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
  int j;

  for (j = 0; j < workq.size(); j++) {
    node_t p_board = board;
    sort_pv(workq, board.ply); // Part of iterative deepening
    move g = workq[j];

    if (!makemove(p_board, g.b)) { // Make the move, if it isn't 
      continue;                    // legal, then go to the next one
    }

    int val = -search_ab(p_board, depth-1, -beta, -alpha);

    if (val > alpha)
    {
      alpha = val;
      if (board.ply == 0)
        move_to_make = g;
      pv[board.ply] = g;
    }

    if (beta <= alpha)
      return alpha; //beta cutoff
    break;
  }

  // BEGIN OpenMP

#ifdef OPENMP_SUPPORT
#pragma omp parallel for shared (workq,alpha,beta)
#endif
  for (int i = j; i < workq.size(); i++) {
    if(alpha >= beta)
        continue; // use cut-off
    node_t p_board = board;

    move g = workq[i];

    if (!makemove(p_board, g.b)) { // Make the move, if it isn't 
      continue;                    // legal, then go to the next one
    }

    int val = -search_ab(p_board, depth-1, -beta, -alpha);

    if (val > alpha)
    {
#ifdef OPENMP_SUPPORT
#pragma omp critical
#endif
      {
        alpha = val;
        if (board.ply == 0)
        {
          move_to_make = g;
        }
        pv[board.ply] = g;
      }
    }
  }
  return alpha;
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
      std::cerr << "Error, comparing similar moves" << std::endl;
    return (a.b.to) > (b.b.to);
  }
  else
    return (a.b.from) > (b.b.from);
}

void sort_pv(std::vector<move>& workq, int ply)
{
  for(int i = 0; i < workq.size() ; i++)
  {
    move temp;
    if (workq[i].u == pv[ply].u) /* If we have a move in the work queue that is the 
                                    same as the best move we have searched before */
    {
      temp = workq[0];
      workq[0] = workq[i];
      workq[i] = temp;
    }
  }
}
