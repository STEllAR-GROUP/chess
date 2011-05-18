/*
 *  search.cpp
 */

#include "search.hpp"
#include <pthread.h>
#include <algorithm>
#include <math.h>
#include <assert.h>

worker workers[bucket_size];

// Maximum number of pthreads we can create
// Even on a two core machine we want 40 or 50
// here to get the factor 2 speedup.
const int max_pcount = 64;

// Depth at which to spawn pthreads
int para_depth;

// Count of how many more threads we can create
// when it reaches zero, we have to stop making them
int pcount = max_pcount;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

inline int min(int a,int b) { return a < b ? a : b; }
inline int max(int a,int b) { return a > b ? a : b; }

bucket_t hash_bucket[bucket_size];
std::vector<move> pv;  // Principle Variation, used in iterative deepening

/**
 * This determines whether we have a capture. It's not sophisticated
 * enough to do en passant.
 */
bool capture(const node_t& board,move& g) {
  return board.color[g.b.to] != EMPTY
      && board.color[g.b.to] != board.color[g.b.from];
}

/**
 * Quiescent evaluator. Originally we wished to avoid the complexity
 * of quiescent move searches, but MTD-f does not seem to work properly
 * without it. This particular search evaluates each move using the
 * evalutaor, unless the move is a capture then it calls itself
 * recursively. The quiescent move search uses alpha-beta to speed
 * itself along, and evaluates non-captures first to get the greatest
 * cutoff.
 **/
score_t qeval(const node_t& board,const score_t& lower,const score_t& upper)
{
    evaluator ev;
    DECL_SCORE(s,ev.eval(board, chosen_evaluator),board.hash);
    s = max(lower,s);
    std::vector<move> workq;
    gen(workq, board); // Generate the moves
    for(int j=0;j < workq.size(); j++) {
        move g = workq[j];
        node_t p_board = board;
        if(!makemove(p_board,g.b))
            continue;
        if(capture(board,g)) {
            //s = max(-qeval(p_board,-upper,-lower),s);
        } else {
            DECL_SCORE(v,ev.eval(p_board,chosen_evaluator),p_board.hash);
            s = max(v,s);
        }
        if(s > upper)
            return s;
    }
    for(int j=0;j < workq.size(); j++) {
        move g = workq[j];
        node_t p_board = board;
        if(!makemove(p_board,g.b))
            continue;
        if(capture(board,g)) {
            s = max(-qeval(p_board,-upper,-lower),s);
        } else {
            //DECL_SCORE(v,ev.eval(p_board,chosen_evaluator),p_board.hash);
            //s = max(v,s);
        }
        if(s > upper)
            return s;
    }
    return s;
}

// think() calls a search function 
int think(node_t& board)
{
  for(int i=0;i<bucket_size;i++)
    hash_bucket[i].init();
  board.ply = 0;
  //para_depth = depth[board.side]-1;
  para_depth = -1;

  if (search_method == MINIMAX) {
    score_t f = search(board, depth[board.side]);
    if (bench_mode)
      std::cout << "SCORE=" << f << std::endl;
  } else if (search_method == MTDF) {
    pv.resize(depth[board.side]);
    for(int i=0;i<pv.size();i++)
        pv[i].u = 0;
    DECL_SCORE(alpha,-10000,board.hash);
    DECL_SCORE(beta,10000,board.hash);
    int stepsize = 2;
    int d = depth[board.side] % stepsize;
    if(d == 0)
        d = stepsize;
    score_t f(search_ab(board,d,alpha,beta));
    while(d < depth[board.side]) {
        d+=stepsize;
        f = mtdf(board,f,d);
    }
    if (bench_mode)
      std::cout << "SCORE=" << f << std::endl;
  } else if (search_method == ALPHABETA) {
    // Initially alpha is -infinity, beta is infinity
    pv.resize(depth[board.side]);
    score_t f;
    DECL_SCORE(alpha,-10000,board.hash);
    DECL_SCORE(beta,10000,board.hash);
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
    if (bench_mode)
      std::cout << "SCORE=" << f << std::endl;
  }

  return 1;
}

#if 0
#pragma mark -
#pragma mark Search functions
#endif



/** MTD-f */
score_t mtdf(const node_t& board,score_t f,int depth)
{
    score_t g = f;
    DECL_SCORE(upper,10000,board.hash);
    DECL_SCORE(lower,-10000,board.hash);
    // Technically, MTD-f uses "zero-width" searches
    // However, Aske Plaat points out that it performs
    // better with a coarser evaluation function. Since
    // this maps readily onto a wider, non-zero width
    // we provide a width setting for optimization.
    const int start_width = 3;
    // Sometimes MTD-f gets stuck and can try many
    // times without finding an answer. If this happens
    // we want to set a threshold for bailing out.
    const int max_tries = 5;
    // If our first guess isn't right, chances are
    // we want to search a little wider the next try
    // to improve our odds.
    const int grow_width = 2;
    int width = start_width;
    const int max_width = start_width+grow_width*max_tries;
    score_t alpha = lower, beta = upper;
    while(lower < upper) {
        if(width >= max_width) {
            g = search_ab(board,depth,lower,upper);
            break;
        } else {
            alpha = max(lower,ADD_SCORE(g,    -(1+width/2)));
            beta  = min(upper,ADD_SCORE(alpha, (1+width)));
        }
        g = search_ab(board,depth,alpha,beta);
        if(g < beta) {
            if(g > alpha)
                break;
            upper = g;
        } else {
            lower = g;
        }
        width += grow_width;
    }
    return g;
}

