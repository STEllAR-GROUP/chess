#ifndef HASH_TABLE_H
#define HASH_TABLE_H


#include <string.h>
#include "node.hpp"
#include "defs.hpp"
#include "data.hpp"

int ProbeHash(node_t& board, int depth, int alpha, int beta);
void RecordHash(node_t& board, int depth, int val, int hashf, move m);

#endif
