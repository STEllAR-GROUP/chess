/*
 *	search2.c
 *	A collection of related search functions for the
 *	parallex chess program.
 *
 *	Phillip LeBlanc, Student Worker
 *	LSU CCT
*/

#include <stdio.h>
#include <string.h>
#include "defs.h"
#include "data.h"
#include "protos.h"

int usebookmove = 1;

char *move_str(move_bytes m);

int pickbestmove(board_t board, int max_depth, int side, int mov, hist_t *hist)
{
	int i, j, x;

	move m;
	if (usebookmove) {
            m.u = book_move(hist);
	    if (m.u != -1)
		return m.u;
            else
                usebookmove = 0;
        } //end if(usebookmove)
	
	tot_nodes += nodes;
	nodes = 0;

	int history[64][64];
	memset(history, 0, sizeof(history));
	memset(pv, 0, sizeof(pv));
	memset(pv_length, 0, sizeof(pv_length));

        check_repeating_moves(hist, history, board, side);
	
	printf("ply      nodes  score  pv\n");
	for (i = 1; i <= max_depth; ++i) {	//This is known as iterative deepening
		
		x = search(-10000, 10000, i, board, side, 0, history, TRUE);
		printf("%3d  %9d  %5d ", i , nodes, x);
		for (j = 0; j < pv_length[0]; ++j)
			printf(" %s", move_str(pv[0][j].b));
		printf("\n");
		fflush(stdout);
		if (x > 9000 || x < -9000)
			return pv[0][0].u;
	} //end for()
	
	//x = search(-10000, 10000, max_depth, board, side, 0);

	return pv[0][0].u;

}



int search(int alpha, int beta, int depth, board_t board, int side, int ply, int history[64][64], BOOL follow_pv)
{
	int lastmove, i, j,  move_score;
	BOOL c, lm;
	board_t newmove;
	
	if (!depth)
		return quiesce(alpha, beta, board, side, ply, history, follow_pv);
	
	pv_length[ply] = ply;
	
	if (ply >= MAX_PLY - 1)
		return eval(board, side);

	/* are we in check? if so, we want to search deeper */
	c = in_check(board, side);
	if (c)
		++depth;

	gen_t legal_moves[GEN_STACK];	//data to hold all of the pseudo-legal moves for this board
	lastmove = genmoves(board, &legal_moves, side, history);	//generate and store all of the pseudo-legal moves

	if (follow_pv)  /* are we following the PV? */
		follow_pv = sort_pv(&legal_moves, ply, lastmove);

	lm = FALSE;	//assume we have no legal moves

	for (i = 0; i < lastmove; i++) {
		sort(&legal_moves, lastmove); //Try to get the best move first
		if(!makeourmove(board, legal_moves[i].m.b, &newmove, side))	//Make the move, store it into newmove. Test for legality
			continue;	//If this move isn't legal, move onto the next one
		move_score = -search(-beta, -alpha, depth - 1, newmove, side ^ 1, ply + 1, history, follow_pv);	//Search again with this move to see opponent's responses
		
		lm = TRUE;	//We have at least one legal move

		if (move_score > alpha) {
			
			/* this move caused a cutoff, so increase the history
			   value so it gets ordered high next time we can serach
			   it */
			
			history[(int)legal_moves[i].m.b.from][(int)legal_moves[i].m.b.to] += depth;
			if (move_score >= beta)
				return beta;
			alpha = move_score;
			
			pv[ply][ply] = legal_moves[i].m;
			for (j = ply + 1; j < pv_length[ply+1]; ++j)
				pv[ply][j] = pv[ply + 1][j];
			pv_length[ply] = pv_length[ply + 1];
			
		} //end if(move_score > alpha)
	} //end for()

	if (lm==FALSE) {	//No legal moves, check for stalemate or checkmate
		if (c)
                {
                    pv[ply][ply].u = -1;
                    return -10000 + ply;
                }
		else
			return 0;
	}
	
	return alpha;
}

