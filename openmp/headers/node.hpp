#ifndef NODE_H
#define NODE_H
#include <iostream>
#include <vector>
#include <stdlib.h>
#include <stdio.h>
#include "defs.hpp"
#include "data.hpp"

struct hist_t {
    move m;
    int capture;
    int castle;
    int ep;
    int fifty;
    int hash;
};

class node_t {
    public:
    node_t() {
        
        /* constructor sets the board to the initial game state. */
        int i;

        for (i = 0; i < 64; ++i) {
            color[i] = init_color[i];
            piece[i] = init_piece[i];
        }
        side = LIGHT;
        castle = 15;
        ep = -1;
        fifty = 0;
        ply = 0;
        hply = 0;
        hist_dat.resize(10);
        init_hash();
        set_hash();  
    }

    // Copy Constructor
    node_t(const node_t& n) {
        int i;

        for (i = 0; i < 64; ++i) {
            color[i] = n.color[i];
            piece[i] = n.piece[i];
        }
        side = n.side;
        castle = n.castle;
        ep = n.ep;
        fifty = n.fifty;
        ply = n.ply;
        hply = n.hply;
        hist_dat = n.hist_dat;
        hash = n.hash;
    }

    std::vector<hist_t> hist_dat;
    int hash;
    char color[64];
    char piece[64];
    int depth;
    int side;
    int castle;
    int ep;
    int fifty;
    int ply;
    int hply;

    /* set_hash() uses the Zobrist method of generating a unique number (hash)
       for the current chess position. Of course, there are many more chess
       positions than there are 32 bit numbers, so the numbers generated are
       not really unique, but they're unique enough for our purposes (to detect
       repetitions of the position). 
       The way it works is to XOR random numbers that correspond to features of
       the position, e.g., if there's a black knight on B8, hash is XORed with
       hash_piece[BLACK][KNIGHT][B8]. All of the pieces are XORed together,
       hash_side is XORed if it's black's move, and the en passant square is
       XORed if there is one. (A chess technicality is that one position can't
       be a repetition of another if the en passant state is different.) */
    void set_hash()
    {
        int i;

        hash = 0;   
        for (i = 0; i < 64; ++i)
            if (color[i] != EMPTY)
                hash ^= hash_piece[color[i]][piece[i]][i];
        if (side == DARK)
            hash ^= hash_side;
        if (ep != -1)
            hash ^= hash_ep[ep];

    }

    /* init_hash() initializes the random numbers used by set_hash(). */
    static void init_hash()
    {

        int i, j, k;

        srand(0);
        for (i = 0; i < 2; ++i)
            for (j = 0; j < 6; ++j)
                for (k = 0; k < 64; ++k)
                    hash_piece[i][j][k] = hash_rand();
        hash_side = hash_rand();
        for (i = 0; i < 64; ++i)
            hash_ep[i] = hash_rand();
    }

    /* hash_rand() XORs some shifted random numbers together to make sure
       we have good coverage of all 32 bits. (rand() returns 16-bit numbers
       on some systems.) */
    static int hash_rand()
    {
        int i;
        int r = 0;

        for (i = 0; i < 32; ++i)
            r ^= rand() << i;
        return r;
    }

    void clear()
    {
        int i;

        for (i = 0; i < 64; ++i) {
            color[i] = init_color[i];
            piece[i] = init_piece[i];
        }
        side = LIGHT;
        castle = 15;
        ep = -1;
        fifty = 0;
        ply = 0;
        hply = 0;
        hist_dat.resize(10);
        set_hash();  
    }

};

#endif
