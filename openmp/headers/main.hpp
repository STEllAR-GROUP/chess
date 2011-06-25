////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2011 Steve Brandt and Philip LeBlanc
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file BOOST_LICENSE_1_0.rst or copy at http://www.boost.org/LICENSE_1_0.txt)
////////////////////////////////////////////////////////////////////////////////
#ifndef MAIN_H
#define MAIN_H
#include <iostream>
#include <string>

#include <sys/timeb.h>
#include "node.hpp"
#include "defs.hpp"
#include "data.hpp"
#include "search.hpp"

void shutdown();
int parseArgs(int, char**);
bool parseIni(const char * filename);
int main(int argc, char *argv[]);
int parse_move(std::vector<move>& workq, const char *s);
char *move_str(move_bytes m);
void print_board(const node_t& board, std::ostream& out);
int print_result(std::vector<move>& workq, node_t& board);
void start_benchmark(std::string filename, int ply_level, int num_runs,bool parallel);
int get_ms();
std::string get_log_name();

#endif
