////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2011 Steve Brandt and Philip LeBlanc
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file BOOST_LICENSE_1_0.rst or copy at http://www.boost.org/LICENSE_1_0.txt)
////////////////////////////////////////////////////////////////////////////////
#ifndef FIXED_VEC_HPP
#define FIXED_VEC_HPP 1
#include <assert.h>
template<class T,int N>
struct FixedVec {
    T data[N];
    int _size;
    FixedVec() : _size(0) {}
    FixedVec(const FixedVec<T,N>& fvec) {
        _size = fvec._size;
        for(int i=0;i < _size;i++)
            data[i] = fvec.data[i];
    }
    T& operator[](int n) {
        assert(n >= 0 && n < _size);
        return data[n];
    }
    const T& operator[](int n) const {
        assert(n >= 0 && n < _size);
        return data[n];
    }
    void operator=(const FixedVec<T,N>& fvec) {
        _size = fvec._size;
        for(int i=0;i < _size;i++)
            data[i] = fvec.data[i];
    }
    T *ptr() { return data; }
    void push_back(T t) {
        if(_size == N) {
            // We're only using this for
            // chess_move history, and only the
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
    void resize(int n) {
        assert(n >= 0 && n < N);
        // Keep Valgrind happy
        while(_size < n) {
            push_back(0);
        }
        if(_size > n) {
            int d = _size-n;
            for(int i=0;i<n;i++) {
                data[i] = data[i+d];
            }
        }
        _size = n;
    }
};
#endif
