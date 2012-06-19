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


int get_done_index(std::vector< smart_ptr<task> > const& task_list){
   for(int i=0;i<task_list.size();i++){
      if(task_list[i]->info->par_done){
         return i;
      }
   }
   return -1;
}

smart_ptr<task> wait_any(std::vector< smart_ptr<task> >& task_list){
   pthread_mutex_t* shared_mut=task_list[0]->info->mut;
   pthread_cond_t* shared_cond=task_list[0]->info->cond;
   int lock_return=pthread_mutex_lock(shared_mut);
   int done_index=get_done_index(task_list);
   if(done_index<0){
      //std::cout<<"cond-wait'ing"<<std::endl;
      pthread_cond_wait(shared_cond,shared_mut);
      //std::cout<<"Done waiting"<<std::endl;
      done_index=get_done_index(task_list);
   }
   smart_ptr<task> to_ret=task_list[done_index];
   task_list.erase(task_list.begin()+done_index);
   pthread_mutex_unlock(shared_mut);
   return to_ret;
}



#endif
