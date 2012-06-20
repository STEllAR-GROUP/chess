#ifndef SEARCH_COORDINATOR_HPP
#define SEARCH_COORDINATOR_HPP

#include <pthread.h>


/**
 * This struct provides a mutex and condition variable to share across multiple tasks for wait_any functionality.
 */

struct search_coordinator{
    pthread_mutex_t* mut;
    pthread_cond_t* cond;
    bool active;

    search_coordinator(){
        mut=0;
        cond=0;
        active=false;
    }
};

#endif
