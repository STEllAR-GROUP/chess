/*
 *	BOARD.C
 *	Tom Kerrigan's Simple Chess Program (TSCP)
 *
 *	Copyright 1997 Tom Kerrigan
 *
 *	Modified by Phillip LeBlanc
 */

#include <stdio.h>
#include <stdlib.h>
#include "defs.h"
#include "data.h"
#include "protos.h"


int set_hash(board_t board, int side);

/* init_board() sets the board to the initial game state. */

void init_board(board_t *board)
{
	int i;

	for (i = 0; i < 64; ++i) {
		board->color[i] = init_color[i];
		board->piece[i] = init_piece[i];
	}
}


/* in_check() returns TRUE if side s is in check and FALSE
   otherwise. It just scans the board to find side s's king
   and calls attack() to see if it's being attacked. */

BOOL in_check(board_t board, int s)
{
	int i;

	for (i = 0; i < 64; ++i)
		if (board.piece[i] == KING && board.color[i] == s)
			return attack(board, i, s ^ 1);
	return TRUE;  /* shouldn't get here */
}


/* attack() returns TRUE if square sq is being attacked by side
   s and FALSE otherwise. */

BOOL attack(board_t board, int sq, int s)
{
	int i, j, n;

	for (i = 0; i < 64; ++i)
		if (board.color[i] == s) {
			if (board.piece[i] == PAWN) {
				if (s == LIGHT) {
					if (COL(i) != 0 && i - 9 == sq)
						return TRUE;
					if (COL(i) != 7 && i - 7 == sq)
						return TRUE;
				}
				else {
					if (COL(i) != 0 && i + 7 == sq)
						return TRUE;
					if (COL(i) != 7 && i + 9 == sq)
						return TRUE;
				}
			}
			else
				for (j = 0; j < offsets[board.piece[i]]; ++j)
					for (n = i;;) {
						n = mailbox[mailbox64[n] + offset[board.piece[i]][j]];
						if (n == -1)
							break;
						if (n == sq)
							return TRUE;
						if (board.color[n] != EMPTY)
							break;
						if (!slide[board.piece[i]])
							break;
					}
		}
	return FALSE;
}


/* genmoves() generates pseudo-legal moves for the current position.
   It scans the board to find friendly pieces and then determines
   what squares they attack. When it finds a piece/square
   combination, it calls gen_push to put the move on the "move
   stack." */

int genmoves(board_t board, gen_t *g, int side, int history[64][64])
{
	int i, j, n;

	int totalmoves = 0;

	for (i = 0; i < 64; ++i)
		if (board.color[i] == side) {
			if (board.piece[i] == PAWN) {
				if (side == LIGHT) {
					if (COL(i) != 0 && board.color[i - 9] == DARK)
						gen_push(i, i - 9, 17, g, totalmoves++, board, side, history);
					if (COL(i) != 7 && board.color[i - 7] == DARK)
						gen_push(i, i - 7, 17, g, totalmoves++, board, side, history);
					if (board.color[i - 8] == EMPTY) {
						gen_push(i, i - 8, 16, g, totalmoves++, board, side, history);
						if (i >= 48 && board.color[i - 16] == EMPTY)
							gen_push(i, i - 16, 24, g, totalmoves++, board, side, history);
					}
				}
				else {
					if (COL(i) != 0 && board.color[i + 7] == LIGHT)
						gen_push(i, i + 7, 17, g, totalmoves++, board, side, history);
					if (COL(i) != 7 && board.color[i + 9] == LIGHT)
						gen_push(i, i + 9, 17, g, totalmoves++, board, side, history);
					if (board.color[i + 8] == EMPTY) {
						gen_push(i, i + 8, 16, g, totalmoves++, board, side, history);
						if (i <= 15 && board.color[i + 16] == EMPTY)
							gen_push(i, i + 16, 24, g, totalmoves++, board, side, history);
					}
				}
			}
			else
				for (j = 0; j < offsets[board.piece[i]]; ++j)
					for (n = i;;) {
						n = mailbox[mailbox64[n] + offset[board.piece[i]][j]];
						if (n == -1)
							break;
						if (board.color[n] != EMPTY) {
							if (board.color[n] == (side^1))
								gen_push(i, n, 1, g, totalmoves++, board, side, history);
							break;
						}
						gen_push(i, n, 0, g, totalmoves++, board, side, history);
						if (!slide[board.piece[i]])
							break;
					}
		}

	return totalmoves;
}

/* gen_caps() is basically a copy of genmoves() that's modified to
 only generate capture and promote moves. It's used by the
 quiescence search. */

