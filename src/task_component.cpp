#include "task_component.hpp"
#include <hpx/hpx.hpp>
#include <hpx/include/components.hpp>
#include <hpx/include/serialization.hpp>

#include "task_component_impl.hpp"
HPX_REGISTER_COMPONENT_MODULE();

typedef hpx::components::managed_component<task_component::server> task_component_type;




typedef task_component::server task_component_server;

HPX_REGISTER_MINIMAL_COMPONENT_FACTORY(task_component_type, task_component_server);
HPX_REGISTER_ACTION_EX(task_component_type::wrapped_type::cancel_action,action_cancel);
HPX_REGISTER_ACTION_EX(task_component_type::wrapped_type::search_action,action_search);


