/*
 *	PROTOS.H
 *	
 *  Collection of the prototypes for the chess program
 *
 */


/* prototypes */

/* board.c */
void init_board(board_t *board);
BOOL in_check(board_t board, int s);
BOOL attack(board_t board, int sq, int s);
int genmoves(board_t board, gen_t *g, int side, int history[64][64]);
int gen_caps(board_t board, gen_t *g, int side, int history[64][64]);
void gen_push(int from, int to, int bits, gen_t *g, int index, board_t board, int side, int history[64][64]);
void gen_promote(int from, int to, int bits, gen_t *g, int index);
BOOL makeourmove(board_t board, move_bytes m, board_t *newboard, int side);
void init_hash();
int hash_rand();
int set_hash(board_t board, int side);

/* book.c */
void open_book();
void close_book();
int book_move(hist_t *history);
BOOL book_match(char *s1, char *s2);

/* search2.c */
int pickbestmove(board_t board, int max_depth, int side, int mov, hist_t *hist);
int search(int alpha, int beta, int depth, board_t board, int side, int ply, int history[64][64], BOOL follow_pv);
int quiesce(int alpha,int beta, board_t board, int side, int ply, int history[64][64], BOOL follow_pv);
void sort(gen_t *gen_dat, int last_move);
BOOL sort_pv(gen_t *gen_dat,  int ply, int lastmove);
void check_repeating_moves(hist_t *hist, int history[64][64], board_t board, int side);

/* eval.c */
int eval(board_t board, int side);
int eval_light_pawn(int sq);
int eval_dark_pawn(int sq);
int eval_light_king(int sq);
int eval_lkp(int f);
int eval_dark_king(int sq);
int eval_dkp(int f);

/* main.c */
int main();

/* file.c */
FILE* set_output(FILE *out, int sside);
int close_output(FILE *out);
void seed_rand();

/* mpi.c */
int mpi();
int get_ms();
void selectOptions();
void parseArgs(int argc, char **argv);
int parse_move(char *s);
char *move_str(move_bytes m);
void print_board(board_t board);
void print_result(board_t board, int side, int xside);
void initialize(int sside);
void ending();
int start_network(int role, char *host, int portno);
void close_network();
int send_move(char *move);
int get_move(char *buff, int len);
int assign_work(int worker, int alpha, int beta, int depth, board_t board, int side, int ply, int follow_pv);
int request_worker();

/* mpimanager.c */
int mpimanager(int size);

/* mpihelper.c */
int mpihelper(int rank);
