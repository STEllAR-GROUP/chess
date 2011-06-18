/*
 *  search.cpp
 */

#include "parallel_support.hpp"
#include "search.hpp"
#include <pthread.h>
#include <algorithm>
#include <math.h>
#include <assert.h>
#include "mpi_support.hpp"
#include "here.hpp"

worker workers[bucket_size];
void capture_last(const node_t& board,std::vector<move>& workq);
score_t multistrike(const node_t& board,score_t f,int depth);

// Maximum number of pthreads we can create
// Even on a two core machine we want 40 or 50
// here to get the factor 2 speedup.
const int max_pcount = 64;

// Depth at which to spawn pthreads
int para_depth_lo, para_depth_hi;

// Count of how many more threads we can create
// when it reaches zero, we have to stop making them
int pcount = max_pcount;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
bool chx_abort = false;
bool multistrike_on = false;

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
    if(chx_abort)
        return s;
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
 
void *qeval_pt(void *vptr)
{
  search_info *info = (search_info *)vptr;
  info->result = qeval(info->board,info->alpha, info->beta);
  return NULL;
}

smart_ptr<task> parallel_task(bool parx) {
    if(mpi_rank != 0){// || !parx) {
        smart_ptr<task> t = new task;
        return t;
    }
    bool par = mpi_task_array[0].dec();
    if(par) {
        smart_ptr<task> t = new pthread_task;
        return t;
    } else {
        for(int i=1;i<mpi_size;i++) {
            if(mpi_task_array[i].dec()) {
                smart_ptr<task> t = new mpi_task(i);
                return t;
            }
        }
    }
    smart_ptr<task> t = new task;
    return t;
}

void *strike(void *vptr) {
  search_info *info = (search_info *)vptr;
  info->result = search_ab(info->board,info->depth,info->alpha,info->beta);
  if(info->alpha < info->result && info->result < info->beta) {
    std::cout << "FOUND: " << VAR(info->result) << std::endl;
    chx_abort = true;
  }
  return NULL;
}

