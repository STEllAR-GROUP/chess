#include "parallel_support.hpp"
#include <hpx/include/iostreams.hpp>
#include "search.hpp"

#ifdef HPX_SUPPORT
    
HPX_PLAIN_ACTION(search_ab_pt,alphabeta_action);
HPX_PLAIN_ACTION(search_pt,minimax_action);
HPX_PLAIN_ACTION(qeval_pt,qeval_action);

using namespace hpx;

// Figure out why actions perform badly
#define USE_ACTIONS 1
#if USE_ACTIONS
void hpx_task::start() {
    assert(info.valid());
    static std::vector<hpx::naming::id_type> all_localities = hpx::find_all_localities();
    static hpx::naming::id_type const locality_id = all_localities[0];

    if (pfunc == search_f)
    {
        result = async<minimax_action>(locality_id, info);
    }
    else if (pfunc == search_ab_f)
    {
        result = async<alphabeta_action>(locality_id, info);
    }
    else if (pfunc == qeval_f)
    {
        result = async<qeval_action>(locality_id, info);
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
