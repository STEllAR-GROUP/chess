#ifndef CHX_HPX_HPP_367926FF_8920_423F_8FCA_E8_CHESS34E8_CHESS5C6_CHESS101
#define CHX_HPX_HPP_367926FF_8920_423F_8FCA_E8_CHESS34E8_CHESS5C6_CHESS101

#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/runtime/actions/plain_action.hpp>
#include "node.hpp"
#include "parallel_support.hpp"

score_t search(search_info*);
score_t search_ab(search_info*);
score_t mtdf(search_info*);
score_t qeval(search_info*);
score_t multistrike(search_info*);

typedef hpx::actions::plain_result_action1<
    score_t,           // return type
    search_info*,      // arguments
    search             // function name
> minimax_action;
    
typedef hpx::actions::plain_result_action1<
    score_t,           // return type
    search_info*,      // arguments
    search_ab             // function name
> alphabeta_action;
    
typedef hpx::actions::plain_result_action1<
    score_t,           // return type
    search_info*,      // arguments
    search             // function name
> mtdf_action;
    
typedef hpx::actions::plain_result_action1<
    score_t,           // return type
    search_info*,      // arguments
    search             // function name
> qeval_action;
    
typedef hpx::actions::plain_result_action1<
    score_t,           // return type
    search_info*,      // arguments
    search             // function name
> multistrike_action;
    
    
    

#endif