// think() calls a search function 
int think(node_t& board,bool parallel)
{
  multistrike_on = false;
  chx_abort = false;
  for(int i=0;i<bucket_size;i++)
    hash_bucket[i].init();
  board.ply = 0;
  if(parallel) {
    para_depth_lo = 3;
    para_depth_hi = 5;
  } else {
    para_depth_lo = -1;
    para_depth_hi = -1;
  }

  if (search_method == MINIMAX) {
    score_t f = search(board, depth[board.side]);
    if (bench_mode)
      std::cout << "SCORE=" << f << std::endl;
  } else if (search_method == MTDF) {
    //pv.resize(depth[board.side]);
    for(int i=0;i<pv.size();i++)
        pv[i].u = 0;
    DECL_SCORE(alpha,-10000,board.hash);
    DECL_SCORE(beta,10000,board.hash);
    int stepsize = 2;
    int d = depth[board.side] % stepsize;
    if(d == 0)
        d = stepsize;
    board.depth = d;
    score_t f(search_ab(board,d,alpha,beta));
    while(d < depth[board.side]) {
        d+=stepsize;
        board.depth = d;
        f = mtdf(board,f,d);
    }
    if (bench_mode)
      std::cout << "SCORE=" << f << std::endl;
  } else if (search_method == MULTISTRIKE) {
    multistrike_on = true;
    //para_depth_lo = para_depth_hi = -1;
    //pv.resize(depth[board.side]);
    for(int i=0;i<pv.size();i++)
        pv[i].u = 0;
    /*
    DECL_SCORE(alpha,-10000,board.hash);
    DECL_SCORE(beta,10000,board.hash);
    int stepsize = 2;
    int d = depth[board.side] % stepsize;
    if(d == 0)
        d = stepsize;
    score_t f(search_ab(board,d,alpha,beta));
    while(d < depth[board.side]) {
        chx_abort = false;
        d+=stepsize;
        f = multistrike(board,f,d);
    }
    */
    score_t f = multistrike(board,0,depth[board.side]);
    if (bench_mode)
      std::cout << "SCORE=" << f << std::endl;
  } else if (search_method == ALPHABETA) {
    // Initially alpha is -infinity, beta is infinity
    //pv.resize(depth[board.side]);
    for(int i=0;i<pv.size();i++)
        pv[i].u = 0;
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

inline int figx(int n) {
    if(n < 0) return -figx(-n);
    if(n <= 3) return 3*n;
    if(n <= 5) return 5*(n-3)+figx(3);
    if(n <= 10) return 8*(n-5)+figx(5);
    return 10*(n-10)+figx(10);
}

score_t multistrike(const node_t& board,score_t f,int depth)
{
    score_t ret=0;
    const int max_parallel = 5;
    const int fac = 600/max_parallel;//10*25/max_parallel;
    std::vector<smart_ptr<task> > tasks;
    DECL_SCORE(lower,-10000,0);
    DECL_SCORE(upper,10000,0);
    //DECL_SCORE(lbound,0,0);
    //DECL_SCORE(ubound,0,0);
    for(int i=-max_parallel;i<=max_parallel;i++) {
        //std::cout << "figx(i=" << i << ")=" << figx(i) << std::endl;
        DECL_SCORE(alpha,i==-max_parallel ? -10000 : fac*(i)  ,0);
        DECL_SCORE(beta, i== max_parallel ?  10000 : fac*(i+1),0);
        //std::cout << "i=" << i << " (" << alpha << "," << beta << ")" << std::endl;
        //DECL_SCORE(alpha,fac*(i)  ,0);
        //DECL_SCORE(beta, fac*(i+1),0);
        //lbound=min(lbound,alpha);
        //ubound=max(ubound,beta);
        beta++;
        smart_ptr<task> t = parallel_task(board.hply==0);
        t->info = new search_info;
        t->info->alpha = alpha;
        t->info->beta = beta;
        t->info->board = board;
        t->info->depth = depth;
        t->pfunc = strike_f;
        t->start();
        tasks.push_back(t);
    }
    for(int i=0;i<tasks.size();i++) {
        tasks[i]->join();
        score_t result = tasks[i]->info->result;
        score_t alpha = tasks[i]->info->alpha;
        score_t beta = tasks[i]->info->beta;
        if(alpha < result && result < beta) {
            ret = result;
            std::cout << VAR(alpha) << VAR(beta) << "ms::SCORE=" << result << std::endl;
        }
    }
    /*
    DECL_SCORE(lbound,  -max_parallel *fac,0);
    DECL_SCORE(ubound,(1+max_parallel)*fac,0);
    bool done = false;
    while(!done) {
        done=true;
        for(int i=0;i<tasks.size();i++) {
            if(((pthread_task*)tasks[i].ptr())->joined)
                continue;
            done=false;
            tasks[i]->join();
            //std::cout << VAR(tc) << VAR(lower) << VAR(lbound) << VAR(ubound) << VAR(upper) << std::endl;
            score_t result = tasks[i]->info->result;
            score_t alpha = tasks[i]->info->alpha;
            score_t beta = tasks[i]->info->beta;
            if(alpha < result && result < beta) {
                ret = result;
                std::cout << "ms::SCORE=" << result << std::endl;
            } else if(result < beta) {
                upper = min(upper,result);
                if(upper < ubound && upper > lbound)
                    upper = lbound;
                if(!chx_abort && lower < upper) {
                    tasks[i]->info->board = board;
                    tasks[i]->info->depth = depth;

                    tasks[i]->info->beta = upper;
                    upper = ADD_SCORE(upper,-fac);
                    tasks[i]->info->alpha = upper;

                    tasks[i]->info->beta++;
                    //std::cout << VAR(upper) << VAR(result) << VAR(alpha) << VAR(beta) << std::endl;
                    tasks[i]->start();
                }
            } else if(result > alpha) {
                lower = max(lower,result);
                if(lower > lbound && lower < ubound)
                    lower = ubound;
                if(!chx_abort && lower < upper) {
                    tasks[i]->info->depth = depth;
                    tasks[i]->info->board = board;

                    tasks[i]->info->alpha = lower;
                    lower = ADD_SCORE(lower,3L);
                    tasks[i]->info->beta = lower;

                    tasks[i]->info->beta++;
                    tasks[i]->start();
                }
            }
        }
    }
    */
    return ret;
}


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
    const int start_width = 8;
    // Sometimes MTD-f gets stuck and can try many
    // times without finding an answer. If this happens
    // we want to set a threshold for bailing out.
    const int max_tries = 4;
    // If our first guess isn't right, chances are
    // we want to search a little wider the next try
    // to improve our odds.
    const int grow_width = 4;
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
    std::vector<smart_ptr<task> > tasks;
    bool para = true;//(para_depth_lo == depth)||(para_depth_hi == depth);

    // loop through the moves
    // We do this twice. The first time we skip
    // quiescent searches, the second time we
    // do the quiescent search. By doing this
    // we get the best value of beta to produce
    // cutoffs within the quiescent search routine.
    // Without doing this, minimax runs extremely
    // slowly.
    for(int mm=0;mm<2;mm++) {
        for(int j=0;j < workq.size(); j++) {
            bool last = (j+1)==workq.size();
            move g = workq[j];
            smart_ptr<search_info> info = new search_info(board);

            if (makemove(info->board, g.b)) { // Make the move, if it isn't 
                DECL_SCORE(z,0,board.hash);
                info->depth = depth-1;
                info->mv.u = g.u;
                info->result = z;
                if(depth == 1 && capture(board,g)) {
                    if(mm==1) {
                        smart_ptr<task> t = 
                            para ? parallel_task(info->board.hply==0) : new task;
                        t->info = info;
                        DECL_SCORE(lo,-10000,0);
                        info->beta = -max;
                        info->alpha = lo;
                        t->pfunc = qeval_f;
                        tasks.push_back(t);
                        t->start();
                    }
                } else {
                    if(mm==0) {
                        smart_ptr<task> t = 
                            para ? parallel_task(info->board.hply == 0) : new task;
                        t->info = info;
                        t->pfunc = search_f;
                        tasks.push_back(t);
                        t->start();
                    }
                }
            }
            if(!para||tasks.size()>=256||last) {
                for(int n=0;n<tasks.size();n++) {
                    smart_ptr<search_info> info = tasks[n]->info;
                    tasks[n]->join();
                    val = -tasks[n]->info->result;

                    if (val > max)
                    {
                        max = val;
                        max_move = info->mv;
                    }
                }
                tasks.clear();
            }
        }
    }
    assert(tasks.size()==0);

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
        assert(max_move.u != INVALID_MOVE);
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
  assert(info->depth == info->board.depth);
  info->result = search_ab(info->board,info->depth, info->alpha, info->beta);
  return NULL;
}

score_t search_ab(const node_t& board, int depth, score_t alpha, score_t beta)
{
  assert(depth >= 0);
  // if we are a leaf node, return the value from the eval() function
  if (depth == 0)
  {
    evaluator ev;
    DECL_SCORE(s,ev.eval(board, chosen_evaluator),board.hash);
    return s;
  }
  assert(depth >= 1);
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
  if(mpi_rank==0 && depth >= zdepth) {
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

  /*
  if(multistrike_on) {
    std::random_shuffle(workq.begin(),workq.end());
    capture_last(board,workq);
  }
  */
  //if(!multistrike_on)
  sort_pv(workq, board.ply); // Part of iterative deepening
  
  const int worksq = workq.size();
  std::vector<smart_ptr<task> > tasks;
  
  int j=0;
  score_t val;
  bool para = (para_depth_lo == depth)||(para_depth_hi == depth);
  
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

    assert(depth >= 1);
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
      if(board.ply < pv.size())
        pv[board.ply] = g;
      max_move = g;
    }
    j++;
    break;
  }
  
  // loop through the moves
  bool last = false;
  for (; j < worksq; j++) {
      assert(!last);
      last = (j+1==worksq);
      move g = workq[j];
      smart_ptr<search_info> info = new search_info(board);
      if(chx_abort) {
        DECL_SCORE(v,-11000,0);
        return v;
      }

      if (makemove(info->board, g.b)) {

          smart_ptr<task> t = para ? parallel_task(info->board.hply == 0) : new task;

          t->info = info;
          info->board.depth = info->depth = depth-1;
          assert(depth >= 0 && depth < 7);
          info->alpha = -beta;
          info->beta = -alpha;
          info->result = -beta;
          info->mv = g;
          if(depth == 1 && capture(board,g))
              t->pfunc = qeval_f;
          else
              t->pfunc = search_ab_f;
          t->start();
          tasks.push_back(t);
      }
      if(!para || tasks.size() >= 3 || last) {
          for(int n=0;n<tasks.size();n++) {
              smart_ptr<search_info> info = tasks[n]->info;
              tasks[n]->join();
              val = -tasks[n]->info->result;

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
                  if(board.ply < pv.size())
                    pv[board.ply] = info->mv;
                  max_move = info->mv;
              }
              if(alpha >= beta)
                break;
          }
          tasks.clear();
          if(alpha >= beta)
              break;
      }
  }
  assert(tasks.size()==0);

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

  for (i = board.hply - board.fifty; i < board.hply; ++i) {
    assert(i < board.hist_dat.size());
    assert(board.hash != 0);
    if (board.hist_dat[i] == board.hash)
      ++r;
  }
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

