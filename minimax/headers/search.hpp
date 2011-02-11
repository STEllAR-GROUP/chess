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

int think(node_t& board);
int search(node_t board, int depth);
int reps(node_t& board);
bool compare_moves(move a, move b);
#endif
