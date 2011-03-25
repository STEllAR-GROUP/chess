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

struct search_info {
    int result;
    int depth;
    node_t board;
    pthread_t th;
    bool parallel;
    bool contin;
};

int think(node_t& board);
int search(const node_t& board, int depth);
void *search_pt(void *);
int search_ab(const node_t& board, int depth, int alpha, int beta);
int reps(const node_t& board);
bool compare_moves(move a, move b);
void sort_pv(std::vector<move>& workq, int ply);

struct minimax_t
{
  int result;
  node_t* board;
  int depth;
  minimax_t(node_t* b, int d) : result(0), depth(d) { board = b; }
}; 

#endif
