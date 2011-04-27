/*
 *  main.cpp
 */

#include "main.hpp"
#include "optlist.hpp"
#include "SimpleIni.h"
#include <signal.h>
#include <fstream>
#include <sys/time.h>

#ifdef READLINE_SUPPORT
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sstream>
#endif

double sum_exec_times2 = 0;
double sum_exec_times = 0;
int count_exec_times;

/* main() is basically an infinite loop that either calls
   think() when it's the computer's turn to move or prompts
   the user for a command (and deciphers it). */

int auto_move = 0;
int computer_side;

int chx_main(int argc, char **argv)
{
  int arguments = 0;
  std::string s("settings.ini");
  parseIni(s.c_str());
  arguments = parseArgs(argc,argv);

  int m;
#ifdef READLINE_SUPPORT
  char *buf;
#else
  std::string s;
#endif

  // If there were no command line arguments, display message
  if (!arguments) {
    std::cout << std::endl;
    std::cout << "Chess (CHX)" << std::endl;
    std::cout << "Phillip LeBlanc and Steve Brandt - CCT" << std::endl;
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

  init_hash();  /* Init hash sets up the hashing function
                   which is used for determining repeated moves */
  init_board(board);  // Initialize the board to its default state
  std::vector<move> workq;  /* workq is a standard vector which contains
                               all possible psuedo-legal moves for the
                               current board position */
  gen(workq, board);  /* gen() takes the current board position and
                         puts all of the psuedo-legal moves for the
                         position inside of the workq vector. */

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
      makemove(board, move_to_make.b); // Make the move for our master board
      board.ply = 0; // Reset the board ply to 0

      workq.clear(); // Clear the work queue in preparation for next move
      gen(workq, board); /* Populate the work queue with the moves for
                            the next board position */

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
    if (s == "search") {
      if (search_method == MINIMAX) {
        std::cout << "Alpha Beta search method now in use" << std::endl;
        search_method = ALPHABETA;
      } else if (search_method == ALPHABETA) {
        std::cout << "MTD-f search method now in use" << std::endl;
        search_method = MTDF;
      } else if (search_method == MTDF) {
        std::cout << "Minimax search method now in use" << std::endl;
        search_method = MINIMAX;
      }
      continue;
    }
    if (s == "help") {
      std::cout << "bench   - starts the benchmark" << std::endl;
      std::cout << "eval    - switches the current move evaluator in use (default original)" << std::endl;
      std::cout << "search  - switches the current search method in use (default minimax)" << std::endl;
      std::cout << "go      - computer makes a move" << std::endl;
      std::cout << "auto    - computer will continue to make moves until game is over" << std::endl;
      std::cout << "new     - starts a new game" << std::endl;
      std::cout << "wd      - sets white search depth (default 3)" << std::endl;
      std::cout << "bd      - sets black search depth (default 3)" << std::endl;
      std::cout << "d       - display the board" << std::endl;
      std::cout << "o       - toggles engine output on or off (default on)" << std::endl;
      std::cout << "exit    - exit the program" << std::endl;
      std::cout << "Enter moves in coordinate notation, e.g., e2e4, e7e8Q" << std::endl;
      std::cout << std::endl;
      continue;
    }
    
    int m;
    m = parse_move(workq, s.c_str());
    move mov;
    mov.u = m;
    node_t newboard = board;
    if (m == -1 || !makemove(newboard, mov.b))
			std::cout << "Illegal move or command." << std::endl;
		else {
      makemove(board, mov.b);
			board.ply = 0;
			workq.clear();
      gen(workq, board);
			print_result(workq, board);
		}
  }

  return 0;
}

int main(int argc, char *argv[])
{
  int retcode = chx_main(argc, argv);
  return retcode;
}

#if 0
#pragma mark -
#pragma mark Benchmark
#endif

void start_benchmark(std::string filename, int ply_level, int num_runs)
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
  logfile.open(logfilename.c_str());

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
  }

  // reading board configuration
  std::string line;
  while ( benchfile.good() )
  {
    getline (benchfile,line);
    line_num++;
    int i = -1;
    for (int j = 0; j < line.size(); j++)
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
  board.hist_dat.resize(10);
  board.hash = set_hash(board);
  //At this point we have the board position configured to the file specification
  print_board(board, std::cout);
  print_board(board, logfile);
  std::vector<move> workq;
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
      logfile << "(no legal moves)" << std::endl;
    }
    else
    {
      std::cout << "  Computer's move: " << move_str(move_to_make.b)
        << std::endl;

      logfile << "  Computer's move: " << move_str(move_to_make.b)
        << std::endl;
    }
    //std::cout << "Sleeping for twenty secs." << std::endl;
    //sleep(20);
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

