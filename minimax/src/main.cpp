/*
 *  MAIN.C
 */

#include "main.hpp"
#include "optlist.hpp"
#include <signal.h>

int parseArgs(int, char**);

/* main() is basically an infinite loop that either calls
   think() when it's the computer's turn to move or prompts
   the user for a command (and deciphers it). */

int auto_move = 0;
int computer_side;

int chx_main(int argc, char **argv)
{
    int arguments = parseArgs(argc,argv);

    std::string s;
    int m;

    if (!arguments) {
        std::cout << std::endl;
        std::cout << "HPX Chess (CHX) Serial Version" << std::endl;
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
            workq.clear();
            gen(workq, board);
            print_result(workq, board);
        }
    }
    close_book();

    return 0;
}

void ctrlc(int s)
{
    if (!auto_move && computer_side == EMPTY)
    {
        close_book();
        exit(0);
    }
    auto_move = 0;
    computer_side = EMPTY;
}

int main(int argc, char *argv[])
{

    signal( SIGINT, ctrlc);

    int retcode = chx_main(argc, argv);
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
        nodes = think(workq, board, 1);
        //FIXME: add timer code back in
        //t[i] = (int)(timer.elapsed()*1000);
        t[i] = 0;
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
int parseArgs(int argc, char **argv)
{
    if (argc == 1)
    {
        return 0;
    }
    option_t *optList, *thisOpt;
    optList = NULL;
    optList = GetOptList(argc, argv, "w:b:mxoht?");
    int flag = 0;

    while (optList != NULL)
    {
        thisOpt = optList;
        optList = optList->next;

        if (('?' == thisOpt->option)||('h' == thisOpt->option))
        {
            // display help
            printf("Command line options:\n");
            printf("-h          Displays this help message\n");
            printf("-w n        Sets the depth of the white side to n (default 3)\n");
            printf("-b n        Sets the depth of the black side to n (default 3)\n");
            printf("-o          Turns off all engine output\n");
            printf("-t          Turns off transposition table\n");
            printf("\n");
            FreeOptList(thisOpt);
            exit(0);
        }

        switch (thisOpt->option)
        {
            case 'w':
                // white depth level
                depth[LIGHT] = atoi(thisOpt->argument);
                if (depth[LIGHT] <= 0)
                {
                    fprintf(stderr, "Illegal depth given, setting depth to 1.\n");
                    depth[LIGHT] = 1;
                }
                flag = 1;
                break;
            case 'b':
                // black depth level
                depth[DARK] = atoi(thisOpt->argument);
                if (depth[DARK] <= 0)
                {
                    fprintf(stderr, "Illegal depth given, setting depth to 1.\n");
                    depth[DARK] = 1;
                }
                flag = 1;
                break;
            case 'x':
                flag = 0;
                break;
            case 'o':
                output = 0;
                break;
            case 't':
                table = 0;
                break;
        }
        free(thisOpt);
    }
    return flag;
}