void *search_pt(void *vptr) {
    search_info *info = (search_info *)vptr;
    info->result = search(info->board,info->depth);
    return NULL;
}

score_t search(const node_t& board, int depth)
{
  score_t val, max;

  // if we are a leaf node, return the value from the eval() function
  if (!depth)
  {
    evaluator ev;
    DECL_SCORE(s,ev.eval(board, chosen_evaluator),board.hash);
    return s;
  }
  /* if this isn't the root of the search tree (where we have
     to pick a move and can't simply return 0) then check to
     see if the position is a repeat. if so, we can assume that
     this line is a draw and return 0. */
  if (board.ply && reps(board)) {
    DECL_SCORE(s,0,board.hash);
    return s;
  }

  std::vector<move> workq;
  move max_move;
  max_move.u = INVALID_MOVE;

  gen(workq, board); // Generate the moves

  DECL_SCORE(minf,-10000,board.hash);
  max = minf; // Set the max score to -infinity

  const int worksq = workq.size();
  smart_ptr<task> tasks[worksq];

  int j=0;
  for(;depth == para_depth && j < workq.size(); j++) {
    move g = workq[j];
    smart_ptr<search_info> info = new search_info(board);

    if (!makemove(info->board, g.b)) { // Make the move, if it isn't 
	  								   // legal, then go to the next one
      continue;
    }
	tasks[j] = new task;

	tasks[j]->info = info;

    DECL_SCORE(z,0,board.hash);
    info->depth = depth-1;
    info->result = z;
	tasks[j]->pfunc = search_pt;
	workers[get_bucket_index(info->board,info->depth)].add(tasks[j]);
  }

  // loop through the moves
  // We do this twice. The first time we skip
  // quiescent searches, the second time we
  // do the quiescent search. By doing this
  // we get the best value of beta to produce
  // cutoffs within the quiescent search routine.
  // Without doing this, minimax runs extremely
  // slowly.
  for (int i = 0; i < workq.size(); i++) {
    move g = workq[i];
    if(i < j) {
        assert(false);
		  if(!tasks[i].valid())
			  continue;
      smart_ptr<search_info> info = tasks[i]->info;
		  tasks[i]->join();
      val = -tasks[i]->info->result;
    } else {
        node_t p_board = board;

        if (!makemove(p_board, g.b)) { // Make the move, if it isn't 
            continue;                    // legal, then go to the next one
        }

        if(depth == 1 && capture(board,g)) {
            //DECL_SCORE(lower,-10000,board.hash);
            //val = -qeval(p_board,lower,-max); 
            continue;
        } else {
            val = -search(p_board, depth - 1); 
        }
    }

    if (val > max)  // Is this value our maximum?
    {
      max = val;

      max_move = g;
    }
  }
  for (int i = 0; i < workq.size(); i++) {
    move g = workq[i];
    if(i < j) {
        assert(false);
		  if(!tasks[i].valid())
			  continue;
      smart_ptr<search_info> info = tasks[i]->info;
		  tasks[i]->join();
      val = -tasks[i]->info->result;
    } else {
        node_t p_board = board;

        if (!makemove(p_board, g.b)) { // Make the move, if it isn't 
            continue;                    // legal, then go to the next one
        }

        if(depth == 1 && capture(board,g)) {
            DECL_SCORE(lower,-10000,board.hash);
            val = -qeval(p_board,lower,-max); 
        } else {
            //val = -search(p_board, depth - 1); 
            continue;
        }
    }

    if (val > max)  // Is this value our maximum?
    {
      max = val;

      max_move = g;
    }
  }

  // no legal moves? then we're in checkmate or stalemate
  if (max_move.u == INVALID_MOVE) {
    if (in_check(board, board.side))
    {
      DECL_SCORE(s,-10000 + board.ply,board.hash);
      return s;
    }
    else
    {
      DECL_SCORE(z,0,board.hash);
      return z;
    }
  }

  if (board.ply == 0) {
    pthread_mutex_lock(&mutex);
    move_to_make = max_move;
    pthread_mutex_unlock(&mutex);
  }

  // fifty move draw rule
  if (board.fifty >= 100) {
    DECL_SCORE(z,0,board.hash);
    return z;
  }

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
 
void *search_ab_pt(void *vptr)
{
  search_info *info = (search_info *)vptr;
  info->result = search_ab(info->board,info->depth, info->alpha, info->beta);
  return NULL;
}

score_t search_ab(const node_t& board, int depth, score_t alpha, score_t beta)
{
  // if we are a leaf node, return the value from the eval() function
  if (!depth)
  {
    evaluator ev;
    DECL_SCORE(s,ev.eval(board, chosen_evaluator),board.hash);
    return s;
  }
  /* if this isn't the root of the search tree (where we have
     to pick a move and can't simply return 0) then check to
     see if the position is a repeat. if so, we can assume that
     this line is a draw and return 0. */
  if (board.ply && reps(board)) {
    DECL_SCORE(z,0,board.hash);
    return z;
  }

  // fifty move draw rule
  if (board.fifty >= 100) {
    DECL_SCORE(z,0,board.hash);
    return z;
  }

  /*
  if(depth >= zdepth) {
    int b_index = get_bucket_index(board,depth);
    hash_bucket[b_index].lock();
    zkey_t* z = hash_bucket[b_index].get(get_entry_index(board,depth));
    if ((z->hash == board.hash)&&(z->depth == depth)) {
        if(z->score >= beta)
        {
          score_t zscore = z->score;
          hash_bucket[b_index].unlock();
          return zscore;
        }
        alpha = max(alpha,z->score);
    }
    hash_bucket[b_index].unlock();
  }
  */

  std::vector<move> workq;
  move max_move;
  max_move.u = INVALID_MOVE; 

  gen(workq, board); // Generate the moves

  sort_pv(workq, board.ply); // Part of iterative deepening
  
  const int worksq = workq.size();
  smart_ptr<task> tasks[worksq];
  
  int j=0;
  score_t val;
  
  /**
   * This loop will execute once and it will do it
   * sequentially. This should produce good cut-offs.
   * It is a 'Younger Brothers Wait' strategy.
   **/
  for(;j < worksq;j++) {
    if(alpha > beta)
      continue;
    move g = workq[j];
    node_t p_board = board;

    if (!makemove(p_board, g.b)) { // Make the move, if it isn't 
      continue;                    // legal, then go to the next one
    }

    if(depth == 1 && capture(board,g)) {
        val = -qeval(p_board, -beta, -alpha);
    } else
        val = -search_ab(p_board, depth-1, -beta, -alpha);

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
      pv[board.ply] = g;
      max_move = g;
    }
    j++;
    break;
  }
  
  // loop through the moves
  for (; depth == para_depth && j < worksq; j++) {
    move g = workq[j];
    smart_ptr<search_info> info = new search_info(board);
    
    if (!makemove(info->board, g.b))
      continue;
    
    tasks[j] = new task;
    
    tasks[j]->info = info;
    info->depth = depth-1;
    info->alpha = -beta;
    info->beta = -alpha;
    DECL_SCORE(z,0,board.hash);
    info->result = z;
    tasks[j]->pfunc = search_ab_pt;
    workers[get_bucket_index(info->board, info->depth)].add(tasks[j]);
  }
  
  for (int i = 0; i < worksq; i++) {  
    if(alpha >= beta)
      break;
    move g = workq[i];
    if (i < j) {
      if (!tasks[i].valid())
        continue;
      smart_ptr<search_info> info = tasks[i]->info;
      tasks[i]->join();
      val = -tasks[i]->info->result;
    } else {
      node_t p_board = board;

      if (!makemove(p_board, g.b)) { // Make the move, if it isn't 
        continue;                    // legal, then go to the next one
      }

      if(depth == 1 && capture(board,g)) {
        val = -qeval(p_board, -beta, -alpha);
      } else {
        val = -search_ab(p_board, depth-1, -beta, -alpha);
      }
    }

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
      pv[board.ply] = g;
      max_move = g;
    }
  }

  /**
   * If we're doing mtd-f, it's possible that all
   * the moves were cut off. Thus we need to check
   * for INVALID_MOVE
   **/
  if (board.ply == 0 && max_move.u != INVALID_MOVE) {
    pthread_mutex_lock(&mutex);
    move_to_make = max_move;
    pthread_mutex_unlock(&mutex);
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
    if (board.hist_dat[i] == board.hash)
      ++r;
  return r;
}

/*
bool compare_moves(move a, move b)
{
  if (a.u == INVALID_MOVE) false;
  if (b.u == INVALID_MOVE) true;
  if (a.b.from == b.b.from) 
  {
    if (a.b.to == b.b.to)
      std::cerr << "Error, comparing similar moves" << std::endl;
    return (a.b.to) > (b.b.to);
  }
  else
    return (a.b.from) > (b.b.from);
}
*/

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

void *run_worker(void *vptr) {
	worker *w = (worker *)vptr;
	while(true) {
		smart_ptr<task> pt = w->remove();
		(*pt->pfunc)(pt->info.ptr());
		pt->finish();
	}
	return 0;
}
