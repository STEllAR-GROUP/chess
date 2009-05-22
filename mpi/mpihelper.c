/*
 * mpihelper.c
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
#include "mpi.h"
#include "defs.h"
#include "data.h"
#include "protos.h"

int mpihelper(int rank)
{
	//TODO: add worker code here
	return 1;
}

int accept_work(int sender)
{
	int errorcode;
	int x;
	MPI_Status status;
	int data[6];
	int history[64][64];
	int best_move;
	board_t board;
	
	memset(pv, 0, sizeof(pv));				//reset the pv array
	memset(pv_length, 0, sizeof(pv_length));
	
	errorcode = MPI_Recv(data, 6, MPI_INT, sender, 3, MPI_COMM_WORLD, &status);				//Tag 3 == search data 1
	errorcode += MPI_Recv(board.color, 64, MPI_INT, sender, 4, MPI_COMM_WORLD, &status);	//Tag 4 == search data 2 (board.color)
	errorcode += MPI_Recv(board.piece, 64, MPI_INT, sender, 5, MPI_COMM_WORLD, &status);	//Tag 5 == search data 3 (board.piece)
	errorcode += MPI_Recv(history, 4096, MPI_INT, sender, 6, MPI_COMM_WORLD, &status);		//Tag 6 == search data 4 (history)
	
	errorcode += MPI_Send(0, 1, MPI_INT, MANAGER, 10, MPI_COMM_WORLD);		//Tag 10 == Request the pv and pv_length arrays
	//This won't work, need to fix: errorcode += MPI_Recv(pv, (sizeof(pv)/sizeof(int)), MPI_INT, MANAGER, 10, MPI_COMM_WORLD, &status);
	errorcode += MPI_Recv(pv_length, (sizeof(pv_length)/sizeof(int)), MPI_INT, MANAGER, 10, MPI_COMM_WORLD, &status);
	
	/*
	alpha = data[0];
	beta = data[1];
	depth = data[2];
	side = data[3];
	ply = data[4];
	follow_pv = data[5];
	*/
	
	x = search(data[0], data[1], data[2], board, data[3], data[4], history, data[5]);
	best_move = pv[0][0].u;
	
	errorcode += MPI_Send(x, 1, MPI_INT, sender, 8, MPI_COMM_WORLD);	//Tag 8 == request to send search score
	if (data[4] == 0)
		errorcode += MPI_Send(best_move, 1, MPI_INT, sender, 13, MPI_COMM_WORLD);	//Tag 13 == send best move
	
	return errorcode;
}
