#ifndef PARALLEL_HPP
#define PARALLEL_HPP

/*
 * By wrapping the HPX mutex and the pthread mutex in the Mutex and
 * ScopedLock classes we make it possible to support both without a
 * lot of ifdefs.
 */

#include <assert.h>

#ifdef HPX_ENABLED
#include <hpx/include/lcos.hpp>
#define pthread_mutex_lock(X) std::cout<<"No pthread mutex locks allowed"<<std::endl; abort();
#define pthread_cond_wait(X,Y) std::cout<<"No pthread cond waits allowed"<<std::endl; abort();
struct Mutex {
    hpx::lcos::local::mutex mut;
    Mutex() : mut() {}
    ~Mutex() {}
};
struct ScopedLock {
    hpx::lcos::local::mutex::scoped_lock l;
    ScopedLock(Mutex& m) : l(m.mut) {}
    ~ScopedLock() {}
};
/*
template<typename T>
struct Future {
    hpx::lcos::future<T> fut;
    T& get() { return fut.get(); }
};
*/
#else
#include <pthread.h>
struct Mutex {
    pthread_mutex_t mut;
    Mutex() {
        pthread_mutex_init(&mut,NULL);
    }
};
struct ScopedLock {
    Mutex *m;
    ScopedLock(Mutex& m_) {
        m = &m_;
        pthread_mutex_lock(m);
    }
    ~ScopedLock() {
        pthread_mutex_unlock(m);
    }
};
#endif

#endif
