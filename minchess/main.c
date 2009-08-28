/*
 * MINChess
 * 
 *	main.c
 *
 *  This program is a minimal implementation of a chess program
 * using an alpha-beta search algorithm
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
           //printf("The game has ended\n");
           break;
       }

       makeourmove(board, m.b, &board, side);
		
      mov++;	//Update the move counter
      printf("Move #: %d\n", mov); //So we know where the game is when testing
      print_board(board);	//Print the board to screen
      side ^= 1;	//Switch sides
	  
	  if (mov > 100)	//Assume king vs king endgame
		exit(0);
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
    if (argc < 9)
    {
         fprintf(stderr, "usage: %s -w <white max depth> <alpha> <beta> -b <black max depth> <alpha> <beta>\n", argv[0]);
         exit(2);
    }
      depth[WHITE] = atoi(argv[2]);
      alpha[WHITE] = atoi(argv[3]);
      beta[WHITE] = atoi(argv[4]);
      depth[BLACK] = atoi(argv[6]);
      alpha[BLACK] = atoi(argv[7]);
      beta[BLACK] = atoi(argv[8]);
#ifdef _OPENMP
      omp_set_num_threads(atoi(argv[10]));
#endif
}
