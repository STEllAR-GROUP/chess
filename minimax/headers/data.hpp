/*
 *  DATA.H
 */


/* this is basically a copy of data.c that's included by most
   of the source files so they can use the data.c variables */


/////////////////////////////////////////////////////////////////////////
// Static Global Variables                                             //
/////////////////////////////////////////////////////////////////////////
extern int hash_piece[2][6][64];
extern int hash_side;
extern int hash_ep[64];
extern int mailbox[120];
extern int mailbox64[64];
extern BOOL slide[6];
extern int offsets[6];
extern int offset[6][8];
extern int castle_mask[64];
extern char piece_char[6];
extern int init_color[64];
extern int init_piece[64];
extern int depth[2];
extern int alpha[2];
extern int beta[2];
extern int output;
extern int chosen_evaluator;
extern int number_threads;

////////////////////////////////////////////////////////////////////////////
//State Information -- The global variables this program uses and modifies//
//                     in order to work properly.                         //
////////////////////////////////////////////////////////////////////////////

extern move move_to_make;
