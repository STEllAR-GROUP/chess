#include <ostream>
#include <iostream>
#include "print_board.hpp"
#include "defs.hpp"

Mutex mtx;
std::ostream& out=std::cout;

void print_board(const node_t& board)
{
  ScopedLock lock(mtx);
  int i;

  //out << std::endl << "8 ";
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
    //  out << std::endl << 7 - ROW(i) << " ";
        out<<std::endl;
  }
  //out << std::endl << std::endl << "   a b c d e f g h" << std::endl << std::endl;
  out<<std::endl;
}
