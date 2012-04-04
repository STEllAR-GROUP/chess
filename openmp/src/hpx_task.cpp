#include "parallel_support.hpp"

score_t search(search_info*);
score_t search_ab(search_info*);
score_t qeval(search_info*);
score_t multistrike(search_info*);
    
typedef hpx::actions::plain_result_action1<
    score_t,           // return type
    search_info*,      // arguments
    search_ab             // function name
> alphabeta_action;

typedef hpx::actions::plain_result_action1<
    score_t,           // return type
    search_info*,      // arguments
    search             // function name
> minimax_action;

typedef hpx::actions::plain_result_action1<
    score_t,           // return type
    search_info*,      // arguments
    multistrike             // function name
> strike_action;

typedef hpx::actions::plain_result_action1<
    score_t,           // return type
    search_info*,      // arguments
    qeval             // function name
> qeval_action;

HPX_REGISTER_PLAIN_ACTION(alphabeta_action);    
HPX_REGISTER_PLAIN_ACTION(minimax_action);    
HPX_REGISTER_PLAIN_ACTION(strike_action);    
HPX_REGISTER_PLAIN_ACTION(qeval_action);    

void hpx_task::start() {
    assert(info.valid());
    info->self = 0;
    hpx::naming::id_type const locality_id = hpx::find_here();

    if (pfunc == search_f)
    {
        result = hpx::lcos::async<minimax_action>(locality_id, info.ptr());
    }
    else if (pfunc == search_ab_f)
    {
        result = hpx::lcos::async<alphabeta_action>(locality_id, info.ptr());
    }
    else if (pfunc == strike_f)
    {
        result = hpx::lcos::async<strike_action>(locality_id, info.ptr());
    }
    else if (pfunc == qeval_f)
    {
        result = hpx::lcos::async<qeval_action>(locality_id, info.ptr());
    }
    else
        assert(false); // should never get here
        
}
