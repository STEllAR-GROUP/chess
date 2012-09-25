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
extern bool chx_abort;

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
    
    smart_ptr<task> this_task;
    
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
    search_info(const node_t& board_) : board(board_), par_done(true),
            result(bad_min_score) {
        pthread_mutex_init(&mut,NULL);
        pthread_cond_init(&cond,NULL);
        this_task = 0;
    }

    search_info() {
    }

    ~search_info() {
    }
};

score_t search(search_info*);
score_t search_ab(search_info*);
score_t qeval(search_info*);

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

inline void search_info::wait_for_done() {
	smart_ptr<task> ttask = this_task;
    pthread_mutex_lock(&mut);
    while(!par_done) {
        if(this_task.valid() && this_task->check_abort())
            break;
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
        parent_task = 0;
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
        if(pfunc != search_f && info->result >= info->beta) {
			smart_ptr<task> hold_task = info->this_task;
            if (info->this_task.valid())
                info->this_task->abort_search();
        }
        joined = true;
    }

    virtual void abort_search() {}

    virtual bool check_abort() { 
        return false;
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
        ScopedLock s(mut);
        //int old = count;
        count += n;
        assert(count <= max_count);
        //if(old == 0 && count > 0)
            //pthread_cond_broadcast(&cond);
        int m = count;
        return m;
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
        ScopedLock s(mut);
        bool ret;
        if(count > 0) {
            ret = true;
            count--;
        } else {
            ret = false;
        }
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
		smart_ptr<task> hold = parent_task;
        pthread_mutex_lock(&mut);
		if(parent_task.valid())
        	parent_task->abort_search_parent();
        pthread_mutex_unlock(&mut);
    }


    virtual bool check_abort() {
        return false;
        pthread_mutex_lock(&mut);
        bool ret=abort_flag;
        smart_ptr<task> hold=parent_task;
        pthread_mutex_unlock(&mut);
        if(ret){
            return true;
        }
        if (parent_task.valid())
            ret=parent_task->check_abort();
        return ret;
    }
};

#ifdef HPX_SUPPORT
#include "hpx_support.hpp"
#include <time.h>
struct hpx_task : public task {
    std::vector<hpx::naming::id_type> all_localities;
    hpx::lcos::future<score_t> result;
    hpx_task()  {
        all_localities = hpx::find_all_localities();
        srand(time(NULL));
    }

    virtual void start();

    virtual void join() {
        info->result = result.get();
        info->set_done_serial();
    }

    virtual void abort_search() {
    }

    virtual bool check_abort() {
        return false;
    }

    virtual hpx::naming::id_type get_random_locality();
};
#endif

#endif
