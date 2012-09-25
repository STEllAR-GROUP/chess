////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2011 Steve Brandt and Philip LeBlanc
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file BOOST_LICENSE_1_0.rst or copy at http://www.boost.org/LICENSE_1_0.txt)
////////////////////////////////////////////////////////////////////////////////
/*
 *  main.cpp
 */

#ifdef HPX_SUPPORT
#include <hpx/hpx_init.hpp>
#include <hpx/include/actions.hpp>
#include <hpx/include/components.hpp>
#endif

#ifdef BOOST_SUPPORT
#include <boost/algorithm/string.hpp>
#endif
#include "main.hpp"
#include <signal.h>
#include <fstream>
#include <sys/time.h>

#include "mpi_support.hpp"

#ifdef MPI_SUPPORT
#undef MPI_SUPPORT
#endif


#ifdef READLINE_SUPPORT
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#endif
#include <sstream>

using namespace std;

double sum_exec_times2 = 0;
double sum_exec_times = 0;
int count_exec_times;

pthread_t rank_0_thread;

void mpi_terminate() {
    if(mpi_rank == 0) {
        for(int i=0;i<3;i++)
            std::cout << std::endl;
        const int n = 2*16+10;
        int data[n];
        data[32] = -1;
        data[33] = -1;
#ifdef MPI_SUPPORT
        for(int i=1;i<mpi_size;i++) {
            MPI_Send(data,n,MPI_INT,
                    i,WORK_ASSIGN_MESSAGE,MPI_COMM_WORLD);
        }
        pthread_join(rank_0_thread,NULL);
        MPI_Finalize();
#endif
    }
}

/* chx_main() processes input from stdin */

int auto_move = 0;
int computer_side;

#ifdef HPX_SUPPORT
int hpx_main(boost::program_options::variables_map& vm)
{
    int ret = chx_main();
    hpx::finalize();
    return ret;
}
#endif

