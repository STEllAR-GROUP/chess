#ifndef NODE_H
#define NODE_H
#include <iostream>
#include <vector>
#include <stdlib.h>
#include <stdio.h>
#include "defs.hpp"
#include "zkey.hpp"

struct hist_t {
    move m;
    int capture;
    int castle;
    int ep;
    int fifty;
    hash_t hash;
};

struct node_t { 
    std::vector<hist_t> hist_dat;
    hash_t hash;
    char color[64];
    char piece[64];
    int depth;
    int side;
    int castle;
    int ep;
    
    /* the number of moves since a capture or pawn move, used to handle the fifty-move-draw rule */
    int fifty;
    
    int ply;
    int hply;
};

inline int get_bucket_index(const node_t& board,int depth) {
    //return ((board.hash >> M) ^ depth) & ((1<<N)-1);
    return board.hash & ((1<<N)-1);
}
inline int get_entry_index(const node_t& board,int depth) {
    //return board.hash & ((1<<M)-1);
    return ((board.hash >> N) ^ depth) & ((1<<M)-1);
}

#endif
