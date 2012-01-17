////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2011 Steve Brandt and Philip LeBlanc
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file BOOST_LICENSE_1_0.rst or copy at http://www.boost.org/LICENSE_1_0.txt)
////////////////////////////////////////////////////////////////////////////////
#ifndef ZKEY_HPP
#define ZKEY_HPP
#include "hash.hpp"
#include "score.hpp"
#include "node.hpp"
#include <math.h>
#include <pthread.h>

const int table_size = 4096;

struct zkey_t {
  score_t lower, upper;
  pthread_mutex_t mut;
  node_t board;
  int depth;
  zkey_t() : depth(-1) {
    pthread_mutex_init(&mut,NULL);
  }
};

extern zkey_t transposition_table[table_size];

bool get_transposition_value(const node_t& board,score_t& lower,score_t& upper);

void set_transposition_value(const node_t& board,score_t lower,score_t upper);

#endif
