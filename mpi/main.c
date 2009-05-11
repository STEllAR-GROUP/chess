/*
 *	MAIN.C
 *	Tom Kerrigan's Simple Chess Program (TSCP)
 *
 *	Copyright 1997 Tom Kerrigan
 *
 *
 *	Modified by Phillip LeBlanc, LSU CCT, 2009
 *  
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include "defs.h"
#include "data.h"
#include "protos.h"
#include <sys/timeb.h>

int get_ms();
void selectOptions();
void parseArgs(int argc, char **argv);
FILE* set_output(FILE *out);
int close_output(FILE *out);
int parse_move(char *s);
char *move_str(move_bytes m);
void print_board(board_t board);
void print_result(board_t board, int side, int xside);
void initialize();
void ending();
int start_network(int role, char *host, int portno);
void close_network();
int send_move(char *move);
int get_move(char *buff, int len);
void init_hash();
int hash_rand();
int set_hash(board_t board, int side);


FILE *out;
BOOL ftime_ok = FALSE;  /* does ftime return milliseconds? */
int mov;
int max_depth;
int side;
int startingside; //Only used if network is enabled
hist_t history[3000];
board_t board;
char movestr[10]; //Move string
move m;
int portnum;
int network;
int server;
char hostname[256];


int main(int argc, char *argv[])
{

    if (argc == 1)
      selectOptions();
    else
	    parseArgs(argc, argv);

    initialize();

    for (;;)
    {
       
       /***********************Starting main loop**************/
       if (network&&(side==startingside))
       {
          /* think about the move and return the move to be made */
          m.u = pickbestmove(board, max_depth, side, mov, history);  //Store the move in m
          if (m.u == -1)
          {
             print_result(board, side, side ^ 1);
             break;
          } //end if(!m.u)
          strcpy(movestr, move_str(m.b));
          send_move(movestr);
          fprintf(stdout, "Sent move %s to other computer/cluster.\n", movestr);
       }
       else if (network)
       {
          get_move(movestr, sizeof movestr); //Get move from other computer
          printf("Successfully received move %s from other computer/cluster.\n", movestr);
          m.u = parse_move(movestr); //Parse it into our format
       }
       else
       {
          /* think about the move and return the move to be made */
          m.u = pickbestmove(board, max_depth, side, mov, history);  //Store the move in m
          if (m.u == -1)
          {
             print_result(board, side, side ^ 1);
             break;
          } //end if(!m.u)
          strcpy(movestr, move_str(m.b));
       }
		   
      history[mov].m = m;


      fprintf(out, "%s\n", movestr);
      fflush(out);
      makeourmove(board, m.b, &board, side);
		
		
      mov++;	//Update the move counter
	    print_board(board);	//Print the board
	    side ^= 1;	//Switch sides
        continue;
    } //end for(;;)

    ending();

    return 0;
}


/* parse the move s (in coordinate notation) and return the move's
   union, or -1 if the move is illegal */

int parse_move(char *s)
{
    int from, to;

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

    move m;

    m.b.from = from;
    m.b.to = to;
    m.b.promote = 0;
    m.b.bits = 0;

    return m.u;
}


/* move_str returns a string with move m in coordinate notation */

char *move_str(move_bytes m)
{
    static char str[6];

    char c;

    if (m.bits & 32)
    {
        switch (m.promote)
        {
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

void print_board(board_t board)
{
    int i;

    printf("\n8 ");
    for (i = 0; i < 64; ++i)
    {
        switch (board.color[i])
        {
        case EMPTY:
            printf(" .");
            break;
        case LIGHT:
            printf(" %c", piece_char[board.piece[i]]);
            break;
        case DARK:
            printf(" %c", piece_char[board.piece[i]] + ('a' - 'A'));
            break;
        }
        if ((i + 1) % 8 == 0 && i != 63)
            printf("\n%d ", 7 - ROW(i));
    }
    printf("\n\n   a b c d e f g h\n\n");
}





/* print_result() checks to see if the game is over, and if so,
   prints the result. */

void print_result(board_t board, int side, int xside)
{
    char result[256];
    if (in_check(board, side)||in_check(board, xside))
        {
            if (side == LIGHT)
                strcpy(result, "0-1 {Black mates}\n");
            else
                strcpy(result, "1-0 {White mates}\n");
        }
        else
            strcpy(result, "1/2-1/2 {Stalemate}\n");
    
    fprintf(out, "%s\n", result);
    printf("\n%s\n", result);
}


int get_ms()
{
    struct timeb timebuffer;
    ftime(&timebuffer);
    if (timebuffer.millitm != 0)
       ftime_ok = TRUE;
    return (timebuffer.time * 1000) + timebuffer.millitm;
}

void initialize()
{
  
    start_time_main = get_ms();
	  memset(history, 0 , sizeof(history));
    init_board(&board);
    seed_rand();
    mov = 0;  //Total moves made
	  tot_nodes = 0;
    open_book();
    out = set_output(out);	//set the output file "chess*.log"
    init_hash();
    max_time = 1 << 25;
    if (network)
      if(start_network(server, hostname, portnum) < 0)
      {
        //fprintf(stderr, "Error starting network, switching to local mode.");
        //network = 0;
          fprintf(stderr, "Error starting network, exiting");
          exit(1);
      }
}

void ending()
{
  
    close_book();


    if (network)
      close_network();
    /**************************Statistics*********************************/

    fprintf(out, "Max Depth Searched: %d\n", max_depth);
    //fprintf(out, "Total Nodes Searched: %d\n", tot_nodes);
    fprintf(out, "Total Moves Made: %d\n", mov);
    fprintf(out, "Execution Time: %d ms\n", mov = (get_ms() - start_time_main));	//Reuse mov to store the execution time
    //fprintf(out, "Average Nodes per ms: %0.1f", (double)tot_nodes/(double)mov);

    /*********************************************************************/

    if (close_output(out))	//close_output closes the logfile and prints out an error message if something went wrong.
      fprintf(stderr, "Error when closing the logfile.");
}

void selectOptions()
{
  fprintf(stderr, "usage: chess -d <max depth> -s <white/black> -n <hostname:port number>\n");
  exit(0);
  network = 0;
  server = 0;
  max_depth = 3;
  portnum = 0;
  side = LIGHT;
}

void parseArgs(int argc, char **argv)
{
 int c;
 char * ptr;
 network = 0;
 server = 0;
 max_depth = 3;
 portnum = 0;
 side = LIGHT;
 strcpy(hostname, "localhost");
 while ((c = getopt(argc, argv, "d:s:n:")) != -1)
   switch (c)
   {
     case 'd':
      max_depth = atoi(optarg);
      break;
     case 's':
      if (!strcmp("white", optarg))
      {
        server = 2;
        startingside = LIGHT;
      }
      else if (!strcmp("black", optarg))
      {
        server = 1;
        startingside = DARK;
      }
      else
      {
        fprintf(stderr, "usage: chess -d <max depth> -s <white/black> -n <hostname:port number>\n");
        exit(1);
      }
      break;
     case 'n':
      network = 1;
      ptr = strtok(optarg, ":");
      strcpy(hostname,ptr); //Copy the hostname
      ptr = strtok(NULL, " :");
      portnum = atoi(ptr);
      break;
     case '?':
     default:
      fprintf(stderr, "usage: chess -d <max depth> -s <white/black> -n <hostname:port number>\n");
      exit(1);
      break;
   }
}
