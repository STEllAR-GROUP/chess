#include "common.h"
#include <file.h>

int starting_board; //If the file already contains n number of boards, start at n+1
int last_board; //number of the last board in the file
#define BOARD_INDEX 3

//TODO: Add a facility for seeking to an arbitrary marker (im microkernel)
//TODO: In the sig_int function, make sure to write the appropriate header on the
//	board file and close it out properly before exiting

void board_file_print(FILE *board_file, board_t board)
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
}

FILE* init_board_file()
{
  FILE *board_file; //Initialize the board file
  board_file = fopen("board_list.txt", "r+"); //Open the file for reading&writing
  char buffer[100]; //Create the buffer file
  if (fgets(buffer, sizeof(buffer), board_file) == NULL) //Read in the first line
  {
	starting_board = 1;
	last_board = 1;
	return board_file;
  }
  
  sscanf(buffer, "%d", &starting_board);
  last_board = starting_board;
  return board_file;
}	
  
int get_starting_board()
{
  return starting_board;
}