void capture_last(const node_t& board,std::vector<move>& workq)
{
    int ilo = 0, ihi = workq.size()-1;
    while(true) {
        while(!capture(board,workq[ilo]))
            ilo++;
        while(capture(board,workq[ihi]))
            ihi--;
        if(ilo < ihi) {
            move tmp = workq[ilo];
            workq[ilo] = workq[ihi];
            workq[ihi] = tmp;
        } else {
            break;
        }
    }
}

/*
void *mpi_job(void *v) {
    search_info *s = (search_info)*v;

    delete s;
}
*/

//std::map<hash_t,std::map<int,worker_result> > results;
worker_result results[worker_result_size];

int result_alloc(hash_t h,int d) {
    int n = abs(h^d)%worker_result_size;
    for(int i=0;i<worker_result_size;i++) {
        int q = (i+n)%worker_result_size;
        pthread_mutex_lock(&results[q].mut);
        bool done = false;
        if(results[q].depth == -1) {
            results[q].depth = d;
            results[q].hash = h;
            results[q].has_result = false;
            results[q].score = 0;
            done = true;
        }
        pthread_mutex_unlock(&results[q].mut);
        if(done)
            return q;
    }
    assert(false);
    return -1;
}
void result_free(int q) {
    pthread_mutex_lock(&results[q].mut);
    results[q].depth = -1;
    pthread_mutex_unlock(&results[q].mut);
}

