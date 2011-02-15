/*
 *  board.cpp
 */


#include "board.hpp"

// init_board() sets the board to the initial game state.

void init_board(node_t& board)
{
    int i;

    for (i = 0; i < 64; ++i) {
        board.color[i] = init_color[i];
        board.piece[i] = init_piece[i];
    }
    board.side = LIGHT;
    board.castle = 15;
    board.ep = -1;
    board.fifty = 0;
    board.ply = 0;
    board.hply = 0;
    board.hist_dat.resize(10);
    // init_hash() must be called before this function
    board.hash = set_hash(board);  
}


// init_hash() initializes the random numbers used by set_hash().

void init_hash()
{
    int i, j, k;

    srand(0);
    for (i = 0; i < 2; ++i)
        for (j = 0; j < 6; ++j)
            for (k = 0; k < 64; ++k)
                hash_piece[i][j][k] = hash_rand();
    hash_side = hash_rand();
    for (i = 0; i < 64; ++i)
        hash_ep[i] = hash_rand();
}


/* hash_rand() XORs some shifted random numbers together to make sure
   we have good coverage of all 32 bits. (rand() returns 16-bit numbers
   on some systems.) */

int hash_rand()
{
    int i;
    int r = 0;

    for (i = 0; i < 32; ++i)
        r ^= rand() << i;
    return r;
}


/* set_hash() uses the Zobrist method of generating a unique number (hash)
   for the current chess position. Of course, there are many more chess
   positions than there are 32 bit numbers, so the numbers generated are
   not really unique, but they're unique enough for our purposes (to detect
   repetitions of the position). 
   The way it works is to XOR random numbers that correspond to features of
   the position, e.g., if there's a black knight on B8, hash is XORed with
   hash_piece[BLACK][KNIGHT][B8]. All of the pieces are XORed together,
   hash_side is XORed if it's black's move, and the en passant square is
   XORed if there is one. (A chess technicality is that one position can't
   be a repetition of another if the en passant state is different.) */

int set_hash(node_t& board)
{
    int i;

    int hash = 0;   
    for (i = 0; i < 64; ++i)
        if (board.color[i] != EMPTY)
            hash ^= hash_piece[board.color[i]][board.piece[i]][i];
    if (board.side == DARK)
        hash ^= hash_side;
    if (board.ep != -1)
        hash ^= hash_ep[board.ep];

    return hash;
}


/* in_check() returns TRUE if side s is in check and FALSE
   otherwise. It just scans the board to find side s's king
   and calls attack() to see if it's being attacked. */

bool in_check(const node_t& board, int s)
{
    int i;

    for (i = 0; i < 64; ++i)
        if (board.piece[i] == KING && board.color[i] == s)
            return attack(board, i, s ^ 1);
    return true;  /* shouldn't get here */
}


/* attack() returns TRUE if square sq is being attacked by side
   s and FALSE otherwise. */

bool attack(const node_t& board, int sq, int s)
{
    int i, j, n;

    for (i = 0; i < 64; ++i)
        if (board.color[i] == s) {
            if (board.piece[i] == PAWN) {
                if (s == LIGHT) {
                    if (COL(i) != 0 && i - 9 == sq)
                        return true;
                    if (COL(i) != 7 && i - 7 == sq)
                        return true;
                }
                else {
                    if (COL(i) != 0 && i + 7 == sq)
                        return true;
                    if (COL(i) != 7 && i + 9 == sq)
                        return true;
                }
            }
            else
                for (j = 0; j < offsets[board.piece[i]]; ++j)
                    for (n = i;;) {
                        n = mailbox[mailbox64[n] + offset[board.piece[i]][j]];
                        if (n == -1)
                            break;
                        if (n == sq)
                            return true;
                        if (board.color[n] != EMPTY)
                            break;
                        if (!slide[board.piece[i]])
                            break;
                    }
        }
    return false;
}


/* gen() generates pseudo-legal moves for the current position.
   It scans the board to find friendly pieces and then determines
   what squares they attack. When it finds a piece/square
   combination, it calls gen_push to put the move on the "move
   stack." */

