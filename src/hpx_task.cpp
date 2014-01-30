#include "parallel_support.hpp"
#include <hpx/include/iostreams.hpp>

score_t search(search_info*);
score_t search_ab(search_info*);
score_t qeval(search_info*);

#ifdef HPX_SUPPORT
    
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
    qeval             // function name
> qeval_action;

HPX_REGISTER_PLAIN_ACTION(alphabeta_action);    
HPX_REGISTER_PLAIN_ACTION(minimax_action);    
HPX_REGISTER_PLAIN_ACTION(qeval_action);    

using namespace hpx;

// Figure out why actions perform badly
#define USE_ACTIONS 0
#if USE_ACTIONS
void hpx_task::start() {
    assert(info.valid());
    info->self = NULL;
    static std::vector<hpx::naming::id_type> all_localities = hpx::find_all_localities();
    static hpx::naming::id_type const locality_id = all_localities[0];

    if (pfunc == search_f)
    {
        result = async<minimax_action>(locality_id, info.ptr());
    }
    else if (pfunc == search_ab_f)
    {
        result = async<alphabeta_action>(locality_id, info.ptr());
    }
    else if (pfunc == qeval_f)
    {
        result = async<qeval_action>(locality_id, info.ptr());
    }
    else
        assert(false); // should never get here
        
}
#else
void hpx_task::start() {
    if (pfunc == search_f)
    {
        result = async(search_pt, info.ptr());
    }
    else if (pfunc == search_ab_f)
    {
        result = async(search_ab_pt, info.ptr());
    }
    else if (pfunc == qeval_f)
    {
        result = async(qeval_pt, info.ptr());
    }
    else
        assert(false); // should never get here
        
}
#endif
#endif
