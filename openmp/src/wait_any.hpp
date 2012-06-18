#ifndef WAIT_ANY_HPP
#define WAIT_ANY_HPP

#include <vector>
#include <iostream> //for testing purposes
#include "smart_ptr.hpp"
#include <pthread.h>
#include "parallel_support.hpp"
#include <sys/time.h>
#include <errno.h>

#include <time.h>
#include <unistd.h>

smart_ptr<task> wait_any_busy(std::vector< smart_ptr<task> >& task_list){
   while(true){
      for(int i=0;i<task_list.size();i++){
         if(task_list[i]->info->par_done){
            smart_ptr<task> to_ret=task_list[i];
            task_list.erase(task_list.begin()+i);
            return to_ret;
         }
      }
      //sleep(1);
      struct timespec req={0};
      req.tv_sec=0;
      req.tv_nsec=1000000L;
      nanosleep(&req,(struct timespec*)0);
   }
}


int get_done_task(std::vector< smart_ptr<task> > const& task_list){
   for(int i=0;i<task_list.size();i++){
      if(task_list[i]->info->par_done){
         return i;
      }
   }
   return -1;
}

smart_ptr<task> wait_any_timeout(std::vector< smart_ptr<task> >& task_list){
   int done_index=get_done_task(task_list);
   if(done_index<0){ //no task already done
      //create shared mutex and condition variable for tasks to signal completion with
      pthread_mutex_t shared_mut;
      pthread_cond_t shared_cond;
      pthread_mutex_init(&shared_mut,0);
      pthread_cond_init(&shared_cond,0);
      //set task's to share cond/mut 
      for(int i=0;i<task_list.size();i++){
         task_list[i]->info->mut=shared_mut;
         task_list[i]->info->cond=shared_cond;
      }
      //timed cond wait because task may have already completed while mutex/cond was being initialized, so don't want to wait forever
      pthread_mutex_lock(&shared_mut);
      struct timeval now;
      struct timespec timeout;
      gettimeofday(&now,0);
      timeout.tv_sec=now.tv_sec+1;
      timeout.tv_nsec=now.tv_usec*1000;
      int wait_return_code=pthread_cond_timedwait(&shared_cond,&shared_mut,&timeout);
      if(wait_return_code==ETIMEDOUT){
         return wait_any_timeout(task_list);
      }
      done_index=get_done_task(task_list);
   }
   smart_ptr<task> to_ret=task_list[done_index];
   task_list.erase(task_list.begin()+done_index);
   return to_ret;
}

smart_ptr<task> wait_any(std::vector< smart_ptr<task> >& task_list){
   pthread_mutex_t shared_mut;
   pthread_cond_t shared_cond;
   pthread_mutex_init(&shared_mut,0);
   pthread_cond_init(&shared_cond,0);
   for(int i=0;i<task_list.size();i++){
      task_list[i]->info->mut=shared_mut;
      task_list[i]->info->cond=shared_cond;
   }
   pthread_mutex_lock(&shared_mut);
   int done_index=get_done_task(task_list);
   if(done_index<0){
      pthread_cond_wait(&shared_cond,&shared_mut);
      done_index=get_done_task(task_list);
   }
   smart_ptr<task> to_ret=task_list[done_index];
   task_list.erase(task_list.begin()+done_index);
   pthread_mutex_unlock(&shared_mut);
   return to_ret;
}


#endif
