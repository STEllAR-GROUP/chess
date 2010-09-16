#ifndef NODE_H
#define NODE_H
#include <iostream>
#include <vector>
#include <stdlib.h>
#include <stdio.h>
#include "defs.hpp"

struct hist_t {
    move m;
    int capture;
    int castle;
    int ep;
    int fifty;
    int hash;
};

struct node_t { 
    std::vector<hist_t> hist_dat;
    int hash;
    int color[64];
    int piece[64];
    int depth;
    int side;
    int castle;
    int ep;
    int fifty;
    int ply;
    int hply;
};

#endif
