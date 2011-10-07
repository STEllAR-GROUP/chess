////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2011 Steve Brandt and Philip LeBlanc
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file BOOST_LICENSE_1_0.rst or copy at http://www.boost.org/LICENSE_1_0.txt)
////////////////////////////////////////////////////////////////////////////////
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
#include "parallel_support.hpp"

int think(node_t& board,bool parallel);
score_t mtdf(const node_t& board,score_t f,int depth, task *parent);
score_t search(const node_t& board, int depth, task *parent);
score_t search_ab(const node_t& board, int depth, score_t alpha, score_t beta, task *parent);
score_t qeval(const node_t& board,const score_t& lower,const score_t& upper, task *parent);
score_t multistrike(const node_t& board,score_t f,int depth, task *parent);
int reps(const node_t& board);
bool compare_moves(move a, move b);
void sort_pv(std::vector<move>& workq, int ply);

#endif
