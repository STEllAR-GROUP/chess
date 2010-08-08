#ifndef SEARCH_H
#define SEARCH_H

#include <stdio.h>
#include <string.h>
#include "node.hpp"
#include "defs.hpp"
#include "data.hpp"
#include "main.hpp"
#include "board.hpp"
#include "hash_table.hpp"
#include "eval.hpp"

int think(std::vector<gen_t>& workq, node_t& board, int out);
//int search(node_t& board, int alpha, int beta, int depth);
int search(node_t board, int alpha, int beta, int depth);
int quiesce(node_t& board, int alpha, int beta);
int reps(node_t& board);
void sort_pv(std::vector<gen_t>& workq, node_t& board);
void sort(std::vector<gen_t>& workq);
#endif