void *do_mpi_thread(void *);

struct MPI_Thread {
    smart_ptr<search_info> info;
    pthread_t thread;
    pthread_mutex_t mut;
    pfunc_v pfunc;
    int windex;
    bool in_use;
    MPI_Thread() : in_use(false) {
        pthread_mutex_init(&mut,NULL);
    }
    bool alloc() {
        pthread_mutex_lock(&mut);
        bool allocated = !in_use;
        if(allocated)
            in_use = true;
        pthread_mutex_unlock(&mut);
        return allocated;
    }
    void free() {
        pthread_mutex_lock(&mut);
        in_use = false;
        pthread_mutex_unlock(&mut);
    }
    void start() {
        pthread_create(&thread,NULL,do_mpi_thread,this);
    }
};
const int MPI_Thread_count = chx_threads_per_proc();

void *do_mpi_thread(void *voidp) {
#ifdef MPI_SUPPORT
    MPI_Thread *mp = (MPI_Thread*)voidp;
    score_t result;
    if(mp->pfunc == search_ab_f) {
        result = search_ab(mp->info->board,mp->info->board.depth,mp->info->alpha,mp->info->beta);
    } else if(mp->pfunc == search_f)
        result = search(mp->info->board,mp->info->board.depth);
    else if(mp->pfunc == qeval_f)
        result = qeval(mp->info->board,mp->info->alpha,mp->info->beta);
    //else if(func == strike_f)
    //result = strike(board,board.depth,alpha,beta);
    else
        abort();
    int result_data[2];
    result_data[1] = result;
    result_data[0] = mp->windex;
    assert(mp->windex != -1);
    MPI_Send(result_data,2,MPI_INT,
            0,WORK_COMPLETED,MPI_COMM_WORLD);
    mp->free();
#endif
    return NULL;
}

