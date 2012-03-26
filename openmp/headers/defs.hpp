////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2011 Steve Brandt and Philip LeBlanc
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file BOOST_LICENSE_1_0.rst or copy at http://www.boost.org/LICENSE_1_0.txt)
////////////////////////////////////////////////////////////////////////////////
/*
 *  DEFS.H
 */
#include <stdint.h>
 
#ifndef DEFS_H
#define DEFS_H

#define LIGHT           0
#define DARK            1

#define PAWN            0
#define KNIGHT          1
#define BISHOP          2
#define ROOK            3
#define QUEEN           4
#define KING            5

#define EMPTY           6

// Evaluator Defs
#define ORIGINAL        0
#define SIMPLE          1

// Search method Defs
#define MINIMAX         0
#define ALPHABETA       1
#define MTDF            2
#define MULTISTRIKE     3

/* useful squares */
#define A1_CHESS              56
#define B1_CHESS              57
#define C1_CHESS              58
#define D1_CHESS              59
#define E1_CHESS              60
#define F1_CHESS              61
#define G1_CHESS              62
#define H1_CHESS              63
#define A8_CHESS              0
#define B8_CHESS              1
#define C8_CHESS              2
#define D8_CHESS              3
#define E8_CHESS              4
#define F8_CHESS              5
#define G8_CHESS              6
#define H8_CHESS              7

#define ROW(x)          (x >> 3)
#define COL(x)          (x & 7)


/* This is the basic description of a chess_move. promote is what
   piece to promote the pawn to, if the chess_move is a pawn
   promotion. bits is a bitfield that describes the chess_move,
   with the following bits:

   1    capture
   2    castle
   4    en passant capture
   8    pushing a pawn 2 squares
   16   pawn chess_move
   32   promote

   It's union'ed with an integer so two moves can easily
   be compared with each other. */

typedef struct {
  int8_t from;
  int8_t to;
  int8_t promote;
  int8_t bits;
} move_bytes;

typedef union {
  move_bytes b;
  int32_t u;
} chess_move;

const int INVALID_MOVE = 0xFFFFFFFF;

#endif
