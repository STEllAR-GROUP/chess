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
#include "chess_move.hpp"

extern bool par_enabled;
int chx_threads_per_proc();
extern pthread_attr_t pth_attr;

struct task;

typedef void *(*pthread_func_t)(void*);

void *search_pt(void *);
void *search_ab_pt(void *);
void *strike(void *);
void *qeval_pt(void *);

struct search_info {
    // self-reference used
    // to delay cleanup
    smart_ptr<search_info> self;
    
    bool abort_flag;
    node_t board;
    bool par_done;
    chess_move mv;
    pthread_mutex_t mut;
    pthread_cond_t cond;
    score_t result;
    int depth;
    int incr;
    score_t alpha;
    score_t beta;

    void set_parallel() {
        par_done = false;
    }
    void set_done() {
        pthread_mutex_lock(&mut);
        par_done = true;
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mut);
    }
    void set_done_serial() {
        par_done = true;
    }
    void wait_for_done();
    search_info(const node_t& board_) : abort_flag(false), board(board_), par_done(true),
            result(bad_min_score) {
        pthread_mutex_init(&mut,NULL);
        pthread_cond_init(&cond,NULL);
    }

    search_info() {
    }

    ~search_info() {
    }
};

struct Threader {
    void *(*func)(void*);
    pthread_t p;
    Threader() : func(0) {}
    void create(void *(*f)(void*),void *args) {
        assert(func == 0);
        func = f;
        pthread_create(&p,0,func,args);
    }
    void join() {
        if(func != 0) {
            pthread_join(p,0);
            func = 0;
        }
    }
    ~Threader() {
        if(func != 0)
            pthread_detach(p);
    }
};

score_t search(search_info*);
score_t search_ab(search_info*);
score_t qeval(search_info*);

enum pfunc_v { no_f, search_f, search_ab_f, strike_f, qeval_f };

struct task {
    smart_ptr<search_info> info;

    pfunc_v pfunc;
    task() : pfunc(no_f) {
    }
    virtual ~task() {
        info = 0;
    }

    virtual void start() = 0;

    virtual void join() = 0;
};

inline void search_info::wait_for_done() {
    pthread_mutex_lock(&mut);
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

struct serial_task : public task {
    bool joined;
    serial_task(): joined(false) {}
    ~serial_task() {
        info = 0;
    }

    virtual void start() { }

    virtual void join() {
        if (joined)
            return;
        if(!info.valid())
            return;
		smart_ptr<search_info> hold = info->self;
        info->self=0;
        if(pfunc == search_f)
            info->result = search(info.ptr());
        else if(pfunc == search_ab_f)
            info->result = search_ab(info.ptr());
        else if(pfunc == strike_f)
            info->result = search_ab(info.ptr());
        else if(pfunc == qeval_f)
            info->result = qeval(info.ptr());
        else
            abort();
        info->set_done_serial();
        joined = true;
    }
};
class pcounter {
    int count, max_count;
    Mutex mut;
public:
    pcounter() : count(0), max_count(0) {
    }
    pcounter(const pcounter& pc) : count(pc.count), max_count(pc.max_count) {
    }
    void set_max(int n) {
        count = max_count = n;
    }
    int add(int n) {
        #ifndef HPX_SUPPORT
        ScopedLock s(mut);
        //int old = count;
        count += n;
        assert(count <= max_count);
        //if(old == 0 && count > 0)
            //pthread_cond_broadcast(&cond);
        int m = count;
        return m;
        #else
        return n;
        #endif
    }
    void wait_dec() {
        /*
        pthread_mutex_lock(&mut);
        while(count == 0)
            pthread_cond_wait(&cond,&mut);
        pthread_mutex_unlock(&mut);
        */
        assert(0);
    }
    bool dec() {
        #ifndef HPX_SUPPORT
        ScopedLock s(mut);
        bool ret;
        if(count > 0) {
            ret = true;
            count--;
        } else {
            ret = false;
        }
        return ret;
        #else
        return true;
        #endif
    }
};
extern std::vector<pcounter> mpi_task_array;
struct pthread_task : public task {
    pthread_mutex_t mut;
    //pthread_attr_t attr;
    bool joined;
    pthread_task() : joined(true) {
        pthread_mutex_init(&mut,NULL);
    }
    ~pthread_task() {
        info = 0;
    }
    virtual void start() {
        joined = false;
        info->set_parallel();
        assert(info.valid());
        info->self = info;
        Threader th;
        if(pfunc == search_f)
            th.create(search_pt,info.ptr());
        else if(pfunc == search_ab_f)
            th.create(search_ab_pt,info.ptr());
        else if(pfunc == strike_f)
            th.create(strike,info.ptr());
        else if(pfunc == qeval_f)
            th.create(qeval_pt,info.ptr());
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

};

#ifdef HPX_SUPPORT
#include "hpx_support.hpp"
#include <hpx/include/iostreams.hpp>
#include <time.h>
struct hpx_task : public task {
    hpx::lcos::future<score_t> result;
    hpx_task()  {
    }

    virtual void start();

    virtual void join() {
        info->result = result.get();
        info->set_done_serial();
    }
};
#endif

#endif
