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

using namespace hpx;

void hpx_task::start() {
    assert(info.valid());
    info->self = 0;
    hpx::naming::id_type const locality_id = get_next_locality();

    if (pfunc == search_f)
    {
        result = async<minimax_action>(locality_id, info.ptr());
    }
    else if (pfunc == search_ab_f)
    {
        result = async<alphabeta_action>(locality_id, info.ptr());
    }
    else if (pfunc == strike_f)
    {
        result = async<strike_action>(locality_id, info.ptr());
    }
    else if (pfunc == qeval_f)
    {
        result = async<qeval_action>(locality_id, info.ptr());
    }
    else
        assert(false); // should never get here
        
}

hpx::naming::id_type hpx_task::get_next_locality() {
    hpx::naming::id_type return_id = all_localities[index];
    index = index + 1;
    if (index > all_localities.size())
        index=0;
    return return_id;
}
