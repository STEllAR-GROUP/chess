#ifndef LOG_BOARD_HPP
#define LOG_BOARD_HPP

#include <ostream>
#include "parallel.hpp"
#include "defs.hpp"
#include "node.hpp"
#include "board.hpp"

void log_board(const node_t& board,std::ostream& out);

#endif
