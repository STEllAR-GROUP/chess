#ifndef TASK_COMPONENT_H
#define TASK_COMPONENT_H 1
#include <hpx/hpx_fwd.hpp>
#include <hpx/include/components.hpp>
#include <hpx/include/actions.hpp>
#include <hpx/include/async.hpp>
#include "parallel_support.hpp"


namespace task_component { 
class HPX_COMPONENT_EXPORT server : public hpx::components::managed_component_base<server>  {

public:
   server()  {}
   enum {
	  cancel_func_code,
	  search_func_code,

   } func_codes;

   void cancel();
   typedef hpx::actions::action0<
	  server, cancel_func_code , &server::cancel
   > cancel_action;

   score_t search(search_info inf);
   typedef hpx::actions::result_action1<
	  server, score_t, search_func_code , search_info, &server::search
   > search_action;

};
}
HPX_REGISTER_ACTION_DECLARATION_EX(task_component::server::cancel_action,action_cancel);
HPX_REGISTER_ACTION_DECLARATION_EX(task_component::server::search_action,action_search);

namespace task_component { 

class stub : public hpx::components::stubs::stub_base<server> {
public:

   static void cancel(
   	hpx::naming::id_type gid)
   {
	  hpx::async<server::cancel_action>(gid).get();
   }

   static hpx::lcos::future<score_t> search_async(
	  hpx::naming::id_type gid, search_info arg0)
   {
	  return hpx::async<server::search_action>(gid, arg0);
   }
   static score_t search(
	  hpx::naming::id_type gid, search_info arg0)
   {
	  return search_async(gid, arg0).get();
   }

};

class client : public hpx::components::client_base<client, stub> {
   typedef hpx::components::client_base<client, task_component::stub> base_type;
public:
   client(hpx::naming::id_type gid) : base_type(gid) {}
   client() : base_type() {}

   void cancel() {
	  BOOST_ASSERT(gid_);
	  this->base_type::cancel(gid_);
   }

   score_t search(search_info arg0) {
	  BOOST_ASSERT(gid_);
	  return this->base_type::search(gid_, arg0);
   }

};
}
#endif
