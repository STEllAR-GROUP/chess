#ifndef NODE_H
#define NODE_H
#include <iostream>
#include <vector>
#include <stdlib.h>
#include <stdio.h>
#include "defs.hpp"
#include "hash.hpp"

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

#endif
