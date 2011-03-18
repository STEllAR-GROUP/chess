/*
 *  search.cpp
 */

#include "search.hpp"
#include <algorithm>
#include <math.h>
#include <assert.h>

inline int min(int a,int b) { return a < b ? a : b; }
inline int max(int a,int b) { return a > b ? a : b; }

const int zdepth = 2;
const int N = 8;  // bits for bucket size
const int M = 8;  // bits for table size N+M is total bits that can be indexed by transposition table
const int bucket_size = pow(2, N);
const int table_size = pow(2, M);

std::vector<bucket_t> hash_bucket;
std::vector<move> pv;  // Principle Variation, used in iterative deepening

/** MTD-f */
int mtdf(const node_t& board,int f,int depth)
{
    int g = f;
    int upper =  10000;
    int lower = -10000;
    // Technically, MTD-f uses "zero-width" searches
    // However, Aske Plaat points out that it performs
    // better with a coarser evaluation function. Since
    // this maps readily onto a wider, non-zero width
    // we provide a width setting for optimization.
    int width = 5;
    while(lower < upper) {
        int alpha = max(lower,g-1-width/2);
        int beta = alpha+width+1;
        g = search_ab(board,depth,alpha,beta);
        if(g < beta) {
            if(g > alpha)
                break;
            upper = g;
        } else {
            lower = g;
        }
    }
    return g;
}

// think() calls search() 
int think(node_t& board)
{
  hash_bucket = std::vector<bucket_t>(bucket_size, table_size);
  board.ply = 0;

  if (search_method == MINIMAX) {
    search(board, depth[board.side]);
  } else if (search_method == MTDF) {
    pv.resize(depth[board.side]);
    int alpha = -10000;
    int beta = 10000;
    int stepsize = 2;
    int d = depth[board.side] % stepsize;
    if(d == 0)
        d = stepsize;
    int f = search_ab(board,d,alpha,beta);
    while(d <= depth[board.side]) {
        f = mtdf(board,f,d);
        d+=stepsize;
    }
    std::cout << "f=" << f << std::endl;
  } else if (search_method == ALPHABETA) {
    // Initially alpha is -infinity, beta is infinity
    pv.resize(depth[board.side]);
    int f;
    int alpha = -10000;
    int beta = 10000;
    bool brk = false;  /* Indicates whether we broke away from iterative deepening 
                          and need to call search on the actual ply */

    int low = 2;
    if(depth[board.side] % 2 == 1)
        low = 1;
    for (int i = low; i <= depth[board.side]; i++) // Iterative deepening
    {
      f = search_ab(board, i, alpha, beta);

      if (i >= iter_depth)
      {
        brk = true;
        break;
      }
    }

    if (brk)
      f=search_ab(board, depth[board.side], alpha, beta);
    pv.clear();
    //std::cout << "f=" << f << std::endl;
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

  if(depth >= zdepth) {
    int b_index = board.hash & ((N>>M)&((1<<N)-1)); // Some bit magic to get the bucket index
    hash_bucket[b_index].lock();
    zkey_t* z = hash_bucket[b_index].get(board.hash & (1<<M - 1));
    if ((z->hash == board.hash)&&(z->depth == depth)) {
        if(z->score >= beta)
        {
          int zscore = z->score;
          hash_bucket[b_index].unlock();
          return zscore;
        }
        alpha = max(alpha,z->score);
    }
    hash_bucket[b_index].unlock();
  }

  std::vector<move> workq;

  gen(workq, board); // Generate the moves


  // loop through the moves
  int j;

  sort_pv(workq, board.ply); // Part of iterative deepening
  for (j = 0; j < workq.size(); j++) {
    node_t p_board = board;
    move g = workq[j];

    if (!makemove(p_board, g.b)) { // Make the move, if it isn't 
      continue;                    // legal, then go to the next one
    }

    int val = -search_ab(p_board, depth-1, -beta, -alpha);

    if (val > alpha)
    {
      
      int b_index = board.hash & ((N>>M)&((1<<N)-1)); // Some bit magic to get the bucket index
      hash_bucket[b_index].lock();
      zkey_t* z = hash_bucket[b_index].get(board.hash & (1<<M - 1));
      z->hash = board.hash;
      z->score = val;
      z->depth = depth;
      hash_bucket[b_index].unlock();
      
      alpha = val;
      if (board.ply == 0)
        move_to_make = g;
      pv[board.ply] = g;
    }

    if (beta <= alpha) {
      return alpha; //beta cutoff
    }
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
        int b_index = board.hash & ((N>>M)&((1<<N)-1)); // Some bit magic to get the bucket index
        hash_bucket[b_index].lock();
        zkey_t* z = hash_bucket[b_index].get(board.hash & (1<<M - 1));
        z->hash = board.hash;
        z->score = val;
        z->depth = depth;
        hash_bucket[b_index].unlock();
        
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
      break;
    }
  }
}
