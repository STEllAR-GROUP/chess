/*
 *	data.h
 */

#ifndef DATA_H
#define DATA_H
/* this is basically a copy of data.c that's included by most
   of the source files so they can use the data.c variables */

extern int mailbox[120];
extern int mailbox64[64];
extern BOOL slide[6];
extern int offsets[6];
extern int offset[6][8];
extern char piece_char[6];
extern int init_color[64];
extern int init_piece[64];

extern int depth[2];
extern int alpha[2];
extern int beta[2];

#endif