#if 0
#pragma mark -
#pragma mark Parsing move notation
#endif

/* parse the move s (in coordinate notation) and return the move's
   int value, or -1 if the move is illegal */

int parse_move(std::vector<move>& workq, const char *s)
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
    if (workq[i].b.from == from && workq[i].b.to == to) {
      if (workq[i].b.bits & 32)
        switch (s[4]) {
          case 'N':
            if (workq[i].b.promote == KNIGHT)
              return workq[i].u;
          case 'B':
            if (workq[i].b.promote == BISHOP)
              return workq[i].u;
          case 'R':
            if (workq[i].b.promote == ROOK)
              return workq[i].u;
          default:
            return workq[i].u;
        }
      return workq[i].u;
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


#if 0
#pragma mark -
#pragma mark Printing functions
#endif

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

int print_result(std::vector<move>& workq, node_t& board)
{
  int i;

  // is there a legal move?
  for (i = 0; i < workq.size() ; ++i) { 
    node_t p_board = board;
    if (makemove(p_board, workq[i].b)) {
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

#if 0
#pragma mark -
#pragma mark Argument Parsing
#endif



int parseArgs(int argc, char **argv)
{
  if (argc == 1)
  {
    return 0;
  }
  option_t *optList, *thisOpt;
  optList = NULL;
  optList = GetOptList(argc, argv,"hs:?");
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
      printf("-s file     Uses the specified settings file instead of settings.ini\n");
      printf("\n");
      FreeOptList(thisOpt);
      exit(0);
    }

    switch (thisOpt->option)
    {
      case 's':
        parseIni(thisOpt->argument);
        break;
    }
    free(thisOpt);
  }
  return flag;
}

#if 0
#pragma mark -
#pragma mark Helper functions
#endif

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

#if 0
#pragma mark -
#pragma mark INI Parsing
#endif

bool parseIni(const char * filename)
{
  CSimpleIniA ini;
  SI_Error rc = ini.LoadFile(filename);
  if (rc < 0) return false;  // file not found

  // arguments to ini.GetValue are 1) section name 2) key name 3) value to return if its not in the ini file
  std::string s = std::string(ini.GetValue("CHX Main", "search_method", "minimax")); // ini.GetValue returns char * , so wrap it in std::string for fun    
  if (s == "minimax")
    search_method = MINIMAX;
  else if (s == "alphabeta")
    search_method = ALPHABETA;
  else if (s == "mtd-f")
    search_method = MTDF;
  else
    std::cerr << "Invalid parameter in ini file for 'search_method', please use \"minimax\" ,\"alphabeta\" or \"mtd-f\" " << std::endl;

  depth[LIGHT] = atoi(ini.GetValue("Depth", "white", "3"));
  depth[DARK]  = atoi(ini.GetValue("Depth", "black", "3"));

  s = std::string(ini.GetValue("CHX Main", "eval_method", "original"));
  if (s == "original")
    chosen_evaluator = ORIGINAL;
  else if (s == "simple")
    chosen_evaluator = SIMPLE;
  else
    std::cerr << "Invalid parameter in ini file for 'eval_method', please use \"original\" or \"simple\" " << std::endl;

  s = std::string(ini.GetValue("CHX Main", "output", "true"));
  if (s == "true")
    output = 1;
  else if (s == "false")
    output = 0;
  else
    std::cerr << "Invalid parameter in ini file for 'output', please use \"true\" or \"false\" " << std::endl;


  // Benchmark ini parsing

  s = std::string(ini.GetValue("Benchmark", "mode", "false"));
  if (s == "true")
  {
    bench_mode = true;
    s = std::string(ini.GetValue("Benchmark", "file", "ERROR"));

    if (s == "ERROR")
    {
      std::cerr << "Could not start benchmark because no input file was specified." << std::endl;
      exit(-1);
    }

    int max_ply = atoi(ini.GetValue("Benchmark", "max_ply", "3"));
    int num_runs  = atoi(ini.GetValue("Benchmark", "num_runs", "1"));

    init_hash();
    start_benchmark(s, max_ply, num_runs);
    exit(0);
  }
  else if (s != "false")
    std::cerr << "Invalid parameter in ini file for 'mode', please use \"true\" or \"false\" " << std::endl;

  iter_depth = atoi(ini.GetValue("CHX Main", "iter_depth", "5"));

  return true;
}

