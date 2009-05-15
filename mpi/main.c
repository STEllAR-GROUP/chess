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
#include "mpi.h"
#include "defs.h"
#include "protos.h"


int main(int argc, char *argv[])
{
	int myrank, num_procs;
	
	MPI_Init(&argc, &argv);
	
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
	
	if (num_procs < 4)
	{
		printf("This program needs at least 4 processors to run");
		MPI_Finalize();
		return -1;
	}

	if (myrank == 0)
    	mpi(LIGHT, argc, &argv);
    else if (myrank == 1)
    	mpi(DARK, argc, &argv);
    else if (myrank == 2)
    	mpimanager(num_procs);
    else
    	mpihelper(myrank);

    MPI_Finalize();
    
    return 0;
}