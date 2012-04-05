#ifndef CHX_HPX_HPP_367926FF_8920_423F_8FCA_E8_CHESS34E8_CHESS5C6_CHESS101
#define CHX_HPX_HPP_367926FF_8920_423F_8FCA_E8_CHESS34E8_CHESS5C6_CHESS101

#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/runtime/actions/plain_action.hpp>
#include "node.hpp"
#include "defs.hpp"
    
namespace boost { namespace serialization
{
template <typename Archive>
void serialize(Archive &ar, search_info& info, const unsigned int)
{
    ar & info.board;
    ar & info.result;
    ar & info.depth;
    ar & info.incr;
    ar & info.alpha;
    ar & info.beta;
}
template <typename Archive>
void serialize(Archive &ar, node_t& board, const unsigned int)
{
    ar & board.hash;
    ar & board.color;
    ar & board.piece;
    ar & board.depth;
    ar & board.side;
    ar & board.castle;
    ar & board.ep;
    ar & board.fifty;
    ar & board.ply;
    ar & board.hply;
}
}}

#endif
