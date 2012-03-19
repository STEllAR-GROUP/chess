////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2011 Steve Brandt and Philip LeBlanc
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file BOOST_LICENSE_1_0.rst or copy at http://www.boost.org/LICENSE_1_0.txt)
////////////////////////////////////////////////////////////////////////////////
#ifndef NODE_H
#define NODE_H
#include <iostream>
#include <vector>
#include <stdlib.h>
#include <stdio.h>
#include "defs.hpp"
#include "hash.hpp"
#include "FixedVec.hpp"

struct base_node_t { 
    hash_t hash;
    char color[64];
    char piece[64];
    int depth;
    int side;
    int castle;
    int ep;
    
    /* the number of moves since a capture or pawn chess_move, used to handle the fifty-chess_move-draw rule */
    int fifty;
    
    int ply;
    int hply;
    //std::vector<hash_t> hist_dat;
};
struct node_t : public base_node_t {
    FixedVec<hash_t,50> hist_dat;
};

#endif