int chx_main()
{

#ifdef READLINE_SUPPORT
    char *buf;
#else
    std::string buf;
#endif

    std::cout << std::endl;
    std::cout << "Chess (CHX)" << std::endl;
    std::cout << "Phillip LeBlanc and Steve Brandt - CCT" << std::endl;
    std::cout << std::endl;
    std::cout << "\"help\" displays a list of commands." << std::endl;
    std::cout << std::endl;
    computer_side = EMPTY;

    node_t board;  // The board state is represented in the node_t struct

    init_hash();  /* Init hash sets up the hashing function
                     which is used for determining repeated moves */
    init_board(board);  // Initialize the board to its default state
    std::vector<chess_move> workq;  /* workq is a standard vector which contains
                                 all possible psuedo-legal moves for the
                                 current board position */
    gen(workq, board);  /* gen() takes the current board position and
                           puts all of the psuedo-legal moves for the
                           position inside of the workq vector. */

    for (;;) {
        if (board.side == computer_side) {  // computer's turn

            // think about the chess_move and make it
            think(board,false);
            if (move_to_make.get32BitMove() == 0) {
                std::cout << "(no legal moves)" << std::endl;
                computer_side = EMPTY;
                continue;
            }
            if (output)
                std::cout << "Computer's chess_move: " << move_str(move_to_make)
                    << std::endl;
            makemove(board, move_to_make); // Make the chess_move for our master board
            board.ply = 0; // Reset the board ply to 0

            workq.clear(); // Clear the work queue in preparation for next chess_move
            gen(workq, board); /* Populate the work queue with the moves for
                                  the next board position */

            if (output)
                print_board(board, std::cout);
            if (auto_move)
                auto_move = print_result(workq, board);
            else
                print_result(workq, board);

            move_to_make.set32BitMove(0); // Reset the chess_move to make

            continue;
        }

        if (auto_move)
        {
            computer_side = board.side;
            continue;
        }
        // get user input
        std::string s;

#ifdef READLINE_SUPPORT
        buf = readline("chx> ");
        if (buf == NULL)
          return 0;
        s = buf;
        free(buf);
        if (s != "")
            add_history(s.c_str());
#else
        std::cout << "chx> ";
        std::cin >> s;
#endif

        if (s.empty())
            continue;

        std::vector<std::string> input;
#ifdef BOOST_SUPPORT
        boost::split(input, s, boost::is_any_of("\t "));
#else
        input.push_back(s);
#endif

        if (input[0] == "go") {
            computer_side = board.side;
            auto_move = 0;
            continue;
        }
        if (input[0] == "auto") {
            computer_side = board.side;
            auto_move = 1;
            auto_move = print_result(workq, board);
            continue;
        }
        if (input[0] == "new") {
            computer_side = EMPTY;
            init_board(board);
            workq.clear();
            gen(workq, board);
            continue;
        }
#ifdef BOOST_SUPPORT
        if (input[0] == "wd") {
          try {
            depth[LIGHT] = atoi(input.at(1).c_str());
          }
          catch (out_of_range&) {
            std::cout << "Set depth of white player: ";
            std::cin >> depth[LIGHT];
          }
            
          if (depth[LIGHT] <= 0)
          {
              std::cerr << "Illegal depth given, setting depth to 1." 
                  << std::endl;
              depth[LIGHT] = 1;
          }
          continue;
        }
        if (input[0] == "bd") {
          try {
            depth[DARK] = atoi(input.at(1).c_str());
          }
          catch (out_of_range&) {
            std::cout << "Set depth of black player: ";
            std::cin >> depth[DARK];
          }
          if (depth[DARK] <= 0)
          {
              std::cerr << "Illegal depth given, setting depth to 1." 
                  << std::endl;
              depth[DARK] = 1;
          }
          continue;
        }
#endif
        if (input[0] == "d") {
            print_board(board, std::cout);
            continue;
        }
        if ((input[0] == "o")||(input[0] == "output")) {
#ifdef BOOST_SUPPORT
          try {
            if (input.at(1) == "on")
              output = 1;
            else
              output = 0;
          }
          catch (out_of_range&) {
#endif
            output ^= 1;
            if (output == 1)
                std::cout << "Output is now on" << std::endl;
            else
                std::cout << "Output is now off" << std::endl;
#ifdef BOOST_SUPPORT
          }
#endif
          continue;
        }
        if ((input[0] == "exit")||(input[0] == "quit")) {
            std::cout << "Thanks for using CHX!" << std::endl;
            break;
        }
        if (input[0] == "parallel") {
            std::string arg;
#ifdef BOOST_SUPPORT
            try {
                arg = input.at(1);
            }
            catch (out_of_range&) {
#endif
#ifdef READLINE_SUPPORT
            buf = readline("Set number of threads: ");
            arg.append(buf);
            free(buf);
#else
            std::cout << "Set number of threads: ";
            std::cin >> arg;
#endif
#ifdef BOOST_SUPPORT
            }
#endif
            mpi_task_array[0].set_max(atoi(arg.c_str()));
            std::cout << "Set the number of threads to " << arg << std::endl;
            continue; 
        }
        if (input[0] == "bench") {
          bench_mode = true;
          int ply_level;
          int num_runs;
          std::string filename;
#ifdef BOOST_SUPPORT
          try {
            filename = input.at(1);
          }
          catch (out_of_range&) {
#endif
#ifdef READLINE_SUPPORT
          buf = readline("Name of file:  ");
          filename.append(buf);
          free(buf);
#else
          std::cout << "Name of file: ";
          std::cin >> filename;
#endif
#ifdef BOOST_SUPPORT
          }

          try {
            ply_level = atoi(input.at(2).c_str());
          }
          catch (out_of_range&) {
#endif
            std::cout << "Search depth (ply): ";
            std::cin >> ply_level;
#ifdef BOOST_SUPPORT
          }
          try {
            num_runs = atoi(input.at(3).c_str());
          }
          catch (out_of_range&) {
#endif
            std::cout << "Number of runs: ";
            std::cin >> num_runs;
#ifdef BOOST_SUPPORT
          }
#endif
          std::cout << std::endl;
          start_benchmark(filename, ply_level, num_runs, true);
          continue;
        }
        if (input[0] == "eval") {
          std::string eval;
#ifdef BOOST_SUPPORT
          try {
            eval = input.at(1);
          }
          catch (out_of_range&) {
#endif
            std::cout << "Name of evaluator (original,simple): ";
            std::cin >> eval;
#ifdef BOOST_SUPPORT
          }
#endif
          if (eval == "simple") {
            chosen_evaluator = SIMPLE;
          } else if (eval == "original") {
            chosen_evaluator = ORIGINAL;
          } else {
            std::cout << "Invalid evaluator specified." << std::endl;
          }
          continue;
        }
        if (input[0] == "search") {
          std::string search_m;
#ifdef BOOST_SUPPORT
          try {
            search_m = input.at(1);
          }
          catch (out_of_range&) {
#endif
            std::cout << "Name of search method (minimax,alphabeta,mtdf,multistrike): ";
            std::cin >> search_m;
#ifdef BOOST_SUPPORT
          }
#endif
          if (search_m == "minimax") {
              search_method = MINIMAX;
          } else if (search_m == "alphabeta") {
              search_method = ALPHABETA;
          } else if (search_m == "mtdf") {
              search_method = MTDF;
          } else if (search_m == "multistrike") {
              search_method = MULTISTRIKE;
          } else {
            std::cout << "Invalid method specified." << std::endl;
          }
          continue;
        }
        if (input[0] == "help") {
          std::cout << std::endl;
          std::cout << "  bench <name of file> <search depth> <number of runs>\n\tstarts the benchmark" << std::endl;
          std::cout << "  parallel <number of threads> \n\tSets the max number of parallel threads (default 0)" << std::endl;
          std::cout << "  eval <evaluator>\n\tswitches the current chess_move evaluator in use (default original)" << std::endl;
          std::cout << "  search <function>\n\tswitches the current search method in use (default minimax)" << std::endl;
          std::cout << "  go\n\tcomputer makes a chess_move" << std::endl;
          std::cout << "  auto\n\tcomputer will continue to make moves until game is over" << std::endl;
          std::cout << "  new\n\tstarts a new game" << std::endl;
#ifdef BOOST_SUPPORT
          std::cout << "  wd <number>\n\tsets white search depth (default 3)" << std::endl;
          std::cout << "  bd <number>\n\tsets black search depth (default 3)" << std::endl;
#endif
          std::cout << "  d\n\tdisplay the board" << std::endl;
          std::cout << "  o <on/off>\n\ttoggles engine output on or off (default on)" << std::endl;
          std::cout << "  exit\n\texit the program" << std::endl;
          std::cout << "  Enter moves in coordinate notation, e.g., e2e4, e7e8Q" << std::endl;
          std::cout << std::endl;
          continue;
        }

        int m;
        m = parse_move(workq, s.c_str());
        chess_move mov;
        mov = m;
        node_t newboard = board;
        if (m == -1 || !makemove(newboard, mov))
            std::cout << "Illegal chess_move or command." << std::endl;
        else {
            makemove(board, mov);
            board.ply = 0;
            workq.clear();
            gen(workq, board);
            print_result(workq, board);
        }
    }

    return 0;
}

