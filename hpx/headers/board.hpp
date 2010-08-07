#ifndef BOARD_H
#define BOARD_H
#include <iostream>
#include "node.hpp"
#include "defs.hpp"
#include "data.hpp"

void init_board(node_t& board);
void init_hash();
int hash_rand();
int set_hash(node_t& board);
BOOL in_check(node_t& board, int s);
BOOL attack(node_t& board, int sq, int s);
void gen(std::vector<gen_t>& workq, node_t& board);
void gen_caps(std::vector<gen_t>& workq, node_t& board);
void gen_push(std::vector<gen_t>& workq, node_t& board, int from, int to, int bits);
void gen_promote(std::vector<gen_t>& workq, int from, int to, int bits);
BOOL makemove(node_t& board, move_bytes m);
void takeback(node_t& board);
#endif
