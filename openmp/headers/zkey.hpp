#ifndef ZKEY_HPP
#define ZKEY_HPP
#include "hash.hpp"
#include "score.hpp"
#include "node.hpp"
#include <math.h>
#include <pthread.h>

const int table_size = 4096;

struct zkey_t {
  score_t score;
  pthread_mutex_t mut;
  node_t board;
  int depth;
  zkey_t() : depth(-1) {
    pthread_mutex_init(&mut,NULL);
  }
};

extern zkey_t transposition_table[table_size];

bool get_transposition_value(const node_t& board,score_t& val);

inline void set_transposition_value(const node_t& board,score_t val);

#endif