void gen(std::vector<gen_t>& workq, const node_t& board)
{
    int i, j, n;

    // so far, we have no moves for the current ply

    for (i = 0; i < 64; ++i)
        if (board.color[i] == board.side) {
            if (board.piece[i] == PAWN) {
                if (board.side == LIGHT) {
                    if (COL(i) != 0 && board.color[i - 9] == DARK)
                        gen_push(workq, board, i, i - 9, 17);
                    if (COL(i) != 7 && board.color[i - 7] == DARK)
                        gen_push(workq, board, i, i - 7, 17);
                    if (board.color[i - 8] == EMPTY) {
                        gen_push(workq, board, i, i - 8, 16);
                        if (i >= 48 && board.color[i - 16] == EMPTY)
                            gen_push(workq, board, i, i - 16, 24);
                    }
                }
                else {
                    if (COL(i) != 0 && board.color[i + 7] == LIGHT)
                        gen_push(workq, board, i, i + 7, 17);
                    if (COL(i) != 7 && board.color[i + 9] == LIGHT)
                        gen_push(workq, board, i, i + 9, 17);
                    if (board.color[i + 8] == EMPTY) {
                        gen_push(workq, board, i, i + 8, 16);
                        if (i <= 15 && board.color[i + 16] == EMPTY)
                            gen_push(workq, board, i, i + 16, 24);
                    }
                }
            }
            else
                for (j = 0; j < offsets[board.piece[i]]; ++j)
                    for (n = i;;) {
                        n = mailbox[mailbox64[n] + offset[board.piece[i]][j]];
                        if (n == -1)
                            break;
                        if (board.color[n] != EMPTY) {
                            if (board.color[n] == board.side ^ 1)
                                gen_push(workq, board, i, n, 1);
                            break;
                        }
                        gen_push(workq, board, i, n, 0);
                        if (!slide[board.piece[i]])
                            break;
                    }
        }

    // generate castle moves
    if (board.side == LIGHT) {
        if (board.castle & 1)
            gen_push(workq, board, E1, G1, 2);
        if (board.castle & 2)
            gen_push(workq, board, E1, C1, 2);
    }
    else {
        if (board.castle & 4)
            gen_push(workq, board, E8, G8, 2);
        if (board.castle & 8)
            gen_push(workq, board, E8, C8, 2);
    }
    
    // generate en passant moves
    if (board.ep != -1) {
        if (board.side == LIGHT) {
            if (COL(board.ep) != 0 && board.color[board.ep + 7] == LIGHT && board.piece[board.ep + 7] == PAWN)
                gen_push(workq, board, board.ep + 7, board.ep, 21);
            if (COL(board.ep) != 7 && board.color[board.ep + 9] == LIGHT && board.piece[board.ep + 9] == PAWN)
                gen_push(workq, board, board.ep + 9, board.ep, 21);
        }
        else {
            if (COL(board.ep) != 0 && board.color[board.ep - 9] == DARK && board.piece[board.ep - 9] == PAWN)
                gen_push(workq, board, board.ep - 9, board.ep, 21);
            if (COL(board.ep) != 7 && board.color[board.ep - 7] == DARK && board.piece[board.ep - 7] == PAWN)
                gen_push(workq, board, board.ep - 7, board.ep, 21);
        }
    }
}


/* gen_push() puts a move on the move stack, unless it's a
   pawn promotion that needs to be handled by gen_promote().
   It also assigns a score to the move for alpha-beta move
   ordering. If the move is a capture, it uses MVV/LVA
   (Most Valuable Victim/Least Valuable Attacker). Otherwise,
   it uses the move's history heuristic value. Note that
   1,000,000 is added to a capture move's score, so it
   always gets ordered above a "normal" move. */

void gen_push(std::vector<gen_t>& workq, const node_t& board, int from, int to, int bits)
{
    gen_t g;
    
    if (bits & 16) {
        if (board.side == LIGHT) {
            if (to <= H8) {
                gen_promote(workq, from, to, bits);
                return;
            }
        }
        else {
            if (to >= A1) {
                gen_promote(workq, from, to, bits);
                return;
            }
        }
    }
    g.m.b.from = (char)from;
    g.m.b.to = (char)to;
    g.m.b.promote = 0;
    g.m.b.bits = (char)bits;
    if (board.color[to] != EMPTY)
        g.score = 1000000 + (board.piece[to] * 10) - board.piece[from];
    workq.push_back(g);
}


