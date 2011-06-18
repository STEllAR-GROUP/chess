#ifndef SEARCH_H
#define SEARCH_H

#include <stdio.h>
#include <string.h>
#include "node.hpp"
#include "defs.hpp"
#include "data.hpp"
#include "main.hpp"
#include "board.hpp"
#include "eval.hpp"
#include <pthread.h>
#include <list>
#include <map>
#include "smart_ptr.hpp"
#include "hash.hpp"
#include "score.hpp"
#ifdef MPI_SUPPORT
#include "mpi.h"
#endif

#define DONE "DONE"

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

typedef void *(*pthread_func_t)(void*);
void *search_pt(void *);
void *search_ab_pt(void *);
void *strike(void *);
void *qeval_pt(void *);

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
struct worker_result {
    pthread_mutex_t mut;
    pthread_cond_t cond;
    bool has_result;
    hash_t hash;
    int depth;
    score_t score;
    worker_result() : has_result(false), depth(-1) {
        pthread_mutex_init(&mut,NULL);
        pthread_cond_init(&cond,NULL);
    }
    void set_result(score_t s) {
        pthread_mutex_lock(&mut);
        assert(!has_result);
        score = s;
        has_result = true;
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mut);
    }
    void clear_result() {
        /*
        pthread_mutex_lock(&mut);
        has_result = false;
        pthread_mutex_unlock(&mut);
        */
    }
    score_t get_result() {
        pthread_mutex_lock(&mut);
        assert(mpi_rank == 0);
        if(!has_result) {
            pthread_cond_wait(&cond,&mut);
        }
        assert(has_result);
        score_t t = score;
        pthread_mutex_unlock(&mut);
        return t;
    }
};
const int worker_result_size = 1000;
extern worker_result results[worker_result_size];
int result_alloc(hash_t h,int d);
const int WORK_ASSIGN_MESSAGE = 1, WORK_COMPLETED = 2, WORK_SUPPLEMENT = 3;
void int_from_chars(int& i1,char c1,char c2,char c3,char c4);
void int_to_chars(int i1,char& c1,char& c2,char& c3,char& c4);

const int mpi_ints = 2*16+13+50;
struct mpi_task : public task {
    int windex;
    int dest;
    bool joined;
    mpi_task(int d) : joined(false), dest(d), windex(-1) {
        assert(d != 0);
    }
    virtual ~mpi_task() {
        join();
    }
    int mpi_data[mpi_ints];
    virtual void start() {
#ifdef MPI_SUPPORT
        //int mpi_data[mpi_ints];
        int n = 0;
        for(int i=0;i<16;i++) {
            int v;
            int_from_chars(v,info->board.color[4*i],info->board.color[4*i+1],
                info->board.color[4*i+2],info->board.color[4*i+3]);
            mpi_data[n++] = v;
        }
        for(int i=0;i<16;i++) {
            int v;
            int_from_chars(v,info->board.piece[4*i],info->board.piece[4*i+1],
                info->board.piece[4*i+2],info->board.piece[4*i+3]);
            mpi_data[n++] = v;
        }
        mpi_data[n++] = info->board.hash;
        assert(info->board.depth >=0 && info->board.depth <= 7);
        mpi_data[n++] = info->board.depth;
        mpi_data[n++] = info->board.side;
        mpi_data[n++] = info->board.castle;
        mpi_data[n++] = info->board.ep;
        mpi_data[n++] = info->board.ply;
        mpi_data[n++] = info->board.hply;
        mpi_data[n++] = info->board.fifty;
        mpi_data[n++] = info->alpha;
        mpi_data[n++] = info->beta;
        mpi_data[n++] = pfunc;
        if(windex == -1)
            windex = result_alloc(info->board.hash,info->board.depth);
        assert(windex != -1);
        mpi_data[n++] = windex;
        mpi_data[n++] = info->board.hist_dat.size();
        for(int i=0;i<info->board.hist_dat.size();i++) {
            mpi_data[n++] = info->board.hist_dat[i];
        }

        //results[info->board.hash][info->depth].clear_result();

        assert(info->board.depth >= 0);
        MPI_Send(mpi_data,mpi_ints,MPI_INT,
            dest,WORK_ASSIGN_MESSAGE,MPI_COMM_WORLD);
        /*
        int fifty_n = info->board.hply;
        assert(fifty_n >=0 && fifty_n < 50);
        if(fifty_n > 0) {
            for(int i=0;i<fifty_n;i++) {
                fifty_array[i] = info->board.hist_dat[i];
            }
            MPI_Send(fifty_array,fifty_n,MPI_INT,
                dest,WORK_SUPPLEMENT,MPI_COMM_WORLD);
        }
        */
#endif
    }
    virtual void join() {
        if(joined)
            return;
        joined = true;
        //info->result = results[info->board.hash][info->depth].get_result();
        info->result = results[windex].get_result();
        score_t s = info->result;
        results[windex].depth = -1;
        mpi_task_array[dest].add(1);
    }
};

int think(node_t& board,bool parallel);
score_t mtdf(const node_t& board,score_t f,int depth);
score_t search(const node_t& board, int depth);
score_t search_ab(const node_t& board, int depth, score_t alpha, score_t beta);
int reps(const node_t& board);
bool compare_moves(move a, move b);
void sort_pv(std::vector<move>& workq, int ply);

void *run_worker(void*);

/** We use pthreads to set up a an array of workers that process
    tasks inserted onto a queue. This avoids the problem of creating
    too many pthreads and is also a first step toward an MPI version
    (which also has a fixed number of workers).*/
struct worker {
    pthread_t th;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    std::list<smart_ptr<task> >  queue;
    /** initialize a worker thread */
    worker() {
        pthread_mutex_init(&lock,NULL);
        pthread_cond_init(&cond,NULL);
        //pthread_create(&th,NULL,run_worker,this);
    }
    /** Thread safe addition of a task to a work queue */
    void add(smart_ptr<task> t) {
        t->start();
        pthread_mutex_lock(&lock);
        queue.push_back(t);
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&lock);
    }
    /** Thread safe removal of a task from a work queue. */
    smart_ptr<task> remove() {
        pthread_mutex_lock(&lock);
        while(queue.size()==0)
            pthread_cond_wait(&cond,&lock);
        smart_ptr<task> t = queue.back();
        queue.pop_back();
        pthread_mutex_unlock(&lock);
        return t;
    }
};

void *mpi_worker(void *);
extern bool chx_abort;

#endif
