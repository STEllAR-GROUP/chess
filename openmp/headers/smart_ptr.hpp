#ifndef SMART_PTR_HPP
#define SMART_PTR_HPP
#include <assert.h>
#include <pthread.h>

template<typename T>
class smart_ptr_guts {
	pthread_mutex_t mut;
    int ref_count;
public:
    T *ptr;
    smart_ptr_guts(int rc,T *p) : ref_count(rc), ptr(p) {
		pthread_mutex_init(&mut,NULL);
	}
    ~smart_ptr_guts() {
        delete ptr;
    }
	void inc() {
		pthread_mutex_lock(&mut);
		ref_count++;
		pthread_mutex_unlock(&mut);
	}
	bool dec() {
		pthread_mutex_lock(&mut);
		int r = ref_count;
		if(ref_count>0)
			ref_count--;
		pthread_mutex_unlock(&mut);
		return r==1;
	}
};

// Count references to an object in a thread-safe
// way using pthreads and do automatic clean up.
template<typename T>
class smart_ptr {
    smart_ptr_guts<T> *guts;
    void clean() {
        if(guts != 0 && guts->dec()) {
            delete guts;
        }
    }
public:
    smart_ptr(T *ptr) : guts(new smart_ptr_guts<T>(1,ptr)) {
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
        guts = new smart_ptr_guts<T>(1,t);
    }
    void operator=(const smart_ptr<T>& s) {
        clean();
        guts = s.guts;
        if(guts != 0)
            guts->inc();
    }
    T& operator*() {
        assert(guts != 0);
        assert(guts->ptr != 0);
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
        assert(guts->ptr != 0);
        return guts->ptr;
	}
	bool valid() {
		return guts != 0 && guts->ptr != 0;
	}
};
#endif
