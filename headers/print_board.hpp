#ifndef PRINT_BOARD_HPP
#define PRINT_BOARD_HPP

#include <ostream>
#include "parallel.hpp"
#include "defs.hpp"
#include "node.hpp"
#include "board.hpp"

extern Mutex mtx;
extern std::ostream& out;

void print_board(const node_t& board);

#endif
