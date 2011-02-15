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
bool in_check(const node_t& board, int s);
bool attack(const node_t& board, int sq, int s);
void gen(std::vector<move>& workq, const node_t& board);
void gen_caps(std::vector<move>& workq, const node_t& board);
void gen_push(std::vector<move>& workq, const node_t& board, int from, int to, int bits);
void gen_promote(std::vector<move>& workq, int from, int to, int bits);
bool makemove(node_t& board, move_bytes m);
void takeback(node_t& board);
#endif