int gen_caps(board_t board, gen_t *g, int side, int history[64][64])
{
	int i, j, n;
	
	int totalmoves = 0;
	
	for (i = 0; i < 64; ++i)
		if (board.color[i] == side) {
			if (board.piece[i]==PAWN) {
				if (side == LIGHT) {
					if (COL(i) != 0 && board.color[i - 9] == DARK)
						gen_push(i, i - 9, 17, g, totalmoves++, board, side, history);
					if (COL(i) != 7 && board.color[i - 7] == DARK)
						gen_push(i, i - 7, 17, g, totalmoves++, board, side, history);
					if (i <= 15 && board.color[i - 8] == EMPTY)
						gen_push(i, i - 8, 16, g, totalmoves++, board, side, history);
				}
				if (side == DARK) {
					if (COL(i) != 0 && board.color[i + 7] == LIGHT)
						gen_push(i, i + 7, 17, g, totalmoves++, board, side, history);
					if (COL(i) != 7 && board.color[i + 9] == LIGHT)
						gen_push(i, i + 9, 17, g, totalmoves++, board, side, history);
					if (i >= 48 && board.color[i + 8] == EMPTY)
						gen_push(i, i + 8, 16, g, totalmoves++, board, side, history);
				}
			}
			else
				for (j = 0; j < offsets[board.piece[i]]; ++j)
					for (n = i;;) {
						n = mailbox[mailbox64[n] + offset[board.piece[i]][j]];
						if (n == -1)
							break;
						if (board.color[n] != EMPTY) {
							if (board.color[n] == (side ^ 1))
								gen_push(i, n, 1, g, totalmoves++, board, side, history);
							break;
						}
						if (!slide[board.piece[i]])
							break;
					}
		}
	
	return totalmoves;
}


/* gen_push() puts a move on the move stack, unless it's a
   pawn promotion that needs to be handled by gen_promote().
   It also assigns a score to the move for alpha-beta move
   ordering. If the move is a capture, it uses MVV/LVA
   (Most Valuable Victim/Least Valuable Attacker). Otherwise,
   it uses the move's history heuristic value. Note that
   1,000,000 is added to a capture move's score, so it
   always gets ordered above a "normal" move. */

void gen_push(int from, int to, int bits, gen_t *g, int index, board_t board, int side, int history[64][64])
{
	
	if (bits & 16) {
		if (side == LIGHT) {
			if (to <= H8) {
				gen_promote(from, to, bits, g, index);
				return;
			}
		}
		else {
			if (to >= A1) {
				gen_promote(from, to, bits, g, index);
				return;
			}
		}
	}


	g[index].m.b.from = (char)from;
	g[index].m.b.to = (char)to;
	g[index].m.b.promote = 0;
	g[index].m.b.bits = (char)bits;
	if (board.color[to] != EMPTY)
		g[index].score = 1000000 + (board.piece[to] * 10) - board.piece[from];
	else
		g[index].score = history[from][to];
}


/* gen_promote() is just like gen_push(), only it puts 4 moves
   on the move stack, one for each possible promotion piece */

void gen_promote(int from, int to, int bits, gen_t *g, int index)
{
	int i;
	
	for (i = KNIGHT; i <= QUEEN; ++i) {
		g[index].m.b.from = (char)from;
		g[index].m.b.to = (char)to;
		g[index].m.b.promote = (char)i;
		g[index].m.b.bits = (char)(bits | 32);
		g[index].score = 1000000 + (i * 10);
	}
}


/* makeourmove() makes a move and stores it into newboard. If the move is illegal, it
   returns FALSE. Otherwise, it returns TRUE. */

BOOL makeourmove(board_t board, move_bytes m, board_t *newboard, int side)
{

	*newboard = board;
	
	/* move the piece */
	newboard->color[(int)m.to] = side;
	if (m.bits & 32)
		newboard->piece[(int)m.to] = m.promote;
	else
		newboard->piece[(int)m.to] = newboard->piece[(int)m.from];
	newboard->color[(int)m.from] = EMPTY;
	newboard->piece[(int)m.from] = EMPTY;

	/* switch sides and test for legality (if we can capture
	   the other guy's king, it's an illegal position and
	   we need to take the move back) */
	if (in_check(*newboard, side))
		return FALSE;
	return TRUE;
}


int hash_piece[2][6][64]; //Indexed by piece[color][type][square]
int hash_side;


/* init_hash() initializes the random numbers used by set_hash(). */

void init_hash()
{
    int i, j, k;

    srand(0);
    for (i = 0; i < 2; ++i)
            for (j=0; j < 6; ++j)
                for (k = 0; k < 64; ++k)
                    hash_piece[i][j][k] = hash_rand();

    hash_side = hash_rand();
}

/* hash_rand() XORs some shifted random numbers together to make sure
 *  we have good coverage of all 32 bits. (rand() returns 16-bit numbers
 *  on some systems.) */

int hash_rand()
{
    int i;
    int r = 0;

    for (i = 0; i < 32; ++i)
        r ^= rand() << i;
    return r;
}

/* set_hast() uses the Zobrist method of generating a unique number (hash)
   for the current chess position. Of course, there are many more chess
   positions than there are 32 bit numbers, so the numbers generated are
   not really unique, but they're unique enough for our purposes (to detect
   repetitions of the position).
   The way it works is to XOR random numbers that correspond to features of
   the position, e.g., if there's a black knight on B8, hash is XORed with
   hash_piece[BLACK][KNIGHT][B8]. All of the pieces are XORed together,
   hash_side is XORed if it's black's move. */

int set_hash(board_t board, int side)
{
    int i;
    int hash = 0;

    for (i = 0; i < 64; ++i)
        if (board.color[i] != EMPTY)
            hash ^= hash_piece[board.color[i]][board.piece[i]][i];
    if (side == DARK)
        hash ^= hash_side;

    return hash;
}
