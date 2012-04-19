////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2011 Steve Brandt and Philip LeBlanc
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file BOOST_LICENSE_1_0.rst or copy at http://www.boost.org/LICENSE_1_0.txt)
////////////////////////////////////////////////////////////////////////////////
#ifndef SMART_PTR_HPP
#define SMART_PTR_HPP
#include <assert.h>
#include "parallel.hpp"

template<typename T>
class smart_ptr_guts {
    Mutex mut;
    int ref_count;
    public:
    T *ptr;
    smart_ptr_guts(int rc,T *p) : ref_count(rc), ptr(p) {
    }
    ~smart_ptr_guts() {
        delete ptr;
    }
    void inc() {
        ScopedLock s(mut);
        ref_count++;
    }
    bool dec() {
        ScopedLock s(mut);
        int r = ref_count;
        if(ref_count>0)
            ref_count--;
        return r==1;
    }
};

// Count references to an object in a thread-safe
// way using pthreads and do automatic clean up.
template<typename T>
class smart_ptr {
    smart_ptr_guts<T> *guts;
    void clean() {
        assert(this != 0);
        if(guts != 0 && guts->dec()) {
            delete guts;
            guts = 0;
        }
    }
    public:
    smart_ptr(T *ptr) {
        if(ptr == 0) {
            guts = 0;
        } else {
            guts = new smart_ptr_guts<T>(1,ptr);
        }
    }
    smart_ptr(const smart_ptr<T> &sm) : guts(sm.guts) {
        if(sm.guts != 0)
            guts->inc();
    }
    smart_ptr() : guts(0) {}
    ~smart_ptr() {
        clean();
    }
    void operator=(T *t) {
        clean();
        if(t == 0) {
            guts = 0;
        } else {
            guts = new smart_ptr_guts<T>(1,t);
        }
    }
    void operator=(const smart_ptr<T>& s) {
        assert(this != 0);
        clean();
        guts = s.guts;
        if(guts != 0)
            guts->inc();
    }
    T& operator*() {
        assert(guts != 0);
        return *guts->ptr;
    }
    T *operator->() const {
        if(guts == 0)
            return 0;
        else
            return guts->ptr;
    }
    T *ptr() {
        assert(guts != 0);
        return guts->ptr;
    }
    bool valid() {
        return guts != 0 && guts->ptr != 0;
    }
};
#endif
