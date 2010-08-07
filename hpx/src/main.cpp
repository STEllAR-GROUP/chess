/*
 *  MAIN.C
 */

#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <boost/program_options.hpp>
#include "main.hpp"
#include <signal.h>

int parseArgs(boost::program_options::variables_map&);

using namespace hpx;
namespace po = boost::program_options;

/* main() is basically an infinite loop that either calls
   think() when it's the computer's turn to move or prompts
   the user for a command (and deciphers it). */

int auto_move = 0;
int computer_side;

int hpx_main(po::variables_map &vm)
{
    int arguments = parseArgs(vm);

    std::string s;
    int m;

    if (!arguments) {
        std::cout << std::endl;
        std::cout << "HPX Chess (CHX)" << std::endl;
        std::cout << "dev build (feature incomplete)" << std::endl;
        std::cout << "Phillip LeBlanc - CCT" << std::endl;
        std::cout << std::endl;
        std::cout << "\"help\" displays a list of commands." << std::endl;
        std::cout << std::endl;
        computer_side = EMPTY;
    }
    else
    {
        computer_side = LIGHT;
        auto_move = 1;
    }
    node_t board;

    if (table)
        hash_table = new HASHE[hash_table_size];

    init_hash();
    init_board(board);
    open_book();
    //workq_t workq;
    //init_workq(&workq);
    std::vector<gen_t> workq;
    gen(workq, board);

    for (;;) {
        if (board.side == computer_side) {  /* computer's turn */

            /* think about the move and make it */
            think(workq, board, output);
            if (!pv[0][0].u) {
                std::cout << "(no legal moves)" << std::endl;
                computer_side = EMPTY;
                continue;
            }
            if (output)
                std::cout << "Computer's move: " << move_str(pv[0][0].b)
                          << std::endl;
            makemove(board, pv[0][0].b);
            board.ply = 0;

            //flush_workq(&workq);
            workq.clear();
            gen(workq, board);

            if (output)
                print_board(board, stdout);
            if (auto_move)
                auto_move = print_result(workq, board);
            else
                print_result(workq, board);

            continue;
        }

        if (auto_move)
        {
            computer_side = board.side;
            continue;
        }
        if (!auto_move && arguments)
        {
            break;
        }

        /* get user input */
        std::cout << "chx> ";
        std::cin >> s;
        if (s.empty())
            return 0;
        if (s == "go") {
            computer_side = board.side;
            auto_move = 0;
            continue;
        }
        if (s == "auto") {
            computer_side = board.side;
            auto_move = 1;
            auto_move = print_result(workq, board);
            continue;
        }
        if (s == "new") {
            computer_side = EMPTY;
            init_board(board);
            //flush_workq(&workq);
            workq.clear();
            gen(workq, board);
            continue;
        }
        if (s == "wd") {
            std::cin >> depth[LIGHT];
            if (depth[LIGHT] <= 0)
            {
                std::cerr << "Illegal depth given, setting depth to 1." << std::endl;
                depth[LIGHT] = 1;
            }
            continue;
        }
        if (s == "bd") {
            std::cin >> depth[DARK];
            if (depth[DARK] <= 0)
            {
                std::cerr << "Illegal depth given, setting depth to 1." << std::endl;
                depth[DARK] = 1;
            }
            continue;
        }
        if (s == "d") {
            print_board(board, stdout);
            continue;
        }
        if (s == "bench") {
            computer_side = EMPTY;
            bench();
            init_board(board);
            //flush_workq(&workq);
            workq.clear();
            gen(workq, board);
            continue;
        }
        if (s == "o") {
            output ^= 1;
            if (output == 1)
                std::cout << "Output is now on" << std::endl;
            else
                std::cout << "Output is now off" << std::endl;
            continue;
        }
        if ((s == "exit")||(s == "quit")) {
            std::cout << "Thanks for using CHX!" << std::endl;
            break;
        }
        if (s == "xboard") {
            xboard();
            break;
        }
        if (s == "help") {
            std::cout << "go - computer makes a move" << std::endl;
            std::cout << "auto - computer will continue to make moves until game is over" << std::endl;
            std::cout << "new - starts a new game" << std::endl;
            std::cout << "wd n - sets white search depth to n (default 3)" << std::endl;
            std::cout << "bd n - sets black search depth to n (default 3)" << std::endl;
            std::cout << "d - display the board" << std::endl;
            std::cout << "bench - run the built-in benchmark" << std::endl;
            std::cout << "o - toggles engine output on or off (default on)" << std::endl;
            std::cout << "exit - exit the program" << std::endl;
            std::cout << std::endl;
            continue;
        }

        /* maybe the user entered a move? */
        m = parse_move(workq, s.c_str());
        move mov;
        mov.u = m;
        if (m == -1 || !makemove(board, mov.b))
            std::cout << "Illegal move." << std::endl;
        else {
            board.ply = 0;
            //flush_workq(&workq);
            workq.clear();
            gen(workq, board);
            print_result(workq, board);
        }
    }
    close_book();

    hpx_finalize();
    return 0;
}

