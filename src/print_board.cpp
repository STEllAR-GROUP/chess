#include <ostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include "parallel.hpp"
#include "defs.hpp"
#include "node.hpp"
#include "board.hpp"
#include "print_board.hpp"

inline void plain_print_board(const node_t& board, std::ostream& out)
{
  int i;

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
      out << std::endl;
  }
  out << std::endl<<std::endl;
}

#ifdef HPX_SUPPORT

std::vector<std::ofstream*> board_printer::streams;

void board_printer::init(){
    const int num_threads=hpx::get_os_thread_count();
    streams.resize(num_threads);
    for(int i=0;i<num_threads;i++){
        std::stringstream fname;
        fname<<"os_thread_"<<i<<".chx_log";
        streams[i]=new std::ofstream;
        streams[i]->open(fname.str());
    }
}

void board_printer::finalize(){
    for(unsigned int i=0;i<streams.size();i++){
        streams[i]->close();
        delete streams[i];
    }
}

void board_printer::print_board(const node_t& board){
    int this_thread=hpx::get_worker_thread_num();
    plain_print_board(board,*(std::ostream*)streams[this_thread]);
}

#else

std::ofstream* board_printer::stream;
Mutex board_printer::lock;

void board_printer::init(){
    stream=new std::ofstream;
    stream->open("os_thread_all.chx_log");
}

void board_printer::finalize(){
    stream->close();
    delete stream;
}

void board_printer::print_board(const node_t& board){
    ScopedLock printer(lock);
    plain_print_board(board,*(std::ostream*)stream);
}

#endif
