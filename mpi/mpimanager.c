/*
 * mpimanager.c
 * The code that manages the available resources
 * Uses the concept of a stack to manage next available nodes
 * Created by Phillip LeBlanc
 */
 
#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"
#include "protos.h"
#include "defs.h"
#include "data.h"

#define FIRST 3

int *avail_ranks;
int ptr;	//Pointer to the last spot in the array with an available node

int mpimanager(int size)
{
	setup_rank(size);
	while (TRUE)
	{
		//TODO: add manager code here
	}
	return 1;
}

void setup_rank(int size)
{
	int i;
	avail_ranks = malloc((size-FIRST)*sizeof(int));
	for (i = FIRST; i < size; i++)
		avail_ranks[i-FIRST] = i;
	ptr = size-FIRST;
}

int get_next_rank()	//aka pop(), using LIFO
{
	if (ptr == -1)
		return -1;
	int to_return = avail_ranks[ptr];
	avail_ranks[ptr] = NULL;
	ptr--;
	return to_return;
}