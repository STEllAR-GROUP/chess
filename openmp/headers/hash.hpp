#ifndef HASH_HPP
#define HASH_HPP
#include <stdint.h>
/*
struct hash_t {
    uint64_t val;
    hash_t() : val(0) {}
    hash_t(uint64_t v,int) : val(v) {}
};
inline bool operator<(const hash_t& h1,const hash_t& h2) {
    return h1.val < h2.val;
}
inline bool operator==(const hash_t& h1,const hash_t& h2) {
    return h1.val == h2.val;
}
inline hash_t& operator^=(hash_t& h1,hash_t& h2) {
    h1.val ^= h2.val;
    return h1;
}
inline hash_t& operator<<(hash_t& h,int n) {
    h.val << n;
    return h;
}
*/
typedef uint64_t hash_t;
#endif
