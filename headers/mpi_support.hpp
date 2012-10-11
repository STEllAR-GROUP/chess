////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2011 Steve Brandt and Philip LeBlanc
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file BOOST_LICENSE_1_0.rst or copy at http://www.boost.org/LICENSE_1_0.txt)
////////////////////////////////////////////////////////////////////////////////
#ifndef MPI_SUPPORT_HPP
#define MPI_SUPPORT_HPP
#include "score.hpp"
#include "parallel_support.hpp"
#ifdef MPI_SUPPORT
#include <mpi.h>
#endif

// Number of ints needed to
// send board over MPI
const int mpi_ints = 2*16+13+50;

// if MPI is not turned on, mpi_rank will
// be zero and mpi_size will be one.
extern int mpi_rank, mpi_size;

void *mpi_worker(void *);

const int worker_result_size = 1000;
const int WORK_ASSIGN_MESSAGE = 1, WORK_COMPLETED = 2, WORK_SUPPLEMENT = 3;

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

extern worker_result results[worker_result_size];

struct mpi_task : public task {
  bool joined;
  int dest;
  int windex;
  mpi_task(int d) : joined(false), dest(d), windex(-1) {
    assert(d != 0);
  }
  virtual ~mpi_task() {
    join();
    info = 0;
  }
  int mpi_data[mpi_ints];
  virtual void start();
  virtual void join() {
    if(joined)
      return;
    joined = true;
    info->result = results[windex].get_result();
    //score_t s = info->result;
    results[windex].depth = -1;
    mpi_task_array[dest].add(1);
  }
  virtual void abort_search() {}

  virtual bool check_abort() { return false; }
};
#endif
extern worker_result results[worker_result_size];
