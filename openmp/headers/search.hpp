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

int think(node_t& board,bool parallel);
score_t mtdf(const node_t& board,score_t f,int depth);
score_t search(const node_t& board, int depth);
score_t search_ab(const node_t& board, int depth, score_t alpha, score_t beta);
score_t qeval(const node_t& board,const score_t& lower,const score_t& upper);
int reps(const node_t& board);
bool compare_moves(move a, move b);
void sort_pv(std::vector<move>& workq, int ply);
extern bool chx_abort;

#endif