void ctrlc(int s)
{
    if (!auto_move && computer_side == EMPTY)
    {
        close_book();
        hpx_finalize();
        exit(0);
    }
    auto_move = 0;
    computer_side = EMPTY;
}

int main(int argc, char *argv[])
{

    signal( SIGINT, ctrlc);
    po::options_description
        desc_commandline("Usage: chx [hpx_options] [options]");
    desc_commandline.add_options()
        ("white", po::value<int>(),
            "Sets the depth of the white side to specified number"
            "(default is 3)")
        ("black", po::value<int>(),
            "Sets the depth of the black side to specified number"
            "(default is 3)")
        ("output", "Turns off all engine output")
        ("xboard", "Turns on xboard mode")
        ("notable", "Turns off transposition table")
        ("sizetable", po::value<int>(),
            "Sets the size of the transposition table to a certain number"
            "of megabytes (default is 500)")
    ;

    int retcode = hpx_init(desc_commandline, argc, argv);
    return retcode;
}


/* parse the move s (in coordinate notation) and return the move's
   int value, or -1 if the move is illegal */

int parse_move(std::vector<gen_t>& workq, const char *s)
{
    int from, to, i;

    /* make sure the string looks like a move */
    if (s[0] < 'a' || s[0] > 'h' ||
            s[1] < '0' || s[1] > '9' ||
            s[2] < 'a' || s[2] > 'h' ||
            s[3] < '0' || s[3] > '9')
        return -1;

    from = s[0] - 'a';
    from += 8 * (8 - (s[1] - '0'));
    to = s[2] - 'a';
    to += 8 * (8 - (s[3] - '0'));

    for (int i = 0; i < workq.size(); i++) {
        if (workq[i].m.b.from == from && workq[i].m.b.to == to) {
            if (workq[i].m.b.bits & 32)
                switch (s[4]) {
                    case 'N':
                        if (workq[i].m.b.promote == KNIGHT)
                            return workq[i].m.u;
                    case 'B':
                        if (workq[i].m.b.promote == BISHOP)
                            return workq[i].m.u;
                    case 'R':
                        if (workq[i].m.b.promote == ROOK)
                            return workq[i].m.u;
                    default:
                        return workq[i].m.u;
                }
            return workq[i].m.u;
        }
    }

    /* didn't find the move */
    return -1;
}


/* move_str returns a string with move m in coordinate notation */

char *move_str(move_bytes m)
{
    static char str[6];

    char c;

    if (m.bits & 32) {
        switch (m.promote) {
            case KNIGHT:
                c = 'n';
                break;
            case BISHOP:
                c = 'b';
                break;
            case ROOK:
                c = 'r';
                break;
            default:
                c = 'q';
                break;
        }
        sprintf(str, "%c%d%c%d%c",
                COL(m.from) + 'a',
                8 - ROW(m.from),
                COL(m.to) + 'a',
                8 - ROW(m.to),
                c);
    }
    else
        sprintf(str, "%c%d%c%d",
                COL(m.from) + 'a',
                8 - ROW(m.from),
                COL(m.to) + 'a',
                8 - ROW(m.to));
    return str;
}


/* print_board() prints the board */