/* gen_promote() is just like gen_push(), only it puts 4 moves
   on the move stack, one for each possible promotion piece */

void gen_promote(std::vector<gen_t>& workq, int from, int to, int bits)
{
    int i;
    gen_t g;
    
    for (i = KNIGHT; i <= QUEEN; ++i) {
        g.m.b.from = (char)from;
        g.m.b.to = (char)to;
        g.m.b.promote = (char)i;
        g.m.b.bits = (char)(bits | 32);
        g.score = 1000000 + (i * 10);
        workq.push_back(g);
    }
}


/* makemove() makes a move. If the move is illegal, it
   undoes whatever it did and returns FALSE. Otherwise, it
   returns TRUE. */

bool makemove(node_t& board, move_bytes m)
{
    
    /* test to see if a castle move is legal and move the rook
       (the king is moved with the usual move code later) */
    if (m.bits & 2) {
        int from, to;

        if (in_check(board, board.side))
            return false;
        switch (m.to) {
            case 62:
                if (board.color[F1] != EMPTY || board.color[G1] != EMPTY ||
                        attack(board, F1, board.side ^ 1) || attack(board, G1, board.side ^ 1))
                    return false;
                from = H1;
                to = F1;
                break;
            case 58:
                if (board.color[B1] != EMPTY || board.color[C1] != EMPTY || board.color[D1] != EMPTY ||
                        attack(board, C1, board.side ^ 1) || attack(board, D1, board.side ^ 1))
                    return false;
                from = A1;
                to = D1;
                break;
            case 6:
                if (board.color[F8] != EMPTY || board.color[G8] != EMPTY ||
                        attack(board, F8, board.side ^ 1) || attack(board, G8, board.side ^ 1))
                    return false;
                from = H8;
                to = F8;
                break;
            case 2:
                if (board.color[B8] != EMPTY || board.color[C8] != EMPTY || board.color[D8] != EMPTY ||
                        attack(board, C8, board.side ^ 1) || attack(board, D8, board.side ^ 1))
                    return false;
                from = A8;
                to = D8;
                break;
            default:  /* shouldn't get here */
                from = -1;
                to = -1;
                break;
        }
        board.color[to] = board.color[from];
        board.piece[to] = board.piece[from];
        board.color[from] = EMPTY;
        board.piece[from] = EMPTY;
    }

    // back up information so we can take the move back later.

    if (board.hply >= board.hist_dat.size())
    {
        board.hist_dat.resize(board.hist_dat.size() + 10);
    }
    board.hist_dat[board.hply].m.b = m;
    board.hist_dat[board.hply].capture = board.piece[(int)m.to];
    board.hist_dat[board.hply].castle = board.castle;
    board.hist_dat[board.hply].ep = board.ep;
    board.hist_dat[board.hply].fifty = board.fifty;
    board.hist_dat[board.hply].hash = board.hash;
    board.ply++;
    board.hply++;

    /* update the castle, en passant, and
       fifty-move-draw variables */
    board.castle &= castle_mask[(int)m.from] & castle_mask[(int)m.to];
    if (m.bits & 8) {
        if (board.side == LIGHT)
            board.ep = m.to + 8;
        else
            board.ep = m.to - 8;
    }
    else
        board.ep = -1;
    if (m.bits & 17)
        board.fifty = 0;
    else
        board.fifty++;

    /* move the piece */
    board.color[(int)m.to] = board.side;
    if (m.bits & 32)
        board.piece[(int)m.to] = m.promote;
    else
        board.piece[(int)m.to] = board.piece[(int)m.from];
    board.color[(int)m.from] = EMPTY;
    board.piece[(int)m.from] = EMPTY;

    /* erase the pawn if this is an en passant move */
    if (m.bits & 4) {
        if (board.side == LIGHT) {
            board.color[m.to + 8] = EMPTY;
            board.piece[m.to + 8] = EMPTY;
        }
        else {
            board.color[m.to - 8] = EMPTY;
            board.piece[m.to - 8] = EMPTY;
        }
    }

    /* switch sides and test for legality (if we can capture
       the other guy's king, it's an illegal position and
       we need to return FALSE) */
    board.side ^= 1;
    
    if (in_check(board, board.side ^ 1)) {
        return false;
    }
    board.hash = set_hash(board);
    return true;
}
