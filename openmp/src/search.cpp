////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2011 Steve Brandt and Philip LeBlanc
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file BOOST_LICENSE_1_0.rst or copy at http://www.boost.org/LICENSE_1_0.txt)
////////////////////////////////////////////////////////////////////////////////
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

#define PV_ON 1

score_t multistrike(const node_t& board,score_t f,int depth);

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
const int num_proc = chx_threads_per_proc();

bool multistrike_on = false;

int min(int a,int b) { return a < b ? a : b; }
int max(int a,int b) { return a > b ? a : b; }

struct safe_move {
    pthread_mutex_t mut;
    move mv;
    safe_move() {
        pthread_mutex_init(&mut,NULL);
    }
    void set(move mv_) {
        pthread_mutex_lock(&mut);
        mv = mv_;
        pthread_mutex_unlock(&mut);
    }
    move get() {
        pthread_mutex_lock(&mut);
        move m = mv;
        pthread_mutex_unlock(&mut);
        return m;
    }
};
std::vector<safe_move> pv;  // Principle Variation, used in iterative deepening

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
score_t qeval(const node_t& board,const score_t& lower,const score_t& upper, smart_ptr<task> this_task)
{
    evaluator ev;
    DECL_SCORE(s,ev.eval(board, chosen_evaluator),board.hash);
    s = max(lower,s);
    std::vector<move> workq;
    gen(workq, board); // Generate the moves
    /*if(chx_abort)
        return bad_min_score;*/
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
            s = max(-qeval(p_board,-upper,-lower, this_task),s);
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
  info->result = qeval(info->board,info->alpha, info->beta, info->this_task);
  return NULL;
}

smart_ptr<task> parallel_task(int depth) {
    /*if(depth >= 3) {
        if(mpi_task_array[0].dec()) {
            smart_ptr<task> t = new pthread_task;
            return t;
        }
    }
    if(depth >= 4) {
        for(int i=1;i<mpi_size;i++) {
            if(mpi_task_array[i].dec()) {
                smart_ptr<task> t = new mpi_task(i);
                return t;
            }
        }
    }*/
    smart_ptr<task> t = new serial_task;
    return t;
}

void *strike(void *vptr) {
  search_info *info = (search_info *)vptr;
  info->result = search_ab(info->board,info->depth,info->alpha,info->beta, info->this_task);
  if(info->alpha < info->result && info->result < info->beta) {
    //std::cout << "FOUND: " << VAR(info->result) << std::endl;
    if (info->this_task->parent_task.valid()) {
      info->this_task->parent_task->abort_search();
      return NULL;
    }
  }
  mpi_task_array[0].add(1);
  return NULL;
}

// think() calls a search function 
int think(node_t& board,bool parallel)
{
  smart_ptr<task> root = new serial_task;
#ifdef PV_ON
  pv.clear();
  pv.resize(depth[board.side]);
  move mvz;
  mvz.u = INVALID_MOVE;
  for(int i=0;i<pv.size();i++) {
    pv[i].set(mvz);
  }
#endif
  for(int i=0;i<table_size;i++) {
    transposition_table[i].depth = -1;
    transposition_table[i].lower = bad_min_score;
    transposition_table[i].upper = bad_max_score;
  }
  multistrike_on = false;
  board.ply = 0;

  if (search_method == MINIMAX) {
    root->pfunc = search_f;
    
    score_t f = search(board, depth[board.side], root);
    
    assert(move_to_make.u != INVALID_MOVE);
    if (bench_mode)
      std::cout << "SCORE=" << f << std::endl;
  } else if (search_method == MTDF) {
    root->pfunc = search_ab_f;
    DECL_SCORE(alpha,-10000,board.hash);
    DECL_SCORE(beta,10000,board.hash);
    int stepsize = 2;
    int d = depth[board.side] % stepsize;
    if(d == 0)
        d = stepsize;
    board.depth = d;
    score_t f(search_ab(board,d,alpha,beta, root));
    while(d < depth[board.side]) {
        d+=stepsize;
        board.depth = d;
        f = mtdf(board,f,d, root);
    }
    if (bench_mode)
      std::cout << "SCORE=" << f << std::endl;
  } else if (search_method == MULTISTRIKE) {
    root->pfunc = strike_f;
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
    board.depth = depth[board.side];
    score_t f = multistrike(board,0,depth[board.side], root);
    if (bench_mode)
      std::cout << "SCORE=" << f << std::endl;
  } else if (search_method == ALPHABETA) {
    root->pfunc = search_ab_f;
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
      f = search_ab(board, i, alpha, beta, root);

      if (i >= iter_depth)  // if our ply is greater than the iter_depth, then break
      {
        brk = true;
        break;
      }
    }

    if (brk)
      f=search_ab(board, depth[board.side], alpha, beta, root);
    if (bench_mode)
      std::cout << "SCORE=" << f << std::endl;
  }
  return 1;
}

score_t multistrike(const node_t& board,score_t f,int depth, smart_ptr<task> this_task)
{
    //std::cout << VAR(num_proc) << VAR(mpi_task_array[0].add(0)) << std::endl;
    //chx_abort = false;
    score_t ret=0;
    const int max_parallel = (num_proc-1)/2;
    assert(max_parallel > 0);
    const int fac = 600/max_parallel;//10*25/max_parallel;
    std::vector<smart_ptr<task> > tasks;
    DECL_SCORE(lower,-10000,0);
    DECL_SCORE(upper,10000,0);
    for(int i=-max_parallel;i<=max_parallel;i++) {
        mpi_task_array[0].wait_dec();
    }
    for(int i=-max_parallel;i<=max_parallel;i++) {
        DECL_SCORE(alpha,i==-max_parallel ? -10000 : fac*(i)  ,0);
        DECL_SCORE(beta, i== max_parallel ?  10000 : fac*(i+1),0);
        beta++;
        smart_ptr<task> t = parallel_task(depth);
        t->info = new search_info(board);
        t->info->alpha = alpha;
        t->info->beta = beta;
        t->info->depth = depth;
        t->pfunc = strike_f;
        t->info->this_task = t;
        this_task->children.push_back(t);
        t->parent_task = this_task;
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
        }
        tasks[i]->info->this_task = 0;
    }
    return ret;
}


/** MTD-f */
score_t mtdf(const node_t& board,score_t f,int depth, smart_ptr<task> this_task)
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
            g = search_ab(board,depth,lower,upper, this_task);
            break;
        } else {
            alpha = max(g == lower ? lower+1 : lower,ADD_SCORE(g,    -(1+width/2)));
            beta  = min(g == upper ? upper-1 : upper,ADD_SCORE(alpha, (1+width)));
        }
        g = search_ab(board,depth,alpha,beta, this_task);
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

void sort_pv(std::vector<move>& workq, int index)
{
  if(index < pv.size())
    return;
  move temp = pv[index].get();
  if(temp.u == INVALID_MOVE)
    return;
  for(int i = 0; i < workq.size() ; i++)
  {
    if (workq[i].u == temp.u) /* If we have a move in the work queue that is the 
                                    same as the best move we have searched before */
    {
      temp = workq[0];
      workq[0] = workq[i];
      workq[i] = temp;
      break;
    }
  }
}

#define TRANSPOSE_ON 1

zkey_t transposition_table[table_size];

bool get_transposition_value(const node_t& board,score_t& lower,score_t& upper) {
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
    pthread_mutex_lock(&z->mut);
    if(board.depth >= z->depth) {
        z->board = board;
        z->lower = lower;
        z->upper = upper;
        z->depth = board.depth;
    }
    pthread_mutex_unlock(&z->mut);
#endif
}
