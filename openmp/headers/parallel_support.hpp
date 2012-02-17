////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2011 Steve Brandt and Philip LeBlanc
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file BOOST_LICENSE_1_0.rst or copy at http://www.boost.org/LICENSE_1_0.txt)
////////////////////////////////////////////////////////////////////////////////
#ifndef PARALLEL_SUPPORT_HPP
#define PARALLEL_SUPPORT_HPP

#include <sys/time.h>
#include "smart_ptr.hpp"
#include "node.hpp"
#include "score.hpp"

extern bool par_enabled;
int chx_threads_per_proc();
extern pthread_attr_t pth_attr;
extern bool chx_abort;

struct task;

typedef void *(*pthread_func_t)(void*);

void *search_pt(void *);
void *search_ab_pt(void *);
void *strike(void *);
void *qeval_pt(void *);
score_t search(const node_t& board, int depth, smart_ptr<task> this_task);
score_t search_ab(const node_t& board, int depth, score_t alpha, score_t beta, smart_ptr<task> this_task);
score_t qeval(const node_t& board,const score_t& lower,const score_t& upper, smart_ptr<task> this_task);

struct search_info {
    // self-reference used
    // to delay cleanup
    smart_ptr<search_info> self;
    
    smart_ptr<task> this_task;
    
    pthread_mutex_t mut;
    pthread_cond_t cond;
    bool par_done;
    score_t result;
    int depth;
    int incr;
    score_t alpha;
    score_t beta;
    node_t board;
    move mv;

    void set_parallel() {
        par_done = false;
    }
    void set_done() {
        pthread_mutex_lock(&mut);
        par_done = true;
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mut);
    }
    void wait_for_done() {
        pthread_mutex_lock(&mut);
        //while(!par_done && !chx_abort) {
        while(!par_done) {
            timespec ts;
            timeval tv;
            gettimeofday(&tv, NULL);
            ts.tv_sec = tv.tv_sec + 0;
            ts.tv_nsec = 10000;
            pthread_cond_timedwait(&cond,&mut,&ts);
        }
        pthread_mutex_unlock(&mut);
    }
    search_info(const node_t& board_) : board(board_), par_done(true),
            result(bad_min_score) {
        pthread_mutex_init(&mut,NULL);
        pthread_cond_init(&cond,NULL);
        this_task = 0;
    }

    ~search_info() {
        this_task = 0;
    }
};

enum pfunc_v { no_f, search_f, search_ab_f, strike_f, qeval_f };

struct task {
    smart_ptr<search_info> info;

    smart_ptr<task> parent_task;
    //pthread_func_t pfunc;
    pfunc_v pfunc;
    task() : pfunc(no_f) {
    }
    virtual ~task() {
        info = 0;
        parent_task = 0;
    }

    virtual void start() = 0;

    virtual void join() = 0;

    virtual void abort_search() = 0;

    virtual void abort_search_parent() {}

    virtual bool check_abort() = 0;
};
struct serial_task : public task {
    serial_task() {}
    ~serial_task() {
        info = 0;
        parent_task = 0;
    }

    virtual void start() { }

    virtual void join() {
        if(!info.valid())
            return;
        info->self=0;
        if(pfunc == search_f)
            info->result = search(info->board,info->depth,info->this_task);
        else if(pfunc == search_ab_f)
            info->result = search_ab(info->board,info->depth, info->alpha, info->beta, info->this_task);
        else if(pfunc == strike_f)
            info->result = search_ab(info->board,info->depth,info->alpha,info->beta, info->this_task);
        else if(pfunc == qeval_f)
            info->result = qeval(info->board,info->alpha, info->beta, info->this_task);
        else
            abort();
        info->set_done();
    }

    virtual void abort_search() {}

    virtual bool check_abort() { 
        return false;
    }
};
class pcounter {
    int count, max_count;
    pthread_mutex_t mut;
    pthread_cond_t cond;
public:
    pcounter() : count(0), max_count(0) {
        pthread_mutex_init(&mut,NULL);
        pthread_cond_init(&cond,NULL);
    }
    void set_max(int n) {
        count = max_count = n;
    }
    int add(int n) {
        pthread_mutex_lock(&mut);
        int old = count;
        count += n;
        assert(count <= max_count);
        if(old == 0 && count > 0)
            pthread_cond_broadcast(&cond);
        int m = count;
        pthread_mutex_unlock(&mut);
        return m;
    }
    void wait_dec() {
        pthread_mutex_lock(&mut);
        while(count == 0)
            pthread_cond_wait(&cond,&mut);
        pthread_mutex_unlock(&mut);
    }
    bool dec() {
        pthread_mutex_lock(&mut);
        bool ret;
        if(count > 0) {
            ret = true;
            count--;
        } else {
            ret = false;
        }
        pthread_mutex_unlock(&mut);
        return ret;
    }
};
extern std::vector<pcounter> mpi_task_array;
struct pthread_task : public task {
    pthread_t thread;
    pthread_mutex_t mut;
    bool abort_flag;
    //pthread_attr_t attr;
    bool joined;
    pthread_task() : joined(true) {
        pthread_mutex_init(&mut,NULL);
        abort_flag = false;
    }
    ~pthread_task() {
        info = 0;
        parent_task = 0;
    }
    virtual void start() {
        //pthread_mutex_lock(&mut);
        joined = false;
        //pthread_mutex_unlock(&mut);
        //pthread_attr_init(&attr);
        //pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_JOINABLE);
        //pthread_create(&thread,&attr,pfunc,info.ptr());
        info->set_parallel();
        assert(info.valid());
        info->self = info;
        if(pfunc == search_f)
            pthread_create(&thread,&pth_attr,search_pt,info.ptr());
        else if(pfunc == search_ab_f)
            pthread_create(&thread,&pth_attr,search_ab_pt,info.ptr());
        else if(pfunc == strike_f)
            pthread_create(&thread,&pth_attr,strike,info.ptr());
        else if(pfunc == qeval_f)
            pthread_create(&thread,&pth_attr,qeval_pt,info.ptr());
        else
            abort();
    }
    virtual void join() {
        if(joined)
            return;
        info->wait_for_done();
        joined = true;
        //pthread_join(thread,NULL);
        //mpi_task_array[0].add(1);
    }

    // We need this function because we don't want to tell everyone
    // to abort, only the immediate parent
    virtual void abort_search_parent() {
        pthread_mutex_lock(&mut);
        abort_flag = true;
        pthread_mutex_unlock(&mut);
    }

    virtual void abort_search() {
        pthread_mutex_lock(&mut);
        abort_flag = true;
        parent_task->abort_search_parent();
        pthread_mutex_unlock(&mut);
    }


    virtual bool check_abort() {
        bool ret;
        pthread_mutex_lock(&mut);
        ret = abort_flag;
        if (parent_task.valid() && parent_task->check_abort())
            ret = true;
        pthread_mutex_unlock(&mut);
        return ret;
    }
};
#endif