int chx_threads_per_proc() {
    const char *th = getenv("CHX_THREADS_PER_PROC");
    int thcount = 0;
    if(th == NULL)
        ;
    else
        thcount = atoi(th);
    //std::cout << "thread count: " << thcount << std::endl;
    return thcount;
}

pthread_attr_t pth_attr;

int main(int argc, char *argv[])
{
    pthread_attr_init(&pth_attr);
    pthread_attr_setdetachstate(&pth_attr, PTHREAD_CREATE_DETACHED);
#ifdef MPI_SUPPORT
    MPI_Init(&argc,&argv);
    MPI_Comm_rank(MPI_COMM_WORLD,&mpi_rank);
    MPI_Comm_size(MPI_COMM_WORLD,&mpi_size);
#endif
    int threads_per_proc = chx_threads_per_proc();
    if(mpi_rank==0) {
        mpi_task_array.resize(mpi_size);
        for(int i=1;i<mpi_size;i++)
            mpi_task_array[i].add(1);
        mpi_task_array[0].set_max(threads_per_proc);
#ifdef MPI_SUPPORT
        // Just a diagnostic message announcing MPI is on
        std::cout << "MPI enabled" << std::endl;
        for(int i=0;i<mpi_size;i++)
            std::cout << mpi_task_array[i].add(0) << " ";
        std::cout << std::endl;
#endif
        pthread_create(&rank_0_thread,NULL,mpi_worker,NULL);
    } else {
        mpi_task_array.resize(1);
        mpi_task_array[0].add(threads_per_proc);
        mpi_worker(NULL);
        return 255;
    }
    mpi_terminate();
#ifdef HPX_SUPPORT
    boost::program_options::options_description
        desc_commandline("usage: " HPX_APPLICATION_STRING " [options]");
    return hpx::init(desc_commandline, argc, argv);
#else
    return chx_main();
#endif
}

