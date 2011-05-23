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
#include "smart_ptr.hpp"
#include "hash.hpp"
#include "score.hpp"

#define DONE "DONE"

struct search_info {
    score_t result;
    int depth;
    score_t alpha;
    score_t beta;
    node_t board;
    search_info(const node_t& board_) : board(board_) {}
    search_info() {}
};

typedef void *(*pthread_func_t)(void*);

struct task {
    pthread_func_t pfunc;
    smart_ptr<search_info> info;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    bool done;
    task() : done(true) {
        pthread_mutex_init(&lock,NULL);
        pthread_cond_init(&cond,NULL);
    }
    void start() {
        pthread_mutex_lock(&lock);
        done = false;
        pthread_mutex_unlock(&lock);
    }
    void finish() {
        pthread_mutex_lock(&lock);
        done = true;
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&lock);
    }
    void join() {
        pthread_mutex_lock(&lock);
        while(!done)
            pthread_cond_wait(&cond,&lock);
        pthread_mutex_unlock(&lock);
    }
};

int think(node_t& board);
score_t mtdf(const node_t& board,score_t f,int depth);
score_t search(const node_t& board, int depth);
void *search_pt(void *);
score_t search_ab(const node_t& board, int depth, score_t alpha, score_t beta);
void *search_ab_pt(void *);
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
        pthread_create(&th,NULL,run_worker,this);
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

/**
 Match the number of worker threads to the number of buckets.
 */
extern worker workers[bucket_size];

#endif
