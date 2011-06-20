#ifndef FIXED_VEC_HPP
#define FIXED_VEC_HPP 1
#include <assert.h>
template<class T,int N>
struct FixedVec {
    T data[N];
    int _size;
    FixedVec() : _size(0) {}
    FixedVec(int _sz) : _size(_size) {
        assert(_size <= N);
    }
    T& operator[](int n) {
        assert(n >= 0 && n < _size);
        return data[n];
    }
    const T& get(int n) const {
        assert(n >= 0 && n < _size);
        return data[n];
    }
    T *ptr() { return data; }
    void push_back(T t) {
        assert(_size < N);
        data[_size++] = t;
    }
    int size() const {
        return _size;
    }
    int resize(int n) {
        _size = n;
        assert(n >= 0 && n < N);
    }
};
#endif
