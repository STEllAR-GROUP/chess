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
#include <boost/atomic.hpp>

template<typename T>
class smart_ptr_guts {
    boost::atomic<int> ref_count;
    public:
    T *ptr;
    smart_ptr_guts(int rc,T *p) : ref_count(rc), ptr(p) {
    }
    ~smart_ptr_guts() {
        delete ptr;
    }
    void inc() {
        ref_count++;
    }
    bool dec() {
        int r = --ref_count;
        if(r < 0) abort();
        return r==0;
    }
    int ref_count_() {
        return ref_count;
    }
};

// Count references to an object in a thread-safe
// way using pthreads and do automatic clean up.
template<typename T>
class smart_ptr {
    smart_ptr_guts<T> *guts;
    public:
    void inc() {
        if(guts != 0)
            guts->inc();
    }
    void dec() {
        assert(this != 0);
        if(guts != 0 && guts->dec()) {
            delete guts;
            guts = 0;
        }
    }
    smart_ptr(T *ptr) {
        if(ptr == 0) {
            guts = 0;
        } else {
            guts = new smart_ptr_guts<T>(1,ptr);
        }
    }
    smart_ptr(const smart_ptr<T> &sm) : guts(sm.guts) {
    }
    smart_ptr() : guts(0) {}
    ~smart_ptr() {
    }
    void operator=(T *t) {
        if(t == 0) {
            guts = 0;
        } else {
            guts = new smart_ptr_guts<T>(1,t);
        }
    }
    void operator=(const smart_ptr<T>& s) {
        assert(this != 0);
        guts = s.guts;
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
    int ref_count() {
        assert(guts != 0);
        return guts->ref_count_();
    }
};

template<typename T>
struct dtor {
    smart_ptr<T> sptr;
    dtor(smart_ptr<T>& s) : sptr(s) {}
    ~dtor() {
        sptr.dec();
    }
};
#endif
