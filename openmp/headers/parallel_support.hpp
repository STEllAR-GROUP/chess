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

typedef void *(*pthread_func_t)(void*);

void *search_pt(void *);
void *search_ab_pt(void *);
void *strike(void *);
void *qeval_pt(void *);

struct task;

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
    }
};

enum pfunc_v { no_f, search_f, search_ab_f, strike_f, qeval_f };

struct task {
    smart_ptr<search_info> info;

    smart_ptr<task> parent_task;
    std::vector< smart_ptr<task> > children;
    //pthread_func_t pfunc;
    pfunc_v pfunc;
    task() : pfunc(no_f) {
    }
    virtual ~task() {}

    virtual void start() = 0;

    virtual void join() = 0;

    virtual void abort_search() = 0;

    virtual bool check_abort() = 0;
};
struct serial_task : public task {
    serial_task() {}
    virtual ~serial_task() {
    }

    virtual void start() {}

    virtual void join() {
        if(pfunc == search_f)
            search_pt(info.ptr());
        else if(pfunc == search_ab_f)
            search_ab_pt(info.ptr());
        else if(pfunc == strike_f)
            strike(info.ptr());
        else if(pfunc == qeval_f)
            qeval_pt(info.ptr());
        else
            abort();
    }

    virtual void abort_search() {}

    virtual bool check_abort() { 
      return false;
    }
};
struct pcounter {
    int count;
    pthread_mutex_t mut;
    pthread_cond_t cond;
    pcounter() : count(0) {
        pthread_mutex_init(&mut,NULL);
        pthread_cond_init(&cond,NULL);
    }
    pcounter(int n) : count(n) {
        pthread_mutex_init(&mut,NULL);
    }
    int add(int n) {
        pthread_mutex_lock(&mut);
        int old = count;
        count += n;
        if(old == 0 && count > 0)
            pthread_cond_broadcast(&cond);
        int m = count;
        pthread_mutex_unlock(&mut);
        return m;
    }
    bool wait_dec() {
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
    virtual ~pthread_task() {
        //join();
    }
    virtual void start() {
        //pthread_mutex_lock(&mut);
        joined = false;
        //pthread_mutex_unlock(&mut);
        //pthread_attr_init(&attr);
        //pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_JOINABLE);
        //pthread_create(&thread,&attr,pfunc,info.ptr());
        info->set_parallel();
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
    virtual void abort_search() {
        pthread_mutex_lock(&mut);
        abort_flag = true;
        pthread_mutex_unlock(&mut);
    }

    virtual bool check_abort() {
        bool ret;
        pthread_mutex_lock(&mut);
        ret = abort_flag;
        pthread_mutex_unlock(&mut);
        return ret;
    }
};
#endif
