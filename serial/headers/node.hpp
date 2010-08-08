#ifndef NODE_H
#define NODE_H
#include <iostream>
#include <vector>
#include <boost/serialization/serialization.hpp>
#include <stdlib.h>
#include <stdio.h>
#include "defs.hpp"

class hist_t {

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & m.u;
        ar & capture;
        ar & ep;
        ar & fifty;
        ar & hash;
    }
    public:
    hist_t() {};


    move m;
    int capture;
    int castle;
    int ep;
    int fifty;
    int hash;
};

class node_t { 
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & hist_dat;
        ar & hash;
        ar & color;
        ar & piece;
        ar & depth;
        ar & side;
        ar & castle;
        ar & ep;
        ar & fifty;
        ar & ply;
        ar & hply;
    }
    public:
    node_t() {};


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
