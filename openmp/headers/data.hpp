#ifndef DATA_HPP
#define DATA_HPP
#include "hash.hpp"
#include <assert.h>
#include "here.hpp"
#include <iostream>
#include <stdlib.h>
#include "Fixed.hpp"
/*
 *  data.hpp
 */

/* this is basically a copy of data.cpp that's included by most
   of the source files so they can use the data.cpp variables */


/////////////////////////////////////////////////////////////////////////
// Static Global Variables                                             //
/////////////////////////////////////////////////////////////////////////
extern hash_t hash_piece[2][6][64];
extern hash_t hash_side;
extern hash_t hash_ep[64];
extern FixedArray<int,120> mailbox;
extern FixedArray<int,64> mailbox64;
extern bool slide[6];
extern FixedArray<int,6> offsets;
extern int offset[6][8];
extern int castle_mask[64];
extern char piece_char[6];
extern int init_color[64];
extern int init_piece[64];
extern int depth[2];
extern int output;
extern int chosen_evaluator;
extern int search_method;
extern int iter_depth;
extern hash_t rnum[];
extern bool bench_mode;
extern bool logging_enabled;
extern int mpi_depth;

////////////////////////////////////////////////////////////////////////////
//State Information -- The global variables this program uses and modifies//
//                     in order to work properly.                         //
////////////////////////////////////////////////////////////////////////////

extern move move_to_make;

void do_data();
#endif
