#include "parallel_support.hpp"

score_t search_ab(search_info*);
    
typedef hpx::actions::plain_result_action1<
    score_t,           // return type
    search_info*,      // arguments
    search_ab             // function name
> alphabeta_action;

HPX_REGISTER_PLAIN_ACTION(alphabeta_action);    

void hpx_task::start() {
    assert(info.valid());
    info->self = 0;
    hpx::naming::id_type const locality_id = hpx::find_here();
    result = hpx::lcos::async<alphabeta_action>(locality_id, info.ptr());
}
