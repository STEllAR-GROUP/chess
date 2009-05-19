/*
 * mpimanager.c
 * The code that manages the available resources
 * Uses the concept of a stack to manage next available nodes
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
#include "mpi.h"
#include "protos.h"
#include "defs.h"
#include "data.h"

#define FIRST 3
#define STACK_MAX 1000

typedef struct {
    int     data[STACK_MAX];
    int     size;
} Stack;

int mpimanager(int size)
{
	Stack ranks;
	MPI_Status status;
	int msg;
	setup_rank(size, *ranks);
	while (TRUE)
	{
		MPI_Recv(&msg, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		
		switch (status.MPI_TAG)
		{
			case 0:
				msg = Stack_Top(*ranks);
				Stack_Pop(*ranks);
				MPI_Send(msg, 1, MPI_INT, status.MPI_SOURCE, 0, MPI_COMM_WORLD);
				break;
			case 1:
				Stack_Push(*ranks, status.MPI_SOURCE);
				break;
			case 2:
				return 1;
		}
	}
	return 1;	//Shouldn't get here
}

void setup_rank(int size, Stack *ranks)
{
	int i;
	Stack_Init(ranks);
	for (i = FIRST; i < size; i++)
		Stack_Push(ranks,i);
}

void Stack_Init(Stack *S)
{
    S->size = 0;
}

int Stack_Top(Stack *S)
{
    if (S->size == 0)
        return -1;
    return S->data[S->size-1];
}

void Stack_Push(Stack *S, int d)
{
    if (S->size < STACK_MAX)
        S->data[S->size++] = d;
    else
        fprintf(stderr, "Error: stack full\n");
}

void Stack_Pop(Stack *S)
{
	if (S->size > 0)
    	S->size--;
}

