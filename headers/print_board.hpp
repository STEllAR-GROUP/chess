#ifndef PRINT_BOARD_HPP
#define PRINT_BOARD_HPP

#include <ostream>
#include <vector>
#include "node.hpp"

#ifdef HPX_SUPPORT
class board_printer{
    private:
        static std::vector<std::ofstream*> streams;
    public:
        static void init();
        static void finalize();
        static void print_board(const node_t& board);
};
#else
#include "parallel.hpp"
class board_printer{
    private:
        static std::ofstream* stream;
        static Mutex lock;
    public:
        static void init();
        static void finalize();
        static void print_board(const node_t& board);
};
#endif

#endif
