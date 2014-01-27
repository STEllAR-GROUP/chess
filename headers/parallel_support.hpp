////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2011 Steve Brandt and Philip LeBlanc
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file BOOST_LICENSE_1_0.rst or copy at http://www.boost.org/LICENSE_1_0.txt)
////////////////////////////////////////////////////////////////////////////////
#ifndef PARALLEL_SUPPORT_HPP
#define PARALLEL_SUPPORT_HPP

#include <sys/time.h>
#include "node.hpp"
#include "score.hpp"
#include "chess_move.hpp"
#include <boost/atomic.hpp>
#include "parallel.hpp"
#include "smart_ptr.hpp"

extern bool par_enabled;
int chx_threads_per_proc();
extern pthread_attr_t pth_attr;

struct task;

typedef void *(*pthread_func_t)(void*);

void *search_pt(void *);
void *search_ab_pt(void *);
void *qeval_pt(void *);

struct search_info {
private:
    boost::atomic<bool>  abort_flag_;
    boost::atomic<bool> *abort_flag;
public:
    bool get_abort() { return *abort_flag; }
    void set_abort(bool b) { *abort_flag = b; }
    void set_abort_ref(search_info *s) {
        abort_flag = s->abort_flag;
    }
    // self-reference used
    // to delay cleanup
    smart_ptr<search_info> self;
    node_t board;
    bool par_done;
    chess_move mv;
    score_t result;
    int depth;
    int incr;
    score_t alpha;
    score_t beta;

    search_info(const node_t& board_) : abort_flag_(false), abort_flag(&abort_flag_), board(board_),
            result(bad_min_score) {
    }

    search_info() : abort_flag_(false), abort_flag(&abort_flag_) {
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

enum pfunc_v { no_f, search_f, search_ab_f, qeval_f };

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
	    dtor<search_info> hold = info;
        if(pfunc == search_f)
            info->result = search(info.ptr());
        else if(pfunc == search_ab_f)
            info->result = search_ab(info.ptr());
        else if(pfunc == qeval_f)
            info->result = qeval(info.ptr());
        else
            abort();
        joined = true;
    }
};
class pcounter {
    boost::atomic<int> count;
    int max_count;
public:
    pcounter() : count(0), max_count(0) {
    }
    pcounter(const pcounter& pc) : count(pc.count.load()), max_count(pc.max_count) {
    }
    void set_max(int n) {
        count = max_count = n;
    }
    int add(int n) {
        count += n;
        assert(count <= max_count);
        int m = count;
        return m;
    }
    int dec() {
        while(true) {
            int expected = count.load();
            if(expected == 0)
                return 0;
            int desired = expected - 1;
            if(count.compare_exchange_strong(expected,desired))
                return desired+1;
        }
    }
};
extern pcounter task_counter;
struct pthread_task : public task {
    bool joined;
    Threader th;
    pthread_task() : joined(true) {}
    ~pthread_task() {
        info = 0;
    }
    virtual void start() {
        joined = false;
        assert(info.valid());
        info->self = info;
        if(pfunc == search_f)
            th.create(search_pt,info.ptr());
        else if(pfunc == search_ab_f)
            th.create(search_ab_pt,info.ptr());
        else if(pfunc == qeval_f)
            th.create(qeval_pt,info.ptr());
        else
            abort();
    }
    virtual void join() {
        if(joined)
            return;
        th.join();
        joined = true;
    }

};

#ifdef HPX_SUPPORT
#include "hpx_support.hpp"
#include <hpx/include/iostreams.hpp>
#include <time.h>
struct hpx_task : public task {
    hpx::lcos::shared_future<score_t> result;
    hpx_task()  {
    }

    virtual void start();

    virtual void join() {
        info->result = result.get();
    }
};
#endif

#endif
