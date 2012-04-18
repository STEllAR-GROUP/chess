////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2011 Steve Brandt and Philip LeBlanc
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file BOOST_LICENSE_1_0.rst or copy at http://www.boost.org/LICENSE_1_0.txt)
////////////////////////////////////////////////////////////////////////////////
#ifndef BOARD_H
#define BOARD_H
#include <iostream>
#include "node.hpp"
#include "defs.hpp"
#include "data.hpp"
#include "hash.hpp"
#include "chess_move.hpp"

void init_board(node_t& board);
void init_hash();
hash_t hash_rand();
hash_t set_hash(node_t& board);
hash_t update_hash(node_t& board, chess_move& m);
bool in_check(const node_t& board, int s);
bool attack(const node_t& board, int sq, int s);
void gen(std::vector<chess_move>& workq, const node_t& board);
void gen_caps(std::vector<chess_move>& workq, const node_t& board);
void gen_push(std::vector<chess_move>& workq, const node_t& board, int from, int to, int bits);
void gen_promote(std::vector<chess_move>& workq, int from, int to, int bits);
bool makemove(node_t& board, chess_move& m);
void takeback(node_t& board);
bool board_equals(const node_t& b1,const node_t& b2);
#endif
