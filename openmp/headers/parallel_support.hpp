#ifndef PARALLEL_SUPPORT_HPP
#define PARALLEL_SUPPORT_HPP

#include "smart_ptr.hpp"
#include "node.hpp"

extern bool par_enabled;
int chx_threads_per_proc();

typedef void *(*pthread_func_t)(void*);

void *search_pt(void *);
void *search_ab_pt(void *);
void *strike(void *);
void *qeval_pt(void *);

struct search_info {
    score_t result;
    int depth;
    int incr;
    score_t alpha;
    score_t beta;
    node_t board;
    move mv;
    search_info(const node_t& board_) : board(board_) {}
    search_info() {}
};

enum pfunc_v { no_f, search_f, search_ab_f, strike_f, qeval_f };

struct task {
    smart_ptr<search_info> info;
    //pthread_func_t pfunc;
    pfunc_v pfunc;
    task() : pfunc(no_f) {}
    virtual ~task() {}

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
};
struct pcounter {
    int count;
    pthread_mutex_t mut;
    pcounter() : count(0) {
        pthread_mutex_init(&mut,NULL);
    }
    pcounter(int n) : count(n) {
        pthread_mutex_init(&mut,NULL);
    }
    int add(int n) {
        pthread_mutex_lock(&mut);
        count += n;
        int m = count;
        pthread_mutex_unlock(&mut);
        return m;
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
    //pthread_mutex_t mut;
    //pthread_attr_t attr;
    bool joined;
    pthread_task() : joined(true) {
        //pthread_mutex_init(&mut,NULL);
    }
    virtual ~pthread_task() {
        join();
    }
    virtual void start() {
        //pthread_mutex_lock(&mut);
        joined = false;
        //pthread_mutex_unlock(&mut);
        //pthread_attr_init(&attr);
        //pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_JOINABLE);
        //pthread_create(&thread,&attr,pfunc,info.ptr());
        if(pfunc == search_f)
            pthread_create(&thread,NULL,search_pt,info.ptr());
        else if(pfunc == search_ab_f)
            pthread_create(&thread,NULL,search_ab_pt,info.ptr());
        else if(pfunc == strike_f)
            pthread_create(&thread,NULL,strike,info.ptr());
        else if(pfunc == qeval_f)
            pthread_create(&thread,NULL,qeval_pt,info.ptr());
        else
            abort();
    }
    virtual void join() {
        if(joined)
            return;
        joined = true;
        pthread_join(thread,NULL);
        mpi_task_array[0].add(1);
    }
};
#endif
