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
#include "parallel.hpp"
#include <algorithm>
#include <math.h>
#include <assert.h>
#include "mpi_support.hpp"
#include "here.hpp"
#include "zkey.hpp"

Mutex mutex;
const int num_proc = chx_threads_per_proc();

bool multistrike_on = false;

int min(int a,int b) { return a < b ? a : b; }
int max(int a,int b) { return a > b ? a : b; }

std::vector<safe_move> pv;  // Principle Variation, used in iterative deepening

/**
 * This determines whether we have a capture. It's not sophisticated
 * enough to do en passant.
 */
bool capture(const node_t& board,chess_move& g) {
  return board.color[g.getTo()] != EMPTY
      && board.color[g.getTo()] != board.color[g.getFrom()];
}

/**
 * Quiescent evaluator. Originally we wished to avoid the complexity
 * of quiescent chess_move searches, but MTD-f does not seem to work properly
 * without it. This particular search evaluates each chess_move using the
 * evalutaor, unless the chess_move is a capture then it calls itself
 * recursively. The quiescent chess_move search uses alpha-beta to speed
 * itself along, and evaluates non-captures first to get the greatest
 * cutoff.
 **/
score_t qeval(search_info* info)
{
    node_t board = info->board;
    score_t lower = info->alpha;
    score_t upper = info->beta;
    smart_ptr<task> this_task = info->this_task;
    evaluator ev;
    DECL_SCORE(s,ev.eval(board, chosen_evaluator),board.hash);
    s = max(lower,s);
    std::vector<chess_move> workq;
    gen(workq, board); // Generate the moves
    /*if(chx_abort)
        return bad_min_score;*/
    for(size_t j=0;j < workq.size(); j++) {
        chess_move g = workq[j];
        node_t p_board = board;
        if(!makemove(p_board,g))
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
    for(size_t j=0;j < workq.size(); j++) {
        chess_move g = workq[j];
        node_t p_board = board;
        if(!makemove(p_board,g))
            continue;
        if(capture(board,g)) {
            search_info* new_info = new search_info;
            new_info->board = p_board;
            new_info->alpha = -upper;
            new_info->beta = -lower;
            new_info->this_task = this_task;
            s = max(-qeval(new_info),s);
            delete new_info;
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
    search_info *info=(search_info*)vptr;
    assert(info!=0);
    assert(info->self.valid());
    {
        info->result=qeval(info);
        info->set_done();
        mpi_task_array[0].add(1);
        smart_ptr<search_info> eg=info->self;
        if(info->result>=info->beta){
            info->this_task->abort_search();
        }
        info->self=0;
        pthread_exit(0);
    }
    return 0;
}

smart_ptr<task> parallel_task(int depth, bool *parallel) {

    if(depth >= 1) {
        if(mpi_task_array[0].dec()) {
#ifdef HPX_ENABLED
            smart_ptr<task> t = new hpx_task;
#else
            smart_ptr<task> t = new pthread_task;
#endif
            *parallel = true;
            return t;
        }
    }
    /*
    if(depth >= 4) {
        for(int i=1;i<mpi_size;i++) {
            if(mpi_task_array[i].dec()) {
                smart_ptr<task> t = new mpi_task(i);
                *parallel = true;
                return t;
            }
        }
    }
    */
    *parallel = false;
    smart_ptr<task> t = new serial_task;
    return t;
}

void *strike(void *vptr) {
  search_info *info = (search_info *)vptr;
  info->result = search_ab(info);
  info->self = 0;
  if(info->alpha < info->result && info->result < info->beta) {
    //std::cout << "FOUND: " << VAR(info->result) << std::endl;
    info->this_task->abort_search();
    return NULL;
  }
  info->set_done();
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
  chess_move mvz;
  mvz = INVALID_MOVE;
  for(size_t i=0;i<pv.size();i++) {
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
    
    search_info* info = new search_info;
    info->board = board;
    info->depth = depth[board.side];
    info->this_task = root;
    score_t f = search(info);
    delete info;
    
    assert(move_to_make != INVALID_MOVE);
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
    search_info* info = new search_info;
    info->board = board;
    info->depth = d;
    info->alpha = alpha;
    info->beta = beta;
    info->this_task = root;
    score_t f(search_ab(info));
    delete info;
    while(d < depth[board.side]) {
        d+=stepsize;
        board.depth = d;
        f = mtdf(board,f,d, root);
        root = new serial_task;
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
    search_info* info = new search_info;
    info->board = board;
    info->alpha = 0;
    info->depth = depth[board.side];
    info->this_task = root;
    score_t f = multistrike(info);
    delete info;
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
      search_info* info = new search_info;
      info->board = board;
      info->depth = i;
      info->alpha = alpha;
      info->beta = beta;
      info->this_task = root; 
      f = search_ab(info);
      delete info;

      if (i >= iter_depth)  // if our ply is greater than the iter_depth, then break
      {
        brk = true;
        break;
      }
      root = new serial_task;
    }

    if (brk) {
      search_info* info = new search_info;
      info->board = board;
      info->depth = depth[board.side];
      info->alpha = alpha;
      info->beta = beta;
      info->this_task = root;
      f=search_ab(info);
      delete info;
    }
    if (bench_mode)
      std::cout << "SCORE=" << f << std::endl;
  }
  return 1;
}

score_t multistrike(search_info* info)
{
    node_t board = info->board;
    int depth = info->depth;
    smart_ptr<task> this_task = info->this_task;
    //std::cout << VAR(num_proc) << VAR(mpi_task_array[0].add(0)) << std::endl;
    //chx_abort = false;
    score_t ret=0;
    const int max_parallel = (num_proc-1)/2 ? (num_proc-1)/2 : 1 ;
    assert(max_parallel > 0);
    const int fac = 600/max_parallel;//10*25/max_parallel;
    std::vector<smart_ptr<task> > tasks;
    // DECL_SCORE(lower,-10000,0);
    // DECL_SCORE(upper,10000,0);
    for(int i=-max_parallel;i<=max_parallel;i++) {
        mpi_task_array[0].wait_dec();
    }
    std::cerr << "Checkpoint #1" << std::endl;
    for(int i=-max_parallel;i<=max_parallel;i++) {
        bool parallel;
        DECL_SCORE(alpha,i==-max_parallel ? -10000 : fac*(i)  ,0);
        DECL_SCORE(beta, i== max_parallel ?  10000 : fac*(i+1),0);
        beta++;
        smart_ptr<task> t = parallel_task(depth, &parallel);
        if (parallel)
            std::cerr << "Checkpoint #1.1 parallel:" << i << std::endl;
        else
            std::cerr << "Checkpoint #1.1:" << i << std::endl;
        t->info = new search_info(board);
        t->info->alpha = alpha;
        t->info->beta = beta;
        t->info->depth = depth;
        t->pfunc = strike_f;
        t->info->this_task = t;
        t->parent_task = this_task;
        t->start();
        tasks.push_back(t);
    }
    std::cerr << "Checkpoint #2" << std::endl;
    for(size_t i=0;i<tasks.size();i++) {
        std::cerr << "Checkpoint #3:" << i << ":" << tasks.size() << std::endl;
        tasks[i]->join();
        std::cerr << "Checkpoint #3:" << i << ":" << tasks.size() << std::endl;
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
            search_info* info = new search_info;
            info->board = board;
            info->depth = depth;
            info->alpha = lower;
            info->beta = upper;
            info->this_task = this_task;
            g = search_ab(info);
            delete info;
            break;
        } else {
            alpha = max(g == lower ? lower+1 : lower,ADD_SCORE(g,    -(1+width/2)));
            beta  = min(g == upper ? upper-1 : upper,ADD_SCORE(alpha, (1+width)));
        }
        search_info* info = new search_info;
        info->board = board;
        info->depth = depth;
        info->alpha = alpha;
        info->beta = beta;
        info->this_task = this_task;
        g = search_ab(info);
        delete info;
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

  for (i = 0; i < board.fifty; ++i) {
    assert(i < board.hist_dat.size());
    assert(board.hash != 0);
    if (board.hist_dat[i] == board.hash)
      ++r;
  }
  return r;
}

void sort_pv(std::vector<chess_move>& workq, int index)
{
  if((size_t)index < pv.size())
    return;
  chess_move temp = pv[index].get();
  if(temp == INVALID_MOVE)
    return;
  for(size_t i = 0; i < workq.size() ; i++)
  {
    if (workq[i] == temp) /* If we have a chess_move in the work queue that is the 
                                    same as the best chess_move we have searched before */
    {
      temp = workq[0];
      workq[0] = workq[i];
      workq[i] = temp;
      break;
    }
  }
}

#define TRANSPOSE_ON 1

// TODO: fix use of pthread mutex here

zkey_t transposition_table[table_size];

bool get_transposition_value(const node_t& board,score_t& lower,score_t& upper) {
    bool gotten = false;
#ifdef TRANSPOSE_ON
    int n = abs(board.hash^board.depth) % table_size;
    zkey_t *z = &transposition_table[n];
    ScopedLock s(z->mut);
    if(z->depth >= 0 && board_equals(board,z->board)) {
        lower = z->lower;
        upper = z->upper;
        gotten = true;
    } else {
        lower = bad_min_score;
        upper = bad_max_score;
    }
#endif
    return gotten;
}

void set_transposition_value(const node_t& board,score_t lower,score_t upper) {
#ifdef TRANSPOSE_ON
    int n = abs(board.hash^board.depth) % table_size;
    zkey_t *z = &transposition_table[n];
    ScopedLock s(z->mut);
    if(board.depth >= z->depth) {
        z->board = board;
        z->lower = lower;
        z->upper = upper;
        z->depth = board.depth;
    }
#endif
}
