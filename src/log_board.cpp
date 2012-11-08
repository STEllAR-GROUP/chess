#include <ostream>
#include <iostream>
#include "log_board.hpp"
#include "timer.hpp"
#include "defs.hpp"

void log_board(const node_t& board,std::ostream& out)
{
  int i;

  out<<timer.elapsed()<<std::endl;

  for (i = 0; i < 64; ++i) {
    switch (board.color[i]) {
      case EMPTY:
        out << " .";
        break;
      case LIGHT:
        out << " " << piece_char[(size_t)board.piece[i]];
        break;
      case DARK:
        char ch = (piece_char[(size_t)board.piece[i]] + ('a' - 'A'));
        out << " " << ch;
        break;
    }
    if ((i + 1) % 8 == 0 && i != 63)
        out<<std::endl;
  }
  out<<std::endl;
}
