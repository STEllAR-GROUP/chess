////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2011 Steve Brandt and Philip LeBlanc
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file BOOST_LICENSE_1_0.rst or copy at http://www.boost.org/LICENSE_1_0.txt)
////////////////////////////////////////////////////////////////////////////////
/*
 *  search.cpp
 */
#ifdef HPX_SUPPORT
#include <hpx/hpx_fwd.hpp>
#endif

#include "parallel_support.hpp"
#include "search.hpp"
#include "parallel.hpp"
#include <algorithm>
#include <math.h>
#include <assert.h>
#include "here.hpp"
#include "zkey.hpp"
#include "log_board.hpp"
#include <fstream>
#include <sstream>

Mutex mutex;
const int num_proc = chx_threads_per_proc();

int min(int a,int b) { return a < b ? a : b; }
int max(int a,int b) { return a > b ? a : b; }

std::vector<safe_move> pv;  // Principle Variation, used in iterative deepening

/**
 * This determines whether we have a capture. It's not sophisticated
 * enough to do en passant.
 */
bool capture(const node_t& board,chess_move& g) {
  //bool b1 = board.color[g.getTo()] != EMPTY
      //&& board.color[g.getTo()] != board.color[g.getFrom()];
  bool b2 = g.getCapture();
  //if(b1 != b2) abort();
  return b2;
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
#ifdef HPX_SUPPORT
    if(file_output_enabled) {
        int n = hpx::get_worker_thread_num();
        if(streams[n] == NULL) {
            std::stringstream outname;
            streams[n] = new std::ofstream();
            outname << "chess_out" << n << ".txt";
            streams[n]->open(outname.str());
            std::cout << "file: " << outname.str() << std::endl;
        }
        log_board(info->board,*streams[n]);
    }
#endif
    node_t board = info->board;
    score_t lower = info->alpha;
    score_t upper = info->beta;
    evaluator ev;
    DECL_SCORE(s,ev.eval(board, chosen_evaluator),board.hash);
    s = max(lower,s);
    std::vector<chess_move> workq;
    gen(workq, board); // Generate the moves
    for(size_t j=0;j < workq.size(); j++) {
        chess_move g = workq[j];
        if(g.getCapture())
            continue;
        if(info->get_abort())
            return s;
        node_t p_board = board;
        if(!makemove(p_board,g))
            continue;
        DECL_SCORE(v,ev.eval(p_board,chosen_evaluator),p_board.hash);
        s = max(v,s);
        if(s > upper) {
            return s;
        }
    }
    for(size_t j=0;j < workq.size(); j++) {
        if(!workq[j].getCapture())
            continue;
        if(info->get_abort())
            return s;
        chess_move g = workq[j];
        node_t p_board = board;
        if(!makemove(p_board,g))
            continue;
        search_info* new_info = new search_info;
        new_info->set_abort_ref(info);
        new_info->board = p_board;
        new_info->alpha = -upper;
        new_info->beta = -lower;
        s = max(-qeval(new_info),s);
        delete new_info;
        if(s > upper) {
            return s;
        }
    }
    return s;
}

void qeval_pt(search_info *info)
{
  info->result = qeval(info);
  task_counter.add(1);
}

smart_ptr<task> parallel_task(int depth, bool *parallel) {

    if(!*parallel) {
        smart_ptr<task> t = new serial_task;
        return t;
    }
    bool use_parallel = false;
//#ifdef HPX_SUPPORT
//    use_parallel = depth % 3 == 2;
//#else
    use_parallel = depth >= 3;
//#endif
    if(use_parallel) {
        int n = task_counter.dec();
        if(n > 0) {
#ifdef HPX_SUPPORT
            smart_ptr<task> t = new hpx_task;
#else
            smart_ptr<task> t = new thread_task;
#endif
            *parallel = (n > 1);
            return t;
        }
    }
    *parallel = false;
    smart_ptr<task> t = new serial_task;
    return t;
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
  board.ply = 0;

  if (search_method == MINIMAX) {
    root->pfunc = search_f;
    
    search_info* info = new search_info;
    info->board = board;
    info->depth = depth[board.side];
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
    score_t f(search_ab(info));
    delete info;
    while(d < depth[board.side]) {
        d+=stepsize;
        board.depth = d;
        f = mtdf(board,f,d);
        root.dec();
        root = new serial_task;
    }
    if (bench_mode)
      std::cout << "SCORE=" << f << std::endl;
  } else if (search_method == ALPHABETA) {
    root->pfunc = search_ab_f;
    // Initially alpha is -infinity, beta is infinity
    score_t f=0;
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
      f = search_ab(info);
      delete info;

      if (i >= iter_depth)  // if our ply is greater than the iter_depth, then break
      {
        brk = true;
        break;
      }
      root.dec();
      root = new serial_task;
    }

    if (brk) {
      search_info* info = new search_info;
      info->board = board;
      info->depth = depth[board.side];
      info->alpha = alpha;
      info->beta = beta;
      f=search_ab(info);
      delete info;
    }
    if (bench_mode)
      std::cout << "SCORE=" << f << std::endl;
  }
  root.dec();
  return 1;
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
            search_info* info = new search_info;
            info->board = board;
            info->depth = depth;
            info->alpha = lower;
            info->beta = upper;
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
