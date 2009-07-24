/*
 * PXChess
 * 
 *	main.c
 *
 *  This program is an implementation of a chess engine, this engine supports
 *  the xboard protocol and searches using the alpha beta heuristic.
 *  This is a very basic implementation and does not support features
 *  such as transposition tables, interative deepening, or killer moves
 *  It does however have a quiescence search.
 *
 * Version 1.551
 *  
 */



#include "common.h"
#include <time.h>

int main(int argc, char *argv[])
{
    int mov;    //Total number of moves made
    int side;       //Current side
    board_t board;  //Local board representation
    move m; //Temporary variable for holding the current move
    
    parseArgs(argc, argv);

    side = WHITE;
    init_board(&board); //Initialize the board to initial game state
    srand(time(0));    //Seed the random number generator
    mov = 0;  //Total moves made is 0

    for (;;)
    {
       m.u = pickbestmove(board,side);  //Store the move in m
       if (m.u == -1) {
           print_board(board);
           printf("The game has ended\n");
           break;
       }

       makeourmove(board, m.b, &board, side);
		
      mov++;	//Update the move counter
      print_board(board);	//Print the board to screen
      side ^= 1;	//Switch sides
      continue;
    } //end for(;;)


    return 0;
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
        case WHITE:
            printf(" %c", piece_char[board.piece[i]]);
            break;
        case BLACK:
            printf(" %c", piece_char[board.piece[i]] + ('a' - 'A'));
            break;
        }
        if ((i + 1) % 8 == 0 && i != 63)
            printf("\n%d ", 7 - ROW(i));
    }
    printf("\n\n   a b c d e f g h\n\n");
}

/* parseArgs() parses the arguments and loads them into local variables */

void parseArgs(int argc, char **argv)
{
 int c;
 while ((c = getopt(argc, argv, "w:b:")) != -1)  //Loop for parsing arguments
   switch (c)
   {
     case 'w':
      depth[WHITE] = atoi(optarg);
      break;
     case 'b':
      depth[BLACK] = atoi(optarg);
      break;
     case '?':
     default:
      fprintf(stderr, "usage: %s -w <white max depth> -b <black max depth>\n", argv[0]);
      exit(1);
      break;
   }
}
