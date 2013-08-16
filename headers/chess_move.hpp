////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2011 Steve Brandt and Phillip LeBlanc
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file BOOST_LICENSE_1_0.rst or copy at http://www.boost.org/LICENSE_1_0.txt)
////////////////////////////////////////////////////////////////////////////////

#ifndef  CHESS_MOVE_HPP_C1BEDE43_B47B_4694_ACF4_F41143D72B97
#define CHESS_MOVE_HPP_C1BEDE43_B47B_4694_ACF4_F41143D72B97

#include <stdint.h>
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

*/
const int INVALID_MOVE = 0xFFFFFFFF;

class chess_move {
    uint32_t u;
    uint8_t from; // MSB
    uint8_t to;
    uint8_t promote;
    uint8_t bits; // LSB
public:
    uint8_t score;

    chess_move() : u(0), from(0), to(0), promote(0), bits(0), score(0) {}
    uint32_t get32BitMove() {
        return u;
    }
    void set32BitMove(int32_t mv)
    {
        u = mv;
        from = ((mv >> 24) & 0xFF);
        to = ((mv >> 16) & 0xFF);
        promote = ((mv >> 8) & 0xFF);
        bits = ((mv >> 0) & 0xFF);
    }
    void setBytes(int8_t from, int8_t to, int8_t promote, int8_t bits)
    {
        this->from = from;
        this->to = to;
        this->promote = promote;
        this->bits = bits;
        u = ((from & 0xFF) << 24) + ((to & 0xFF) << 16) + ((promote & 0xFF) << 8)
            + ((bits & 0xFF) << 0);
    }
    uint8_t getFrom() { return from; }
    uint8_t getTo() { return to; }
    uint8_t getPromote() { return promote; }
    uint8_t getBits() { return bits; }
    uint8_t getCapture() const {
        return (bits & 1) != 0;
    }
    void operator=(const uint32_t mv)
    {
        set32BitMove(mv);
    }
    void operator=(const chess_move& mv)
    {
        u = mv.u;
        from = mv.from;
        to = mv.to;
        promote = mv.promote;
        bits = mv.bits;
        score = mv.score;
    }
    bool operator==(const chess_move& mv)
    {
        return (mv.u == u);
    }
    bool operator==(const uint32_t mv)
    {
        return (mv == u);
    }
    bool operator!=(const uint32_t mv)
    {
        return (mv != u);
    }
    template <class Archive>
    void serialize(Archive &ar, const unsigned int)
    {
        ar & this->u;
        ar & this->from;
        ar & this->to;
        ar & this->promote;
        ar & this->bits;
    }
};

#endif