void *mpi_worker(void *)
{
    int result_data[2];
#ifdef MPI_SUPPORT
    if(mpi_rank==0) {
        int count = mpi_size -1;
        while(count>0)
        {
            MPI_Status status;
            int err = MPI_Recv(result_data,3,MPI_INT,
                MPI_ANY_SOURCE,WORK_COMPLETED,MPI_COMM_WORLD,&status);
            assert(err == MPI_SUCCESS);
            if(result_data[0] == -1)
                count--;
            else {
                //pthread_mutex_lock(&res_mut);
                //results[result_data[0]][result_data[1]].set_result(result_data[2]);
                results[result_data[0]].set_result(result_data[1]);
                //pthread_mutex_unlock(&res_mut);
            }
        }
    } else {
        init_hash();
        MPI_Thread mpi_threads[MPI_Thread_count];
        while(true)
        {
            int mpi_data[mpi_ints];
            MPI_Status status;
            int err = MPI_Recv(mpi_data,mpi_ints,MPI_INT,
                    0,WORK_ASSIGN_MESSAGE,MPI_COMM_WORLD,&status);
            assert(err == MPI_SUCCESS);
            int n = 0;
            node_t board;
            for(int i=0;i<16;i++) {
                int_to_chars(mpi_data[n++],
                    board.color[4*i],board.color[4*i+1],
                    board.color[4*i+2],board.color[4*i+3]);
            }
            for(int i=0;i<16;i++) {
                int_to_chars(mpi_data[n++],
                    board.piece[4*i],board.piece[4*i+1],
                    board.piece[4*i+2],board.piece[4*i+3]);
            }
            board.hash = mpi_data[n++];
            board.depth = mpi_data[n++];
            if(board.depth == -1) {
                result_data[0] = -1;
                MPI_Send(result_data,2,MPI_INT,
                    0,WORK_COMPLETED,MPI_COMM_WORLD);
                MPI_Finalize();
                exit(0);
            }
            assert(board.depth >= 0 && board.depth < 7);
            board.side =  mpi_data[n++];
            board.castle = mpi_data[n++];
            board.ep = mpi_data[n++];
            board.ply = mpi_data[n++];
            board.hply = mpi_data[n++];
            board.fifty = mpi_data[n++];
            score_t alpha = mpi_data[n++];
            score_t beta = mpi_data[n++];
            pfunc_v func = (pfunc_v)mpi_data[n++];
            int windex = mpi_data[n++];
            int hist_size = mpi_data[n++];

            if(board.hply > 0) {
                //int fifty_array[50];
                /*
                FixedArray<int,50> fifty_array;
                int err = MPI_Recv(fifty_array.ptr(),board.hply,MPI_INT,
                        0,WORK_SUPPLEMENT,MPI_COMM_WORLD,&status);
                assert(err == MPI_SUCCESS);
                */
                board.hist_dat.resize(hist_size);
                for(int i=0;i<hist_size;i++)
                    board.hist_dat[i] = mpi_data[n++];//fifty_array[i];
            }
            
            for(int i=0;i<MPI_Thread_count;i++) {
                if(mpi_threads[i].alloc()) {
                    smart_ptr<search_info> info = new search_info(board);
                    info->beta = beta;
                    info->alpha = alpha;
                    info->depth = board.depth;
                    MPI_Thread *mp = &mpi_threads[i];
                    mp->windex = windex;
                    mp->pfunc = func;
                    mp->info = info;
                    mp->start();
                    break;
                }
            }
            /*
            search_info *s = new search_info(board);
            s->alpha = alpha;
            s->beta = beta;
            s->depth = board.depth;
            pthread_create(
            */

            /*
            score_t result;
            if(func == search_ab_f) {
                result = search_ab(board,board.depth,alpha,beta);
            } else if(func == search_f)
                result = search(board,board.depth);
            else if(func == qeval_f)
                result = qeval(board,alpha,beta);
            //else if(func == strike_f)
                //result = strike(board,board.depth,alpha,beta);
            else
                abort();
            int result_data[2];
            result_data[1] = result;
            result_data[0] = windex;
            assert(windex != -1);
            MPI_Send(result_data,2,MPI_INT,
                    0,WORK_COMPLETED,MPI_COMM_WORLD);
                    */
        }
    }
#endif
    return NULL;
}

void sort_pv(std::vector<move>& workq, int ply)
{
  if(ply >= pv.size())
    return;
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
void int_from_chars(int& i1,char c1,char c2,char c3,char c4) {
    i1 = c1 << 8*3 | c2 << 8*2 | c3 << 8 | c4;
}
void int_to_chars(int i1,char& c1,char& c2,char& c3,char& c4) {
    c4 = i1 & 0xFF;
    c3 = (i1 >> 8) & 0xFF;
    c2 = (i1 >> 8*2) & 0xFF;
    c1 = (i1 >> 8*3) & 0xFF;
}
