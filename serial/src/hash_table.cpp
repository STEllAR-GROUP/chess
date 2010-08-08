/*
 * HASH_TABLE.C
 */


#include "hash_table.hpp"

int ProbeHash(node_t& board, int depth, int alpha, int beta)
{
    // This next line gets the entry from the hash_table by 
    // calling set_hash() which takes the current board position
    // and does a zobrist hash, which then stores the result in
    // the global variable "hash"
    board.hash = abs(board.hash);
    HASHE * phashe = &hash_table[board.hash % hash_table_size];

    if (phashe->key == board.hash) {
        if (phashe->depth >= depth) {
            if (phashe->flags == hashfEXACT)
            {
                return phashe->val;
            }
            if ((phashe->flags == hashfALPHA) &&
                    (phashe->val <= alpha))
            {
                return alpha;
            }
            if ((phashe->flags == hashfBETA) &&
                    (phashe->val >= beta))
            {
                return beta;
            }
        }
        //gen_t g;
        //g.m = phashe->best;
        //g.score = 0;
    }
    return valUNKNOWN;
}


void RecordHash(node_t& board, int depth, int val, int hashf, move m)
{
    board.hash = abs(board.hash);
    HASHE * phashe = &hash_table[board.hash % hash_table_size];

    phashe->key = board.hash;
    phashe->best = m;
    phashe->val = val;
    phashe->flags = hashf;
    phashe->depth = depth;
}
