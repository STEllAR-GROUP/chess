/*
 * mpi.c
 * Created by Phillip LeBlanc
 *
 * MPI Tags:
 * 0 == assign me an available process
 * 1 == I am finished working, available for more work
 * 2 == terminate immediately
 * 3 == search data part 1
 * 4 == search data part 2 (board.color)
 * 5 == search data part 3 (board.piece)
 * 6 == search data part 4 (history)
 * 8 == send search score
 * 10 == request pv & pv_length arrays
 * 11 == send updated pv array
 * 12 == send updated pv_length array
 * 13 == send best move
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
#include "mpi.h"
 
FILE *out;
int mov;
int max_depth;
int side;
hist_t history[3000];
board_t board;
char movestr[10]; //Move string
move m;

int mpi(int sside, int argc, char **argv)
{
    parseArgs(argc, argv);

    initialize(sside);
    MPI_Status status;

    for (;;)
    {
       
       /***********************Starting main loop**************/
       if (side==sside)
       {
          /* think about the move and return the move to be made */
          m.u = pickbestmove(board, max_depth, side, mov, history);  //Store the move in m
          if (m.u == -1)
          {
             print_result(board, side, side ^ 1);
             break;
          } //end if(!m.u)
          strcpy(movestr, move_str(m.b));
          MPI_Send(movestr, strlen(movestr)+1, MPI_CHAR, sside ^ 1, 0, MPI_COMM_WORLD);	//Send move to the other process
          printf("%d: Sent move %s to other process.\n", sside, movestr);
       }
       else
       {
       	  MPI_Recv(movestr, sizeof movestr + 1, MPI_CHAR, sside ^ 1, 0, MPI_COMM_WORLD, &status);
          printf("%d: Successfully received move %s from other process.\n", sside, movestr);
          m.u = parse_move(movestr); //Parse it into our format
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
    return (timebuffer.time * 1000) + timebuffer.millitm;
}

void initialize(int sside)
{
  
    start_time_main = get_ms();
	memset(history, 0 , sizeof(history));
    init_board(&board);
    seed_rand();
    mov = 0;  //Total moves made
	tot_nodes = 0;
    open_book();
    out = set_output(out, sside);	//set the output file "chess*.log"
    init_hash();
    max_time = 1 << 25;
}

void ending()
{
  
    close_book();

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

void parseArgs(int argc, char **argv)
{
 int c;
 max_depth = 3;
 while ((c = getopt(argc, argv, "d:")) != -1)
   switch (c)
   {
     case 'd':
      max_depth = atoi(optarg);
      break;
     case '?':
     default:
      fprintf(stderr, "usage: mpirun -n <num_procs> mpichess -d <max depth>\n");
      exit(1);
      break;
   }
}

int request_worker()
{
	int worker;
        MPI_Status status;
	MPI_Send(0, 1, MPI_INT, MANAGER, 0, MPI_COMM_WORLD);
	MPI_Recv(&worker, 1, MPI_INT, MANAGER, 0, MPI_COMM_WORLD, &status);
	return worker;
}

int assign_work(int worker, int alpha, int beta, int depth, board_t board, int side, int ply, int follow_pv)
{
	int errorcode;
	int data[] = {alpha, beta, depth, side, ply, follow_pv};
	errorcode = MPI_Send(data, 6, MPI_INT, worker, 3, MPI_COMM_WORLD);
	errorcode += MPI_Send(board.color, 64, MPI_INT, worker, 4, MPI_COMM_WORLD);
	errorcode += MPI_Send(board.piece, 64, MPI_INT, worker, 5, MPI_COMM_WORLD);
	errorcode += MPI_Send(history, 4096, MPI_INT, worker, 6, MPI_COMM_WORLD);
	
	return errorcode;
}
