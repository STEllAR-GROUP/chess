#ifndef MAIN_H
#define MAIN_H
#include <iostream>
#include <string>

#include "node.hpp"
#include "defs.hpp"
#include "data.hpp"
#include "search.hpp"
#include "book.hpp"

int main(int argc, char *argv[]);
int parse_move(std::vector<gen_t>& workq, const char *s);
char *move_str(move_bytes m);
void print_board(node_t& board, FILE *stream);
void xboard();
int print_result(std::vector<gen_t>& workq, node_t& board);
void bench();

#endif
