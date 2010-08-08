/*
 *  SEARCH.C
 */


#include "search.hpp"

int nodes;


/* think() calls search() iteratively. Search statistics
   are printed depending on the value of out:
   0 = no output
   1 = normal output
   2 = xboard format output */

int think(std::vector<gen_t>& workq, node_t& board, int out)
{
    int i, j, val;

    /* try the opening book first */
    pv[0][0].u = book_move(workq, board);
    if (pv[0][0].u != -1)
        return 0;

    board.ply = 0;
    nodes = 0;

    memset(pv, 0, sizeof(pv));
    if (table)
        memset(hash_table, 0, sizeof(HASHE) * hash_table_size);
    if (out == 1)
        printf("ply      nodes  score  pv\n");
    for (i = 1; i <= depth[board.side]; ++i) {
        follow_pv = TRUE;
        val = search(board, -10000, 10000, i);
        if (out == 1)
            printf("%3d  %9d  %5d ", i, nodes, val);
        if (out) {
            for (j = 0; j < pv_length[0]; ++j)
                printf(" %s", move_str(pv[0][j].b));
            printf("\n");
            fflush(stdout);
        }
        if (val > 9000 || val < -9000)
            break;
    }
    return nodes;
}


/* search() does just that, in negamax fashion */

int search(node_t board, int alpha, int beta, int depth)
{
    int i, j, val;
    BOOL c, f;

    if (table)
        if ((val = ProbeHash(board, depth, alpha, beta)) != valUNKNOWN)
            return val;

    /* we're as deep as we want to be; call quiesce() to get
       a reasonable score and return it. */
    if (!depth)
    {
        val = quiesce(board, alpha,beta);
        if (table)
            RecordHash(board, depth, val, hashfEXACT, 
                       pv[board.ply][board.ply]);
        return val;
    }
    ++nodes;

    pv_length[board.ply] = board.ply;

    /* if this isn't the root of the search tree (where we have
       to pick a move and can't simply return 0) then check to
       see if the position is a repeat. if so, we can assume that
       this line is a draw and return 0. */
    if (board.ply && reps(board))
        return 0;

    /* are we too deep? */
    if (board.ply >= MAX_PLY - 1)
        return eval(board);
    if (board.hply >= HIST_STACK - 1)
        return eval(board);

    /* are we in check? if so, we want to search deeper */
    c = in_check(board, board.side);
    if (c)
        ++depth;
    std::vector<gen_t> workq;
    gen(workq, board);
    if (follow_pv)  /* are we following the PV? */
        sort_pv(workq, board);
    f = FALSE;

    int hashf = hashfALPHA;

    /* loop through the moves */
    while (workq.size() != 0) {
        gen_t g = workq.back();
        workq.pop_back();
        sort(workq);
        if (!makemove(board, g.m.b)) {
            continue;
        }

        f = TRUE;
        val = -search(board, -beta, -alpha, depth - 1);
        takeback(board);

        if (val > alpha) {

            if (val >= beta)
            {
                // We want to record the hash, 
                // storing the move that caused this cutoff
                if (table)
                    RecordHash(board, depth, beta, hashfBETA, g.m);
                return beta;
            }
            hashf = hashfEXACT;
            alpha = val;

            /* update the PV */
            pv[board.ply][board.ply] = g.m;
            for (j = board.ply + 1; j < pv_length[board.ply + 1]; ++j)
                pv[board.ply][j] = pv[board.ply + 1][j];
            pv_length[board.ply] = pv_length[board.ply + 1];
        }
    }

    /* no legal moves? then we're in checkmate or stalemate */
    if (!f) {
        if (c)
            return -10000 + board.ply;
        else
            return 0;
    }

    /* fifty move draw rule */
    if (board.fifty >= 100)
        return 0;
    if (table)
        RecordHash(board, depth, alpha, hashf, pv[board.ply][board.ply]);
    return alpha;
}


/* quiesce() is a recursive minimax search function with
   alpha-beta cutoffs. In other words, negamax. It basically
   only searches capture sequences and allows the evaluation
   function to cut the search off (and set alpha). The idea
   is to find a position where there isn't a lot going on
   so the static evaluation function will work. */

int quiesce(node_t& board, int alpha,int beta)
{
    int i, j, val;

    ++nodes;

    pv_length[board.ply] = board.ply;

    /* are we too deep? */
    if (board.ply >= MAX_PLY - 1)
        return eval(board);
    if (board.hply >= HIST_STACK - 1)
        return eval(board);

    /* check with the evaluation function */
    val = eval(board);
    if (val >= beta)
        return beta;
    if (val > alpha)
        alpha = val;

    std::vector<gen_t> workq;
    gen_caps(workq, board);
    if (follow_pv)  /* are we following the PV? */
        sort_pv(workq, board);

    /* loop through the moves */
    while (workq.size() != 0) {
        gen_t g = workq.back();
        workq.pop_back();
        sort(workq);
        if (!makemove(board, g.m.b)) {
            continue;
        }
        val = -quiesce(board, -beta, -alpha);
        takeback(board);
        if (val > alpha) {
            if (val >= beta)
            {
                return beta;
            }
            alpha = val;

            /* update the PV */
            pv[board.ply][board.ply] = g.m;
            for (j = board.ply + 1; j < pv_length[board.ply + 1]; ++j)
                pv[board.ply][j] = pv[board.ply + 1][j];
            pv_length[board.ply] = pv_length[board.ply + 1];
        }
    }
    return alpha;
}


/* reps() returns the number of times the current position
   has been repeated. It compares the current value of hash
   to previous values. */

int reps(node_t& board)
{
    int i;
    int r = 0;

    for (i = board.hply - board.fifty; i < board.hply; ++i)
        if (board.hist_dat[i].hash == board.hash)
            ++r;
    return r;
}


/* sort_pv() is called when the search function is following
   the PV (Principal Variation). It looks through the current
   ply's move list to see if the PV move is there. If so,
   it adds 10,000,000 to the move's score so it's played first
   by the search function. If not, follow_pv remains FALSE and
   search() stops calling sort_pv(). */

void sort_pv(std::vector<gen_t>& workq, node_t& board)
{

    follow_pv = FALSE;
    for (int i = 0; i < workq.size(); i++) {
        if (workq[i].m.u == pv[0][board.ply].u) {
            follow_pv = TRUE;
            workq[i].score += 10000000;
            workq.push_back(workq[i]);
            workq.erase(workq.begin() + i);
            return;
        }
    }
}

/* sort() searches the current ply's move list from 'from'
   to the end to find the move with the highest score. Then it
   swaps that move and the 'from' move so the move with the
   highest score gets searched next, and hopefully produces
   a cutoff. */

void sort(std::vector<gen_t>& workq)
{
    int bs = -1; // Best Score
    for (int i = 0; i < workq.size(); i++) {
        if (workq[i].score > bs) {
            bs = workq[i].score;
            workq.push_back(workq[i]);
            workq.erase(workq.begin() + i);
        }
    }
}