void print_board(node_t& board, FILE *stream)
{
    int i;

    fprintf(stream, "\n8 ");
    for (i = 0; i < 64; ++i) {
        switch (board.color[i]) {
            case EMPTY:
                fprintf(stream, " .");
                break;
            case LIGHT:
                fprintf(stream, " %c", piece_char[board.piece[i]]);
                break;
            case DARK:
                fprintf(stream, " %c", piece_char[board.piece[i]] + ('a' - 'A'));
                break;
        }
        if ((i + 1) % 8 == 0 && i != 63)
            fprintf(stream, "\n%d ", 7 - ROW(i));
    }
    fprintf(stream, "\n\n   a b c d e f g h\n\n");
}


/* xboard() is a substitute for main() that is XBoard
   and WinBoard compatible. See the following page for details:
http://www.research.digital.com/SRC/personal/mann/xboard/engine-intf.html */

void xboard()
{
    int computer_side;
    char line[256], command[256];
    int m;
    int post = 0;
    std::vector<gen_t> workq;

    signal(SIGINT, SIG_IGN);
    std::cout << "" << std::endl;
    node_t board;
    init_board(board);
    computer_side = EMPTY;
    for (;;) {
        fflush(stdout);
        if (board.side == computer_side) {
            think(workq, board, post);
            if (!pv[0][0].u) {
                computer_side = EMPTY;
                continue;
            }
            std::cout << "move " << move_str(pv[0][0].b) << std::endl;
            makemove(board, pv[0][0].b);
            board.ply = 0;
            workq.clear();
            gen(workq, board);
            print_result(workq, board);
            continue;
        }
        if (!fgets(line, 256, stdin))
            return;
        if (line[0] == '\n')
            continue;
        sscanf(line, "%s", command);
        if (!strcmp(command, "xboard"))
            continue;
        if (!strcmp(command, "new")) {
            init_board(board);
            workq.clear();
            gen(workq, board);
            computer_side = DARK;
            continue;
        }
        if (!strcmp(command, "quit"))
            return;
        if (!strcmp(command, "force")) {
            computer_side = EMPTY;
            continue;
        }
        if (!strcmp(command, "white")) {
            board.side = LIGHT;
            workq.clear();
            gen(workq, board);
            computer_side = DARK;
            continue;
        }
        if (!strcmp(command, "black")) {
            board.side = DARK;
            workq.clear();
            gen(workq, board);
            computer_side = LIGHT;
            continue;
        }
        if (!strcmp(command, "otim")) {
            continue;
        }
        if (!strcmp(command, "go")) {
            computer_side = board.side;
            continue;
        }
        if (!strcmp(command, "hint")) {
            think(workq, board, 0);
            if (!pv[0][0].u)
                continue;
            std::cout << "Hint: " << move_str(pv[0][0].b) << std::endl;
            continue;
        }
        if (!strcmp(command, "undo")) {
            if (!board.hply)
                continue;
            takeback(board);
            board.ply = 0;
            workq.clear();
            gen(workq, board);
            continue;
        }
        if (!strcmp(command, "remove")) {
            if (board.hply < 2)
                continue;
            takeback(board);
            takeback(board);
            board.ply = 0;
            workq.clear();
            gen(workq, board);
            continue;
        }
        if (!strcmp(command, "post")) {
            post = 2;
            continue;
        }
        if (!strcmp(command, "nopost")) {
            post = 0;
            continue;
        }
        m = parse_move(workq, line);
        move mov;
        mov.u = m;
        if (m == -1 || !makemove(board, mov.b))
            std::cerr << "Error (unknown command): " << command << std::endl;
        else {
            board.ply = 0;
            workq.clear();
            gen(workq, board);
            print_result(workq, board);
        }
    }
}


/* print_result() checks to see if the game is over, and if so,
   prints the result. */

int print_result(std::vector<gen_t>& workq, node_t& board)
{
    int i;

    /* is there a legal move? */
    for (i = 0; i < workq.size() ; ++i) { 
        if (makemove(board, workq[i].m.b)) {
            takeback(board);
            break;
        }
    }
    if (i == workq.size()) {
        if (in_check(board, board.side)) {
            if (board.side == LIGHT)
                std::cout << "0-1 {Black mates}" << std::endl;
            else
                std::cout << "1-0 {White mates}" << std::endl;
        }
        else
            std::cout << "1/2-1/2 {Stalemate}" << std::endl;
        return 0;
    }
    else if (reps(board) == 3)
    {
        std::cout << "1/2-1/2 {Draw by repetition}" << std::endl;
        return 0;
    }
    else if (board.fifty >= 100)
    {
        std::cout << "1/2-1/2 {Draw by fifty move rule}" << std::endl;
        return 0;
    }
    return 1;
}


