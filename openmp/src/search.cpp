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
#include "zkey.hpp"

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

smart_ptr<task> parallel_task(int depth) {
    if(mpi_rank != 0 || depth <= 3 || depth % 2 == 1) {
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
  pv.clear();
  for(int i=0;i<table_size;i++)
    transposition_table[i].depth = -1;
  multistrike_on = false;
  chx_abort = false;
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
    assert(move_to_make.u != INVALID_MOVE);
    if (bench_mode)
      std::cout << "SCORE=" << f << std::endl;
  } else if (search_method == MTDF) {
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
      board.depth = i;
      f = search_ab(board, i, alpha, beta);

      if (i >= iter_depth)  // if our ply is greater than the iter_depth, then break
      {
        brk = true;
        break;
      }
    }

    if (brk)
      f=search_ab(board, depth[board.side], alpha, beta);
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
    const int max_parallel = 30;
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
        smart_ptr<task> t = parallel_task(depth);
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
    const int start_width = 4;//atoi(getenv("START_WIDTH"));
    // Sometimes MTD-f gets stuck and can try many
    // times without finding an answer. If this happens
    // we want to set a threshold for bailing out.
    const int max_tries = 4;//atoi(getenv("MAX_TRIES"));
    // If our first guess isn't right, chances are
    // we want to search a little wider the next try
    // to improve our odds.
    const int grow_width = 4;//atoi(getenv("GROW_WIDTH"));
    int width = start_width;
    const int max_width = start_width+grow_width*max_tries;
    score_t alpha = lower, beta = upper;
    while(lower < upper) {
        if(width >= max_width) {
            g = search_ab(board,depth,lower,upper);
            break;
        } else {
            alpha = max(g == lower ? lower+1 : lower,ADD_SCORE(g,    -(1+width/2)));
            beta  = min(g == upper ? upper-1 : upper,ADD_SCORE(alpha, (1+width)));
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
                            para ? parallel_task(depth) : new task;
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
                            para ? parallel_task(depth) : new task;
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

    score_t max_val = bad_min_score;
    score_t zlo,zhi;
    if(get_transposition_value(board,zlo,zhi)) {
        if(zlo >= beta)
            return zlo;
        if(alpha >= zhi)
            return zhi;
        alpha = max(zlo,alpha);
        beta  = min(zhi,beta);
    }
    if(alpha >= beta)
        return alpha;

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
        if(alpha >= beta)
            continue;
        move g = workq[j];
        node_t p_board = board;

        if (!makemove(p_board, g.b)) { // Make the move, if it isn't 
            continue;                    // legal, then go to the next one
        }

        assert(depth >= 1);
        p_board.depth = depth-1;
        if(depth == 1 && capture(board,g)) {
            val = -qeval(p_board, -beta, -alpha);
        } else
            val = -search_ab(p_board, depth-1, -beta, -alpha);

        if (val > max_val)
        {
            max_val = val;
            max_move = g;
            if (val > alpha)
            {
                alpha = val;
                pthread_mutex_lock(&mutex);
                if(board.ply >= pv.size())
                    pv.resize(board.ply+1);
                pv[board.ply] = g;
                pthread_mutex_unlock(&mutex);
            }
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
            DECL_SCORE(v,-10000,0);
            return v;
        }

        if (makemove(info->board, g.b)) {

            smart_ptr<task> t = para ? parallel_task(depth) : new task;

            t->info = info;
            info->board.depth = info->depth = depth-1;
            assert(depth >= 0);
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

                if (val > max_val) {
                    max_val = val;
                    max_move = info->mv;
                    if (val > alpha)
                    {
                        alpha = val;
                        pthread_mutex_lock(&mutex);
                        if(board.ply >= pv.size())
                            pv.resize(board.ply+1);
                        pv[board.ply] = info->mv;
                        pthread_mutex_unlock(&mutex);
                        if(alpha >= beta)
                            break;
                    }
                }
            }
            tasks.clear();
            if(alpha >= beta)
                break;
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

    set_transposition_value(board,
        max(zlo,max_val >= beta  ? max_val : bad_min_score),
        min(zhi,max_val <= alpha ? max_val : bad_max_score));

    assert(max_val != bad_min_score);
    return max_val;
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
    if (board.hist_dat.get(i) == board.hash)
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

//#define TRANSPOSE_ON 1

zkey_t transposition_table[table_size];

bool get_transposition_value(const node_t& board,score_t& lower,score_t& uppper) {
    bool gotten = false;
#ifdef TRANSPOSE_ON
    int n = abs(board.hash^board.depth) % table_size;
    zkey_t *z = &transposition_table[n];
    pthread_mutex_lock(&z->mut);
    if(z->depth >= 0 && board_equals(board,z->board)) {
        lower = z->lower;
        upper = z->upper;
        gotten = true;
    } else {
        lower = bad_min_score;
        upper = bad_max_score;
    }
    pthread_mutex_unlock(&z->mut);
#endif
    return gotten;
}

void set_transposition_value(const node_t& board,score_t lower,score_t upper) {
#ifdef TRANSPOSE_ON
    int n = abs(board.hash^board.depth) % table_size;
    zkey_t *z = &transposition_table[n];
    if(board.depth < z->depth)
        return;
    pthread_mutex_lock(&z->mut);
    z->board = board;
    z->lower = lower;
    z->upper = upper;
    z->depth = board.depth;
    pthread_mutex_unlock(&z->mut);
#endif
}
