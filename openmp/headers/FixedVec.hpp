#ifndef FIXED_VEC_HPP
#define FIXED_VEC_HPP 1
#include <assert.h>
template<class T,int N>
struct FixedVec {
    T data[N];
    int _size;
    FixedVec() : _size(0) {}
    T& operator[](int n) {
        assert(n >= 0 && n < _size);
        return data[n];
    }
    const T& operator[](int n) const {
        assert(n >= 0 && n < _size);
        return data[n];
    }
    T *ptr() { return data; }
    void push_back(T t) {
        if(_size == N) {
            // We're only using this for
            // move history, and only the
            // last N moves matter.
            for(int i=1;i<N;i++)
                data[i-1] = data[i];
            data[_size-1] = t;
        } else {
            data[_size++] = t;
        }
    }
    int size() const {
        return _size;
    }
    int resize(int n) {
        // Keep Valgrind happy
        while(_size < n) {
            push_back(0);
        }
        _size = n;
        assert(n >= 0 && n < N);
    }
};
#endif
