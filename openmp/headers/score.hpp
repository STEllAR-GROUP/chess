#ifndef SCORE_HPP
#define SCORE_HPP
#include "hash.hpp"
#include <ostream>

#define LONG_SCORE 1
#define SHORT_SCORE 2
#define CLASS_SCORE 3

#define SCORE_TYPE LONG_SCORE

#if SCORE_TYPE == CLASS_SCORE
/**
 * This doesn't work exactly the same as the
 * official version, LONG_SCORE, but it
 * is close.
 **/
const uint32_t BITS = 16L;
const uint32_t MASK = (1L<<(BITS+1))-1L;
struct score_t {
    signed long base;
    signed long hash;
    score_t(int b,hash_t h) : base(b), hash(h) {
        hash &= MASK;
    }
    score_t(const score_t& sc) : base(sc.base), hash(sc.hash) {
        hash &= MASK;
    }
    inline void operator=(const score_t& s) {
        base = s.base;
        hash = s.hash;
    }
    score_t() : base(0), hash(0) {}
};
inline std::ostream& operator<<(std::ostream& o,const score_t& s) {
    return o << s.base;
}
inline bool operator<(const score_t& a,const score_t& b) {
    if(a.base == b.base) return a.hash < b.hash;
    return a.base < b.base;
}
inline bool operator>(const score_t& a,const score_t& b) {
    if(a.base == b.base) return a.hash > b.hash;
    return a.base > b.base;
}
inline bool operator>=(const score_t& a,const score_t& b) {
    if(a.base == b.base) return a.hash >= b.hash;
    return a.base >= b.base;
}
inline bool operator<=(const score_t& a,const score_t& b) {
    if(a.base == b.base) return a.hash <= b.hash;
    return a.base <= b.base;
}
inline bool operator==(const score_t& a,const score_t& b) {
    return a.base == b.base && a.hash == b.hash;
}
inline bool operator!=(const score_t& a,const score_t& b) {
    return a.base != b.base || a.hash != b.hash;
}
inline score_t operator-(const score_t& a,int b) {
    score_t s(a.base-b,a.hash);
    return s;
}
inline score_t operator-(const score_t& a) {
    score_t s(a);
    s.base *= -1;
    s.hash *= -1;
    return s;
}
inline score_t operator+(const score_t& a,int b) {
    score_t s(a.base+b,a.hash);
    return s;
}
inline const score_t& max(const score_t& a,const score_t& b) {
    if(a >= b)
        return a;
    else
        return b;
}
#define DECL_SCORE(name,val,hcode) score_t name(val,hcode);
#define ADD_SCORE(var,val) var+val
#endif

#if SCORE_TYPE == LONG_SCORE
typedef signed long score_t;
const uint32_t BITS = 16L;
const uint32_t MASK = (1L<<(BITS+1))-1L;
#define DECL_SCORE(name,val,hcode) score_t name = (score_t(val)<<BITS) | (hcode & MASK);
#define ADD_SCORE(var,val) var + (score_t(val)<<BITS)
#endif

#if SCORE_TYPE == SHORT_SCORE
typedef signed int score_t;
#define DECL_SCORE(name,val,hcode) score_t name = val;
#define ADD_SCORE(var,val) var + val
#endif

#endif
