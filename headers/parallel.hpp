#ifndef PARALLEL_HPP
#define PARALLEL_HPP

/*
 * By wrapping the HPX mutex and the thread mutex in the Mutex and
 * ScopedLock classes we make it possible to support both without a
 * lot of ifdefs.
 */

#include <assert.h>

#ifdef HPX_SUPPORT
#include <hpx/include/lcos.hpp>
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
#else
#include <mutex>
struct Mutex {
    std::mutex mut;
    Mutex() {}
};
struct ScopedLock {
    Mutex *m;
    ScopedLock(Mutex& m_) {
        m = &m_;
        m->mut.lock();
    }
    ~ScopedLock() {
        m->mut.unlock();
    }
};
#endif

#endif
