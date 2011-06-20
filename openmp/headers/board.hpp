#ifndef BOARD_H
#define BOARD_H
#include <iostream>
#include "node.hpp"
#include "defs.hpp"
#include "data.hpp"
#include "hash.hpp"

void init_board(node_t& board);
void init_hash();
hash_t hash_rand();
hash_t set_hash(node_t& board);
bool in_check(const node_t& board, int s);
bool attack(const node_t& board, int sq, int s);
void gen(std::vector<move>& workq, const node_t& board);
void gen_caps(std::vector<move>& workq, const node_t& board);
void gen_push(std::vector<move>& workq, const node_t& board, int from, int to, int bits);
void gen_promote(std::vector<move>& workq, int from, int to, int bits);
bool makemove(node_t& board, const move_bytes m);
void takeback(node_t& board);
bool board_equals(const node_t& b1,const node_t& b2);
#endif
