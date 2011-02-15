/*
 *  main.cpp
 */

#include "main.hpp"
#include "optlist.hpp"
#include <signal.h>
#include <fstream>
#include <sys/time.h>
#include <math.h>

#ifdef READLINE_SUPPORT
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sstream>
#endif

#ifdef OPENMP_SUPPORT
#include <omp.h>
#endif

struct timeval startt;
double sum_exec_times2 = 0;
double sum_exec_times = 0;
int count_exec_times;

int parseArgs(int, char**);

/* main() is basically an infinite loop that either calls
   think() when it's the computer's turn to move or prompts
   the user for a command (and deciphers it). */

int auto_move = 0;
int computer_side;

int chx_main(int argc, char **argv)
{
    int arguments = parseArgs(argc,argv);
    
    int m;
    #ifdef READLINE_SUPPORT
    char *buf;
    #else
    std::string s;
    #endif

    // If there were no command line arguments, display message
    if (!arguments) {
        std::cout << std::endl;
        #ifdef OPENMP_SUPPORT
        std::cout << "Chess (CHX) OpenMP Minimax Version" << std::endl;
        #else
        std::cout << "Chess (CHX) Serial Minimax Version" << std::endl;
        #endif
        std::cout << "Phillip LeBlanc - CCT" << std::endl;
        std::cout << std::endl;
        std::cout << "\"help\" displays a list of commands." << std::endl;
        std::cout << std::endl;
        computer_side = EMPTY;
    }
    else  // Else we start making moves
    {
        computer_side = LIGHT;
        auto_move = 1;
    }
    node_t board;  // The board state is represented in the node_t struct

    init_hash();  // Init hash sets up the hashing function
                  // which is used for determining repeated moves
    init_board(board);  // Initialize the board to its default state
    std::vector<gen_t> workq;  // workq is a standard vector which contains
                               // all possible psuedo-legal moves for the
                               // current board position
    gen(workq, board);  // gen() takes the current board position and
                        // puts all of the psuedo-legal moves for the
                        // position inside of the workq vector.

    for (;;) {
        if (board.side == computer_side) {  // computer's turn

            // think about the move and make it
            think(board);
            if (move_to_make.u == 0) {
                std::cout << "(no legal moves)" << std::endl;
                computer_side = EMPTY;
                continue;
            }
            if (output)
                std::cout << "Computer's move: " << move_str(move_to_make.b)
                          << std::endl;
            makemove(board, move_to_make.b); // Make the move for our master
                                             // board
            board.ply = 0; // Reset the board ply to 0

            workq.clear(); // Clear the work queue in preparation for next move
            gen(workq, board); // Populate the work queue with the moves for
                               // the next board position

            if (output)
                print_board(board, std::cout);
            if (auto_move)
                auto_move = print_result(workq, board);
            else
                print_result(workq, board);

            move_to_make.u = 0; // Reset the move to make

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

        // get user input
        
        #ifdef READLINE_SUPPORT
        buf = readline("chx> ");
        std::string s(buf);
        free(buf);
        if (s != "")
              add_history(s.c_str());
        #else
        std::cout << "chx> ";
        std::cin >> s;
        #endif
        
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
            std::cout << "Set depth of white player: ";
            std::cin >> depth[LIGHT];
            if (depth[LIGHT] <= 0)
            {
                std::cerr << "Illegal depth given, setting depth to 1." 
                          << std::endl;
                depth[LIGHT] = 1;
            }
            continue;
        }
        if (s == "bd") {
            std::cout << "Set depth of black player: ";
            std::cin >> depth[DARK];
            if (depth[DARK] <= 0)
            {
                std::cerr << "Illegal depth given, setting depth to 1." 
                          << std::endl;
                depth[DARK] = 1;
            }
            continue;
        }
        if (s == "d") {
            print_board(board, std::cout);
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
        #ifdef OPENMP_SUPPORT
        std::string nt = "nt";
        if (s == nt) {
            int nt;
            std::cout << "Set number of threads: ";
            std::cin >> nt;
            number_threads = nt;
            omp_set_num_threads(nt);
            continue;
        }
        if (s.compare(0,nt.length(),nt)==0) {
            int nt;
            std::istringstream iss(s);
            std::string first;
            iss >> first;
            
            iss >> nt;
            number_threads = nt;
            omp_set_num_threads(nt);
            continue;
        }
        #endif
        if ((s == "exit")||(s == "quit")) {
            std::cout << "Thanks for using CHX!" << std::endl;
            break;
        }
		std::string bench = "bench";
        if (s == bench) {
            int ply_level;
            int num_runs;
            
            #ifdef READLINE_SUPPORT
            buf = readline("Name of file:  ");
            std::string filename(buf);
            free(buf);
            #else
            std::string filename;
            std::cout << "Name of file: ";
            std::cin >> filename;
            #endif
            
            std::cout << "Search depth (ply): ";
            std::cin >> ply_level;
            std::cout << "Number of runs: ";
            std::cin >> num_runs;
            std::cout << std::endl;
            start_benchmark(filename, ply_level, num_runs);
			continue;
        }
		if(s.compare(0,bench.length(),bench)==0) {
            int ply_level;
            int num_runs;
			std::istringstream iss(s);
			std::string first;
			iss >> first;
            
            std::string filename;
            iss >> filename;
            
            iss >> ply_level;
            iss >> num_runs;
            std::cout << std::endl;
            start_benchmark(filename, ply_level, num_runs);
			continue;
		}
		if (s == "eval") {
		    if (chosen_evaluator == ORIGINAL) {
                std::cout << "Switching evaluator to simple material evaluator" << std::endl;
                chosen_evaluator = SIMPLE;
            } else if (chosen_evaluator == SIMPLE) {
                std::cout << "Switching evaluator to original evaluator" << std::endl;
                chosen_evaluator = ORIGINAL;
            }
            continue;
		}
        if (s == "help") {
            std::cout << "bench   - starts the benchmark" << std::endl;
            std::cout << "eval    - switches the current move evaluator in use" << std::endl;
            std::cout << "go      - computer makes a move" << std::endl;
            std::cout << "auto    - computer will continue to make moves until game is over" << std::endl;
            std::cout << "new     - starts a new game" << std::endl;
            std::cout << "wd      - sets white search depth (default 3)" << std::endl;
            std::cout << "bd      - sets black search depth (default 3)" << std::endl;
            std::cout << "d       - display the board" << std::endl;
            std::cout << "o       - toggles engine output on or off (default on)" << std::endl;
            std::cout << "exit    - exit the program" << std::endl;
            #ifdef OPENMP_SUPPORT
            std::cout << "nt      - sets the number of threads for parallel execution" << std::endl;
            #endif
            std::cout << std::endl;
            continue;
        }

        std::cout << "Illegal command." << std::endl;
    }

    return 0;
}

int main(int argc, char *argv[])
{
	#ifdef OPENMP_SUPPORT
    omp_set_num_threads(1);
    #endif
    int retcode = chx_main(argc, argv);
    return retcode;
}

void start_benchmark(std::string filename, int ply_level, int num_runs)
{
    depth[LIGHT] = ply_level;
    depth[DARK]  = ply_level;
    
    node_t board;
    
    init_board(board);
    
    int line_num = -1;
    char c;
    int spot;
    
    // Logging to file code
    std::string logfilename = get_log_name();
    
    std::ofstream logfile;
    logfile.open(logfilename.c_str());

	std::cout << "Using benchmark file: '" << filename << "'" << std::endl;
	std::cout << "  ply level: " << ply_level << std::endl;
	std::cout << "  num runs: " << num_runs << std::endl;
	#ifdef OPENMP_SUPPORT
    std::cout << "  num threads: " << number_threads << std::endl;
    #endif
	
	logfile << "Using benchmark file: '" << filename << "'" << std::endl;
	logfile << "  ply level: " << ply_level << std::endl;
	logfile << "  num runs: " << num_runs << std::endl;
	#ifdef OPENMP_SUPPORT
    logfile << "  num threads: " << number_threads << std::endl;
    #endif
	if (chosen_evaluator == ORIGINAL) {
        std::cout << "  evaluator: original" << std::endl;
        logfile << "  evaluator: original" << std::endl;
    } else if (chosen_evaluator == SIMPLE) {
        std::cout << "  evaluator: simple" << std::endl;
        logfile << "  evaluator: simple" << std::endl;
    }
    
    // reading board configuration
    std::string line;
    std::ifstream benchfile(filename.c_str());
    if (benchfile.is_open())
    {
        while ( benchfile.good() )
        {
            getline (benchfile,line);
            line_num++;
            //Each line has 8 characters
			if(line.length()!=8)
				break;
            for (int i = 0; i < 8; i++)
            {
                c = line.at(i);
                spot = line_num*8+i;
                if (c == '.')
                {
                    board.color[spot] = 6;
                    board.piece[spot] = 6;
                }
                else if(islower(c))
                {
                    board.color[spot] = 1;
                    switch (c)
                    {
                        case 'k':
                            board.piece[spot] = 5;
                            break;
                        case 'q':
                            board.piece[spot] = 4;
                            break;
                        case 'r':
                            board.piece[spot] = 3;
                            break;
                        case 'b':
                            board.piece[spot] = 2;
                            break;
                        case 'n':
                            board.piece[spot] = 1;
                            break;
                        case 'p':
                            board.piece[spot] = 0;
                            break;
                        default:
                            //error
                            break;
                    }
                }
                else if(isupper(c))
                {
                    board.color[spot] = 0;
                    switch (c)
                    {
                        case 'K':
                            board.piece[spot] = 5;
                            break;
                        case 'Q':
                            board.piece[spot] = 4;
                            break;
                        case 'R':
                            board.piece[spot] = 3;
                            break;
                        case 'B':
                            board.piece[spot] = 2;
                            break;
                        case 'N':
                            board.piece[spot] = 1;
                            break;
                        case 'P':
                            board.piece[spot] = 0;
                            break;
                        default:
                            //error
                            break;
                    }
                }
            }
        }
        benchfile.close();
    }
    else {
        std::cout << "Unable to open file";
        logfile << "Unable to open file";
        return;
    }
    board.side = LIGHT;
    board.castle = 15;
    board.ep = -1;
    board.fifty = 0;
    board.ply = 0;
    board.hply = 0;
    board.hist_dat.resize(10);
    board.hash = set_hash(board);
    //At this point we have the board position configured to the file specification
    print_board(board, std::cout);
    print_board(board, logfile);
    std::vector<gen_t> workq;
    gen(workq, board);
    
    int start_time;
    int* t;
    t = (int *)malloc(sizeof(int)*num_runs);  // Allocate an int array to hold all of the times for the runs
    double average_time = 0;
    int best_time = 0;
    
    for (int i = 0; i < num_runs; ++i)
    {
        std::cout << "Run " << i+1 << " ";
        logfile << "Run " << i+1 << " ";
        fflush(stdout);
        start_time = get_ms();          // Start the clock
        think(board);                   // Do the processing
        t[i] = get_ms() - start_time;   // Measure the time
        if (i == 0)
            best_time = t[0];
        else if (t[i] < best_time)
            best_time = t[i];
        std::cout << "time: " << t[i] << " ms" << std::endl;
        logfile << "time: " << t[i] << " ms" << std::endl;
        
        if (move_to_make.u == 0) {
            std::cout << "(no legal moves)" << std::endl;
            logfile << "time: " << t[i] << " ms" << std::endl;
        }
        else
        {
            std::cout << "  Computer's move: " << move_str(move_to_make.b)
                      << std::endl;
            
            logfile << "  Computer's move: " << move_str(move_to_make.b)
                      << std::endl;
        }
    }
    
    for (int i = 0; i < num_runs; ++i)
    {
        average_time += t[i];
    }
    average_time = average_time / num_runs;
    
    free(t);
    
    std::cout << std::endl;
    std::cout << "Results:" << std::endl;
    std::cout << "Number of runs:       " << num_runs << std::endl;
    std::cout << "Time for best run:    " << best_time << " ms"<< std::endl;
    std::cout << "Average time for run: " << (int)average_time << " ms" << std::endl;
    
    logfile << std::endl;
    logfile << "Results:" << std::endl;
    logfile << "Number of runs:       " << num_runs << std::endl;
    logfile << "Time for best run:    " << best_time << " ms"<< std::endl;
    logfile << "Average time for run: " << (int)average_time << " ms" << std::endl;
    
    logfile.close(); // Close the open file

}


/* parse the move s (in coordinate notation) and return the move's
   int value, or -1 if the move is illegal */

int parse_move(std::vector<gen_t>& workq, const char *s)
{
    int from, to, i;

    // make sure the string looks like a move
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

    // didn't find the move
    return -1;
}


// move_str returns a string with move m in coordinate notation

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


// print_board() prints the board

void print_board(node_t& board, std::ostream& out)
{
    int i;

    out << std::endl << "8 ";
    for (i = 0; i < 64; ++i) {
        switch (board.color[i]) {
            case EMPTY:
                out << " .";
                break;
            case LIGHT:
                out << " " << piece_char[board.piece[i]];
                break;
            case DARK:
                char ch = (piece_char[board.piece[i]] + ('a' - 'A'));
                out << " " << ch;
                break;
        }
        if ((i + 1) % 8 == 0 && i != 63)
            out << std::endl << 7 - ROW(i) << " ";
    }
    out << std::endl << std::endl << "   a b c d e f g h" << std::endl << std::endl;
}


/* print_result() checks to see if the game is over, and if so,
   prints the result. */

int print_result(std::vector<gen_t>& workq, node_t& board)
{
    int i;

    // is there a legal move?
    for (i = 0; i < workq.size() ; ++i) { 
        node_t p_board = board;
        if (makemove(p_board, workq[i].m.b)) {
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

int parseArgs(int argc, char **argv)
{
    if (argc == 1)
    {
        return 0;
    }
    option_t *optList, *thisOpt;
    optList = NULL;
    #ifdef OPENMP_SUPPORT
    optList = GetOptList(argc, argv,"w:b:mxohet:?");
    #else
    optList = GetOptList(argc, argv,"w:b:mxohe?");
    #endif
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
            printf("-e          Sets the evaluation method to simple material evaluation\n");
            #ifdef OPENMP_SUPPORT
            printf("-t n        Sets the number of threads to n\n");
            #endif
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
            case 'o':
                output = 0;
                flag = 1;
                break;
            case 'e':
                flag = 1;
                chosen_evaluator = SIMPLE;
                break;
            #ifdef OPENMP_SUPPORT
            case 't':
                int nt = atoi(thisOpt->argument);
                number_threads = nt;
                omp_set_num_threads(nt);
                break;
            #endif
        }
        free(thisOpt);
    }
    return flag;
}

// get_ms() gets the current time in milliseconds

int get_ms()
{
    struct timeb timebuffer;
    ftime(&timebuffer);
    return (timebuffer.time * 1000) + timebuffer.millitm;
}

std::string get_log_name()
{
    time_t rawtime;
    struct tm * timeinfo;
    time ( &rawtime );
    timeinfo = localtime ( &rawtime );
    
    std::stringstream logfilenamestream;
    logfilenamestream << (timeinfo->tm_year+1900) << ".";
    
    if ((timeinfo->tm_mon+1) < 10)
        logfilenamestream << "0";
    logfilenamestream << (timeinfo->tm_mon+1) << ".";
    if ((timeinfo->tm_mday) < 10)
        logfilenamestream << "0";
    logfilenamestream << (timeinfo->tm_mday) << ".";
    if ((timeinfo->tm_hour) < 10)
        logfilenamestream << "0";
    logfilenamestream << (timeinfo->tm_hour) << ":"; 
    if (timeinfo->tm_min+1 < 10)
        logfilenamestream << "0";
    logfilenamestream << (timeinfo->tm_min+1) << ":";
    if (timeinfo->tm_sec+1 < 10)
        logfilenamestream << "0";
    logfilenamestream << (timeinfo->tm_sec+1) << ".chx_bench.log";
                      
    std::string logfilename;
    logfilename = logfilenamestream.str();
    
    return logfilename;
}