/* quiesce() is a recursive minimax search function with
 alpha-beta cutoffs. In other words, negamax. It basically
 only searches capture sequences and allows the evaluation
 function to cut the search off (and set alpha). The idea
 is to find a position where there isn't a lot going on
 so the static evaluation function will work. */

int quiesce(int alpha,int beta, board_t board, int side, int ply, int history[64][64], BOOL follow_pv)
{
	int i, j, x, lastmove;
	
	board_t newmove;
	
	++nodes;
	
	pv_length[ply] = ply;
	
	/* are we too deep? */
	if (ply >= MAX_PLY - 1)
		return eval(board, side);
	
	/* check with the evaluation function */
	x = eval(board, side);
	if (x >= beta)
		return beta;
	if (x > alpha)
		alpha = x;
	
	gen_t legal_moves[GEN_STACK];	//data to hold all of the pseudo-legal moves for this board
	lastmove = gen_caps(board, &legal_moves, side, history);	//generate and store all of the pseudo-legal moves

	if (follow_pv)  /* are we following the PV? */
		follow_pv = sort_pv(&legal_moves, ply, lastmove);
	
	/* loop through the moves */
	for (i = 0; i < lastmove; ++i) {
		sort(&legal_moves, lastmove); //Try to get the best move first
		if(!makeourmove(board, legal_moves[i].m.b, &newmove, side))	//Make the move, store it into newmove. Test for legality
			continue;	//If this move isn't legal, move onto the next one
		x = -quiesce(-beta, -alpha, newmove, side ^ 1, ply+1, history, follow_pv);
		if (x > alpha) {
			if (x >= beta)
				return beta;
			alpha = x;
			
			/* update the PV */
			pv[ply][ply] = legal_moves[i].m;
			for (j = ply + 1; j < pv_length[ply+1]; ++j)
				pv[ply][j] = pv[ply + 1][j];
			pv_length[ply] = pv_length[ply + 1];
		}
	}
	return alpha;
}


/* sort() searches the current ply's move list
 to the end to find the move with the highest score. Then it
 swaps that move and the 'from' move so the move with the
 highest score gets searched next, and hopefully produces
 a cutoff. */

void sort(gen_t *gen_dat, int last_move)
{
	int i;
	int bs;  /* best score */
	int bi;  /* best i */
	gen_t g;
	
	bs = -1;
	bi = 0;
	for (i = 0; i < last_move; ++i)
		if (gen_dat[i].score > bs) {
			bs = gen_dat[i].score;
			bi = i;
		}
	g = gen_dat[0];
	gen_dat[0] = gen_dat[bi];
	gen_dat[bi] = g;
	
}

/* sort_pv() is called when the search function is following
 the PV (Principal Variation). It looks through the current
 ply's move list to see if the PV move is there. If so,
 it adds 10,000,000 to the move's score so it's played first
 by the search function. If not, follow_pv remains FALSE and
 search() stops calling sort_pv(). */

BOOL sort_pv(gen_t *gen_dat, int ply, int lastmove)
{
	int i;
	
	for(i = 0; i < lastmove; ++i)
		if (gen_dat[i].m.u == pv[0][ply].u) {
			gen_dat[i].score += 10000000;
			return TRUE;
		}
	return FALSE;
}


void check_repeating_moves(hist_t *hist, int history[64][64], board_t board, int side)
{
    int threshhold = 3; //How many repeating moves before we discourage that move
    BOOL done = FALSE;
    int hindex = 0;
    int repeats = 0;
    int index;

    while (!done)
    {
        if(hist[hindex].m.u != 0)
            hindex++;
        else
            done = TRUE;
    }

    if (hindex < 10)
        return;
    
    index = hindex;
    done = FALSE;

    int hash = set_hash(board, side);

    while (!done)
    {
        if (hist[index].hash == hash)
            repeats += 4;
        else
            done = TRUE;
        if (repeats >= threshhold)
            done = TRUE;
        index--;
    }


    //TODO: add code that modifies history[64][64] to have -10000 as the score
    //for repeating moves
    for (index = hindex; hindex-repeats < index ; index--)
        history[(int)hist[index].m.b.from][(int)hist[index].m.b.to] = -100000;

}