/* bench: This is a little benchmark code that calculates how many
   nodes per second CHX searches.
   It sets the position to move 17 of Bobby Fischer vs. J. Sherwin,
   New Jersey State Open Championship, 9/2/1957.
   Then it searches five ply three times. It calculates nodes per
   second from the best time. */

int bench_color[64] = {
    6, 1, 1, 6, 6, 1, 1, 6,
    1, 6, 6, 6, 6, 1, 1, 1,
    6, 1, 6, 1, 1, 6, 1, 6,
    6, 6, 6, 1, 6, 6, 0, 6,
    6, 6, 1, 0, 6, 6, 6, 6,
    6, 6, 0, 6, 6, 6, 0, 6,
    0, 0, 0, 6, 6, 0, 0, 0,
    0, 6, 0, 6, 0, 6, 0, 6
};

int bench_piece[64] = {
    6, 3, 2, 6, 6, 3, 5, 6,
    0, 6, 6, 6, 6, 0, 0, 0,
    6, 0, 6, 4, 0, 6, 1, 6,
    6, 6, 6, 1, 6, 6, 1, 6,
    6, 6, 0, 0, 6, 6, 6, 6,
    6, 6, 0, 6, 6, 6, 0, 6,
    0, 0, 4, 6, 6, 0, 2, 0,
    3, 6, 2, 6, 3, 6, 5, 6
};

void bench()
{
    int i;
    int t[3];
    double nps;

    /* setting the position to a non-initial position confuses the opening
       book code. */
    close_book();

    node_t board;

    for (i = 0; i < 64; ++i) {
        board.color[i] = bench_color[i];
        board.piece[i] = bench_piece[i];
    }
    board.side = LIGHT;
    board.castle = 0;
    board.ep = -1;
    board.fifty = 0;
    board.ply = 0;
    board.hply = 0;
    board.hash = set_hash(board);
    print_board(board, stdout);
    depth[LIGHT] = 5;
    depth[DARK] = 5;
    std::vector<gen_t> workq;
    int start_time;
    int nodes;
    for (i = 0; i < 3; ++i) {
        hpx::util::high_resolution_timer timer;
        nodes = think(workq, board, 1);
        t[i] = (int)(timer.elapsed()*1000);
        std::cout << "Time: " << t[i] << " ms" << std::endl;
    }
    if (t[1] < t[0])
        t[0] = t[1];
    if (t[2] < t[0])
        t[0] = t[2];
    std::cout << std::endl;
    std::cout << "Nodes: " << nodes << std::endl;
    std::cout << "Best time: " << t[0] << " ms" << std::endl;
    if (t[0] == 0) {
        std::cout << "(invalid)" << std::endl;
        return;
    }
    nps = (double)nodes / (double)t[0];
    nps *= 1000.0;

    std::cout << "Nodes per second: " << (int)nps 
              << " (Score: " << (float)nps/177362 << ")" << std::endl;

    open_book();
}

int parseArgs(po::variables_map &vm)
{
    int flag = 0;


    if (vm.count("white")) {
        depth[LIGHT] = vm["white"].as<int>();
        if (depth[LIGHT] < 1)
            depth[LIGHT] = 1;
        flag = 1;
    }

    if (vm.count("black")) {
        depth[DARK] = vm["black"].as<int>();
        if (depth[DARK] < 1)
            depth[DARK] = 1;
        flag = 1;
    }

    if (vm.count("xboard")) {
        flag = 0;
    }

    if (vm.count("output")) {
        output = 0;
    }

    if (vm.count("notable")) {
        table = 0;
    }

    if (vm.count("sizetable")) {
        table = 1;
        hash_table_size = 
             (vm["sizetable"].as<int>() * 1048576) / sizeof(HASHE);
    }

    return flag;

}
