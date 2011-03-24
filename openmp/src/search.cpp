/*
 *  search.cpp
 */

#include "search.hpp"
#include <pthread.h>
#include <algorithm>
#include <math.h>
#include <assert.h>

pthread_mutex_t mutex;

inline int min(int a,int b) { return a < b ? a : b; }
inline int max(int a,int b) { return a > b ? a : b; }

bucket_t hash_bucket[bucket_size];
std::vector<move> pv;  // Principle Variation, used in iterative deepening

inline int get_bucket_index(const node_t& board,int depth) {
    return ((board.hash >> M) ^ depth) & ((1<<N)-1);
}
inline int get_entry_index(const node_t& board,int depth) {
    return board.hash & ((1<<M)-1);
}

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
  for(int i=0;i<bucket_size;i++)
    hash_bucket[i].init();
  board.ply = 0;

  if (search_method == MINIMAX) {
    pthread_mutex_init(&mutex, NULL);
    pthread_t mini;
    minimax_t info(&board, depth[board.side]);
    pthread_create(&mini, NULL, search, &info);
    pthread_join(mini, NULL);
    //search(board, depth[board.side]);
  } else if (search_method == MTDF) {
    pv.resize(depth[board.side]);
    for(int i=0;i<pv.size();i++)
        pv[i].u = 0;
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
    //std::cout << "f=" << f << std::endl;
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

      if (i >= iter_depth)  // if our ply is greater than the iter_depth, then break
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


//int search(const node_t& board, int depth)
void* search(void* i)
{
  minimax_t* info = (minimax_t*)i;
  int depth = info->depth;
  node_t& board = *info->board;
  int val, max;

  // if we are a leaf node, return the value from the eval() function
  if (!depth)
  {
    evaluator ev;
    info->result = ev.eval(board, chosen_evaluator);
    return info;
  }
  /* if this isn't the root of the search tree (where we have
     to pick a move and can't simply return 0) then check to
     see if the position is a repeat. if so, we can assume that
     this line is a draw and return 0. */
  if (board.ply && reps(board))
    return info;

  std::vector<move> workq;
  std::vector<pthread_t *> children;
  std::vector<minimax_t *> children_info;
  std::vector<move> legalworkq;
  std::vector<move> max_moves; /* This is a vector of the moves that all have the same score and are the highest. 
                                  To be sorted by the hash value. */
  gen(workq, board); // Generate the moves

  max = -10000; // Set the max score to -infinity

  // loop through the moves
  node_t p_board;
  for (int i = 0; i < workq.size(); i++) {
    p_board = board;

    move g = workq[i];

    if (!makemove(p_board, g.b)) { // Make the move, if it isn't 
      continue;                    // legal, then go to the next one
    }

    pthread_t* child =  new pthread_t;
    minimax_t* child_info = new minimax_t(&p_board, depth - 1);
    pthread_create(child, NULL, search, child_info);
    children.push_back(child);
    legalworkq.push_back(g);
  }
  
  for(size_t i = 0; i < children.size() ; ++i)
  {
    minimax_t* child_info;
    pthread_join(*children[i], (void **)&child_info);
    children_info.push_back(child_info);
    delete children[i];
  }
  //assert(children_info.size() == legalworkq.size());
  for(size_t i = 0; i < children_info.size(); ++i)
  {
    move g = legalworkq[i];
    val = -children_info[i]->result;
    //printf("val=%d\n", val);
    //val = -search(p_board, depth - 1);

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

  // no legal moves? then we're in checkmate or stalemate
  if (max_moves.size() == 0) {
    if (in_check(board, board.side))
    {
      info->result = -10000 + board.ply;
      return info;
    }
    else
      return info;
  }

  if (board.ply == 0) {
    pthread_mutex_lock(&mutex);
    sort(max_moves.begin(), max_moves.end(), compare_moves);
    move_to_make = *(max_moves.begin());
    pthread_mutex_unlock(&mutex);
  }

  // fifty move draw rule
  if (board.fifty >= 100)
    return info;

  info->result = max;
  return info;
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
    int b_index = get_bucket_index(board,depth);
    hash_bucket[b_index].lock();
    zkey_t* z = hash_bucket[b_index].get(get_entry_index(board,depth));
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

  sort_pv(workq, board.ply); // Part of iterative deepening
  // loop through the moves
  for (int j = 0; j < workq.size(); j++) {
    if(alpha >= beta)
        continue; // use cut-off
    node_t p_board = board;
    move g = workq[j];

    if (!makemove(p_board, g.b)) { // Make the move, if it isn't 
      continue;                    // legal, then go to the next one
    }

    int val = -search_ab(p_board, depth-1, -beta, -alpha);

    if (val > alpha)
    {
      int b_index = get_bucket_index(board,depth);
      hash_bucket[b_index].lock();
      zkey_t* z = hash_bucket[b_index].get(get_entry_index(board,depth));

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
