/*
 *	DATA.H
 *	Tom Kerrigan's Simple Chess Program (TSCP)
 *
 *	Copyright 1997 Tom Kerrigan
 */


/* this is basically a copy of data.c that's included by most
   of the source files so they can use the data.c variables */

extern int max_time;
extern int start_time;
extern int stop_time;
extern int nodes;
extern int tot_nodes;
extern int mailbox[120];
extern int mailbox64[64];
extern BOOL slide[6];
extern int offsets[6];
extern int offset[6][8];
extern char piece_char[6];
extern int init_color[64];
extern int init_piece[64];
extern int start_time_main;

extern move pv[MAX_PLY][MAX_PLY];
extern int pv_length[MAX_PLY];
