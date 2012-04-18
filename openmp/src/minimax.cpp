////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2012 Steve Brandt and Phillip LeBlanc
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file BOOST_LICENSE_1_0.rst or copy at http://www.boost.org/LICENSE_1_0.txt)
////////////////////////////////////////////////////////////////////////////////

#include "parallel_support.hpp"
#include "search.hpp"
#include <assert.h>
#include <pthread.h>

void *search_pt(void *vptr) {
    search_info *info = (search_info *)vptr;
    //assert(info->depth == info->board.depth);
    info->result = search(info);
    if(info->self.valid()) {
        info->set_done();
        mpi_task_array[0].add(1);
        info->self = 0;
        pthread_exit(NULL);
    }
    return NULL;
}

score_t search(search_info* info)
{
    node_t board = info->board;
    int depth = info->depth;
    smart_ptr<task> this_task = info->this_task;
    assert(depth >= 0);
    // if we are a leaf node, return the value from the eval() function
    if (depth == 0)
    {
        evaluator ev;
        DECL_SCORE(s,ev.eval(board, chosen_evaluator),board.hash);
        return s;
    }
    /* if this isn't the root of the search tree (where we have
       to pick a chess_move and can't simply return 0) then check to
       see if the position is a repeat. if so, we can assume that
       this line is a draw and return 0. */
    if (board.ply && reps(board)) {
        DECL_SCORE(s,0,board.hash);
        return s;
    }

    // fifty chess_move draw rule
    if (board.fifty >= 100) {
        DECL_SCORE(z,0,board.hash);
        return z;
    }

    score_t val, max;

    std::vector<chess_move> workq;
    chess_move max_move;
    max_move = INVALID_MOVE;

    gen(workq, board); // Generate the moves

    // DECL_SCORE(minf,-10000,board.hash);
    max = bad_min_score; // Set the max score to -infinity

    // const int worksq = workq.size();
    std::vector<smart_ptr<task> > tasks;

    // loop through the moves
    // We do this twice. The first time we skip
    // quiescent searches, the second time we
    // do the quiescent search. By doing this
    // we get the best value of beta to produce
    // cutoffs within the quiescent search routine.
    // Without doing this, minimax runs extremely
    // slowly.
    for(int mm=0;mm<2;mm++) {
        for(size_t j=0;j < workq.size(); j++) {
            bool last = (j+1)==workq.size();
            chess_move g = workq[j];
            smart_ptr<search_info> info = new search_info(board);

            if (makemove(info->board, g)) {  
                DECL_SCORE(z,0,board.hash);
                info->depth = depth-1;
                info->mv = g;
                info->result = z;
                bool parallel;
                if(depth == 1 && capture(board,g)) {
                    if(mm==1) {
                        smart_ptr<task> t = 
                            parallel_task(depth, &parallel);
                        t->info = info;
                        DECL_SCORE(lo,-10000,0);
                        t->info->beta = -max;
                        t->info->alpha = lo;
                        t->info->this_task = t;
                        t->parent_task = this_task;
                        t->pfunc = qeval_f;
                        tasks.push_back(t);
                        t->start();
                    }
                } else {
                    if(mm==0) {
                        smart_ptr<task> t = 
                            parallel_task(depth, &parallel);
                        t->info = info;
                        t->pfunc = search_f;
                        tasks.push_back(t);
                        t->start();
                    }
                }
            }
            if(tasks.size()>=(size_t)num_proc||last) {
                for(size_t n=0;n<tasks.size();n++) {
                    smart_ptr<search_info> info = tasks[n]->info;
                    tasks[n]->join();
                    val = -tasks[n]->info->result;

                    if (val > max)
                    {
                        max = val;
                        max_move = info->mv;
                    }
                    tasks[n]->info->this_task = 0;
                    //tasks[n]->info = 0;
                }
                tasks.clear();
            }
        }
    }
    assert(tasks.size()==0);

    // no legal moves? then we're in checkmate or stalemate
    if (max_move == INVALID_MOVE) {
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
        assert(max_move != INVALID_MOVE);
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
