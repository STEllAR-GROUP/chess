/*
 *	PROTOS.H
 *	
 *  Collection of the prototypes for the chess program
 *
 */

#ifndef PROTOS_H
#define PROTOS_H

/* prototypes */

/* board.c */
void init_board(board_t *board);
BOOL in_check(board_t board, int s);
BOOL attack(board_t board, int sq, int s);
int genmoves(board_t board, movestack *g, int side);
int gen_caps(board_t board, movestack *g, int side);
void gen_push(int from, int to, int bits, movestack *g, int index, board_t board, int side);
void gen_promote(int from, int to, int bits, movestack *g, int index);
BOOL makeourmove(board_t board, move_bytes m, board_t *newboard, int side);

/* search2.c */
int pickbestmove(board_t board, int side);
int search(int alpha, int beta, int depth, board_t board, int side);
void sort(movestack *moves, int last_move);

/* eval.c */
int eval(board_t board, int side);
int eval_light_pawn(int sq);
int eval_dark_pawn(int sq);
int eval_light_king(int sq);
int eval_lkp(int f);
int eval_dark_king(int sq);
int eval_dkp(int f);

/* main.c */
int main();
void parseArgs(int argc, char **argv);
void print_board(board_t board);

#endif
