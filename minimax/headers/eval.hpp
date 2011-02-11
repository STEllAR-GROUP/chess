#ifndef EVAL_H
#define EVAL_H


#include "node.hpp"
#include "defs.hpp"
#include "data.hpp"


#define DOUBLED_PAWN_PENALTY		10
#define ISOLATED_PAWN_PENALTY		20
#define BACKWARDS_PAWN_PENALTY		8
#define PASSED_PAWN_BONUS			20
#define ROOK_SEMI_OPEN_FILE_BONUS	10
#define ROOK_OPEN_FILE_BONUS		15
#define ROOK_ON_SEVENTH_BONUS		20

struct evaluator {
    int pawn_rank[2][10];
    int piece_mat[2];  // the value of a side's pieces
    int pawn_mat[2];  // the value of a side's pawns

	int eval(node_t& board, int evaluator);
	int eval_simple(node_t& board);
	int eval_orig(node_t& board);
	int eval_light_pawn(int sq);
	int eval_dark_pawn(int sq);
	int eval_light_king(int sq);
	int eval_lkp(int f);
	int eval_dark_king(int sq);
	int eval_dkp(int f);
};

#endif
