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
 * For some reason, the score_t class does not work properly
 **/
struct score_t {
    int base;
    hash_t hash;
    score_t(int b,hash_t h) : base(b), hash(h) {}
    score_t(const score_t& sc) : base(sc.base), hash(sc.hash) {}
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
    score_t s(a);
    s.base -= b;
    return s;
}
inline score_t operator-(const score_t& a) {
    score_t s(a);
    s.base *= -1;
    return s;
}
inline score_t operator+(const score_t& a,int b) {
    score_t s(a);
    s.base += b;
    return s;
}
inline const score_t& max(const score_t& a,const score_t& b) {
    if(a <= b)
        return a;
    else
        return b;
}
#define DECL_SCORE(name,val,hcode) score_t name(val,hcode);
#define ADD_SCORE(var,val) var+val
#endif

#if SCORE_TYPE == LONG_SCORE
#define BITS 16L
#define MASK ((1L<<(BITS+1))-1L)
typedef signed long score_t;
#define DECL_SCORE(name,val,hcode) score_t name = (((signed long)(val))<<BITS) | (hcode & MASK) ;
#define ADD_SCORE(var,val) var + (((signed long)(val)<<BITS))
#endif

#if SCORE_TYPE == SHORT_SCORE
typedef signed int score_t;
#define DECL_SCORE(name,val,hcode) score_t name = val;
#define ADD_SCORE(var,val) var + val
#endif

#endif
