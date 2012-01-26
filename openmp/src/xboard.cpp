/* xboard() is a substitute for chx_main() that is XBoard
   and WinBoard compatible. See the following page for details:
http://home.hccnet.nl/h.g.muller/engine-intf.html */

#include "search.hpp"
#include <signal.h>

void xboard()
{
    int computer_side;
    std::string command;
    int m;
    int post = 0;
    std::vector<move> workq;

    signal(SIGINT, SIG_IGN);
    
    
    std::cout << std::endl;
    std::cout.flush();

    node_t board;
    init_hash();
    init_board(board);
    computer_side = EMPTY;

    gen(workq, board);

    for (;;) {
        command.clear();
        if (board.side == computer_side) {
            think(board, false);
            if (move_to_make.u == 0) {
                computer_side = EMPTY;
                continue;
            }
            std::cout << "move " << move_str(move_to_make.b) << std::endl;
            std::cout.flush();
            makemove(board, move_to_make.b);
            board.ply = 0;
            workq.clear();
            gen(workq, board);
            print_result(workq, board);

            move_to_make.u = 0;
            continue;
        }
        std::cin >> command;

        if (command.empty())
            continue;

        if (command == "xboard")
            continue;

        if (command == "new") {
            computer_side = EMPTY;
            init_board(board);
            workq.clear();
            gen(workq, board);
            continue;
        }
        if (command == "quit")
            return;
        if (command == "force") {
            computer_side = EMPTY;
            continue;
        }
        if (command == "white") {
            board.side = LIGHT;
            workq.clear();
            gen(workq, board);
            computer_side = DARK;
            continue;
        }
        if (command == "black") {
            board.side = DARK;
            workq.clear();
            gen(workq, board);
            computer_side = LIGHT;
            continue;
        }
        if (command == "otim") {
            continue;
        }
        if (command == "go") {
            computer_side = board.side;
            continue;
        }
        if (command == "hint") {
            think(board, false);
            if (move_to_make.u == 0)
                continue;
            std::cout << "Hint: " << move_str(move_to_make.b) << std::endl;
            continue;
        }

        m = parse_move(workq, command.c_str());
        move mov;
        mov.u = m;
        node_t newboard = board;
        if (m == -1 || !makemove(newboard, mov.b))
            std::cerr << "Error (unknown command): " << command << std::endl;
        else {
            makemove(board, mov.b);
            board.ply = 0;
            workq.clear();
            gen(workq, board);
            print_result(workq, board);
        }
    }
}