void start_benchmark(std::string filename, int ply_level, int num_runs,bool parallel)
{
  std::ifstream benchfile(filename.c_str());

  if (!benchfile.is_open())
  {
    std::cerr << "Unable to open file" << std::endl;
    return;
  }

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
  if (logging_enabled)
    logfile.open(logfilename.c_str());
  else
    logfile.open("/dev/null");

  std::cout << "Using benchmark file: '" << filename << "'" << std::endl;
  std::cout << "  ply level: " << ply_level << std::endl;
  std::cout << "  num runs: " << num_runs << std::endl;

  logfile << "Using benchmark file: '" << filename << "'" << std::endl;
  logfile << "  ply level: " << ply_level << std::endl;
  logfile << "  num runs: " << num_runs << std::endl;
  if (chosen_evaluator == ORIGINAL) {
    std::cout << "  evaluator: original" << std::endl;
    logfile << "  evaluator: original" << std::endl;
  } else if (chosen_evaluator == SIMPLE) {
    std::cout << "  evaluator: simple" << std::endl;
    logfile << "  evaluator: simple" << std::endl;
  }

  if (search_method == MINIMAX) {
    std::cout << "  search method: minimax" << std::endl;
    logfile << "  search method: minimax" << std::endl;
  } else if (search_method == ALPHABETA) {
    std::cout << "  search method: alpha-beta" << std::endl;
    logfile << "  search method: alpha-beta" << std::endl;
  } else if (search_method == MTDF) {
    std::cout << "  search method: MTD-f" << std::endl;
    logfile << "  search method: MTD-f" << std::endl;
  } else if (search_method == MULTISTRIKE) {
    std::cout << "  search method: Multistrike" << std::endl;
    logfile << "  search method: Multistrike" << std::endl;
  }

  // reading board configuration
  std::string line;
  while ( benchfile.good() )
  {
    getline (benchfile,line);
    line_num++;
    int i = -1;
    for (size_t j = 0; j < line.size(); j++)
    {
      c = line.at(j);
      if (j == 0 && c == '#') {
        line_num--; break;
        break;
      }
      if (c == ' ')
        continue;
      i++;
      if(i >= 8)
        break;
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
    if(i >= 0 && i < 7) {
        throw i;
    }
  }
  if(line_num != 8)
      throw line_num;
  benchfile.close();

  board.side = LIGHT;
  board.castle = 15;
  board.ep = -1;
  board.fifty = 0;
  board.ply = 0;
  board.hply = 0;
  board.hash = set_hash(board);
  //At this point we have the board position configured to the file specification
  print_board(board, std::cout);
  print_board(board, logfile);
  std::vector<chess_move> workq;
  gen(workq, board);

  int start_time;
  int* t;
  t = (int *)malloc(sizeof(int)*num_runs);  // Allocate an int array to hold all of the times for the runs
  double average_time = 0;
  int best_time = 0;

  std::cout << "Parallel=" << parallel << std::endl;
  for (int i = 0; i < num_runs; ++i)
  {
    std::cout << "Run " << i+1 << " ";
    logfile << "Run " << i+1 << " ";
    fflush(stdout);
    start_time = get_ms();          // Start the clock
    think(board,parallel);          // Do the processing
    t[i] = get_ms() - start_time;   // Measure the time
    if (i == 0)
      best_time = t[0];
    else if (t[i] < best_time)
      best_time = t[i];
    std::cout << "time: " << t[i] << " ms" << std::endl;
    logfile << "time: " << t[i] << " ms" << std::endl;

    if (move_to_make.get32BitMove() == 0) {
      std::cout << "(no legal moves)" << std::endl;
      logfile << "(no legal moves)" << std::endl;
    }
    else
    {
      std::cout << "  Computer's chess_move: " << move_str(move_to_make)
        << std::endl;

      logfile << "  Computer's chess_move: " << move_str(move_to_make)
        << std::endl;
    }
    // Allow time for aborted threads to get cleaned up
    sleep(2);
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

/* parse the chess_move s (in coordinate notation) and return the chess_move's
   int value, or -1 if the chess_move is illegal */

int parse_move(std::vector<chess_move>& workq, const char *s)
{
  int from, to;

  // make sure the string looks like a chess_move
  if (s[0] < 'a' || s[0] > 'h' ||
      s[1] < '0' || s[1] > '9' ||
      s[2] < 'a' || s[2] > 'h' ||
      s[3] < '0' || s[3] > '9')
    return -1;

  from = s[0] - 'a';
  from += 8 * (8 - (s[1] - '0'));
  to = s[2] - 'a';
  to += 8 * (8 - (s[3] - '0'));

  for (size_t i = 0; i < workq.size(); i++) {
    if (workq[i].getFrom() == from && workq[i].getTo() == to) {
      if (workq[i].getBits() & 32)
        switch (s[4]) {
          case 'N':
            if (workq[i].getPromote() == KNIGHT)
              return workq[i].get32BitMove();
          case 'B':
            if (workq[i].getPromote() == BISHOP)
              return workq[i].get32BitMove();
          case 'R':
            if (workq[i].getPromote() == ROOK)
              return workq[i].get32BitMove();
          default:
            return workq[i].get32BitMove();
        }
      return workq[i].get32BitMove();
    }
  }

  // didn't find the chess_move
  return -1;
}


// move_str returns a string with chess_move m in coordinate notation

char *move_str(chess_move& m)
{
  static char str[6];

  char c;

  if (m.getBits() & 32) {
    switch (m.getPromote()) {
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
        COL(m.getFrom()) + 'a',
        8 - ROW(m.getFrom()),
        COL(m.getTo()) + 'a',
        8 - ROW(m.getTo()),
        c);
  }
  else
    sprintf(str, "%c%d%c%d",
        COL(m.getFrom()) + 'a',
        8 - ROW(m.getFrom()),
        COL(m.getTo()) + 'a',
        8 - ROW(m.getTo()));
  return str;
}

// print_board() prints the board

void print_board(const node_t& board, std::ostream& out)
{
  int i;

  out << std::endl << "8 ";
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
      out << std::endl << 7 - ROW(i) << " ";
  }
  out << std::endl << std::endl << "   a b c d e f g h" << std::endl << std::endl;
}


/* print_result() checks to see if the game is over, and if so,
   prints the result. */

int print_result(std::vector<chess_move>& workq, node_t& board)
{
  size_t i;

  // is there a legal chess_move?
  for (i = 0; i < workq.size() ; ++i) { 
    node_t p_board = board;
    if (makemove(p_board, workq[i])) {
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
    std::cout << "1/2-1/2 {Draw by fifty chess_move rule}" << std::endl;
    return 0;
  }
  return 1;
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
