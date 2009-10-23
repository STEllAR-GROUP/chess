#include "common.h"

int starting_board; //If the file already contains n number of boards, start at n+1
int last_board; //number of the last board in the file
#define BOARD_INDEX 3
FILE *board_file; //Initialize the board file

//TODO: Add a facility for seeking to an arbitrary marker (im microkernel)
//TODO: In the sig_int function, make sure to write the appropriate header on the
//	board file and close it out properly before exiting

void board_file_print(board_t board)
{
  fseek(board_file, 0L, SEEK_END); //Make sure we are at the end of the file
  fprintf(board_file, "[%d]\n", last_board);
  last_board++;
  int i;
  for (i = 0; i < 64; i++)
	fprintf(board_file, "%d ", board.color[i]);
  fprintf(board_file, "\n");
  for (i = 0; i < 64; i++)
	fprintf(board_file, "%d ", board.piece[i]);
  fprintf(board_file, "\n");
  fflush(board_file);
}

FILE* init_board_file()
{
  board_file = fopen("board_list.txt", "r");
  if (!board_file)
  {
	board_file = fopen("board_list.txt", "w");
	starting_board = 1;
	last_board = 1;
	return board_file;
  }
  system("wc -l board_list.txt > bc");
  FILE* bc = fopen("bc", "r");
  fscanf(bc, "%d", &starting_board);
  last_board = (starting_board/3) + 1;
  board_file = fopen("board_list.txt", "a");
  return board_file;
}	
  
int get_starting_board()
{
  return starting_board;
}

//Call this function when the user interrupts or the end of the program
int close_board_file()
{
  //fseek(board_file, 0L, SEEK_SET); //Move file position indicator to the beginning
  //fprintf(board_file, "<<%d>>\n", last_board); //Write header file
  return fclose(board_file);
}
