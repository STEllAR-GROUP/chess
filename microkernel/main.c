#include "common.h"
#include <time.h>

int main(int argc, char *argv[])
{
	printf("Starting MPI microkernel....\n\n");

	//Need to set up initial game conditions

	int depth = 3;
	if (argc > 1)
		depth = atoi(argv[1]);	

	int mov;
	int side;
	board_t board;

	side = WHITE;
	init_board(&board);
	srand(time(0));
	mov = 0;

	/*********GAME LOOP*********/
	while (mov < 200)
	{
	//	printf("Move number: %d\n", mov);
		//gen_random_board(&board, &side);
		//Insert any timing functions here
		alpha_beta(board, -10000, 10000, depth, side, side);
		mov++;
	}

	return 1;	

}

int alpha_beta(board_t board, int alpha, int beta, int depth, int side, int rootside)
{
	if (depth <= 0)
		return eval(board, side);

	movestack legal_moves[MAX_MOVES];

	int width = genmoves(board, legal_moves, side);

	if (width == 0)
		return eval(board, side);

	int cutoff = FALSE;
	int i = 1;
	int val;
	board_t newboard;

	while ((i <= width)&&(cutoff==FALSE))
	{
		if (!makeourmove(board, legal_moves[i].m.b, &newboard, side))
		{
			i++;
			continue;
		}
		val = alpha_beta(newboard, alpha, beta, depth - 1, side ^ 1, rootside);
		if ((side==rootside)&&(val > alpha))
			alpha = val;
		else if ((side!=rootside)&&(val < beta))
			beta = val;
		if (alpha > beta)
			cutoff = TRUE;
		i++;
		//printf("while loop in alpha beta, i = %d\n", i);
	}
	if (side==rootside)
		return alpha;
	else
		return beta;
}
