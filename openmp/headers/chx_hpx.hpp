#ifndef CHX_HPX_HPP_367926FF_8920_423F_8FCA_E8_CHESS34E8_CHESS5C6_CHESS101
#define CHX_HPX_HPP_367926FF_8920_423F_8FCA_E8_CHESS34E8_CHESS5C6_CHESS101

#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/runtime/actions/plain_action.hpp>
#include "node.hpp"

//typedef hpx::actions::plain_result_action1<
//    score_t,           // return type
//    search_info*,      // arguments
//    search             // function name
//> minimax_action;
    
//typedef hpx::actions::plain_result_action1<
//    score_t,           // return type
//    search_info*,      // arguments
//    qeval             // function name
//> qeval_action;
    
    
//HPX_REGISTER_PLAIN_ACTION(minimax_action);    
//HPX_REGISTER_PLAIN_ACTION(qeval_action);    
    
namespace boost { namespace serialization
{
template <typename Archive>
void serialize(Archive &ar, search_info& info, const unsigned int)
{
    
}
template <typename Archive>
void serialize(Archive &ar, smart_ptr<search_info>& info, const unsigned int)
{}
template <typename Archive>
void serialize(Archive &ar, smart_ptr<task>& info, const unsigned int)
{}
template <typename Archive>
void serialize(Archive &ar, node_t& board, const unsigned int)
{}
template <typename Archive>
void serialize(Archive &ar, chess_move& m, const unsigned int)
{}
template <typename Archive>
void serialize(Archive &ar, pthread_mutex_t mut, const unsigned int)
{}
template <typename Archive>
void serialize(Archive &ar, pthread_cond_t c, const unsigned int)
{}
}}

#endif
