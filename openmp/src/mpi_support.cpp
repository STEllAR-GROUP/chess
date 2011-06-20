#include "mpi_support.hpp"
#include "search.hpp"

int mpi_size=1, mpi_rank=0;
std::vector<pcounter> mpi_task_array;

void int_from_chars(int& i1,char c1,char c2,char c3,char c4);
void int_to_chars(int i1,char& c1,char& c2,char& c3,char& c4);

worker_result results[worker_result_size];

int result_alloc(hash_t h,int d) {
    int n = abs(h^d)%worker_result_size;
    for(int i=0;i<worker_result_size;i++) {
        int q = (i+n)%worker_result_size;
        pthread_mutex_lock(&results[q].mut);
        bool done = false;
        if(results[q].depth == -1) {
            results[q].depth = d;
            results[q].hash = h;
            results[q].has_result = false;
            results[q].score = 0;
            done = true;
        }
        pthread_mutex_unlock(&results[q].mut);
        if(done)
            return q;
    }
    assert(false);
    return -1;
}

void mpi_task::start() {
#ifdef MPI_SUPPORT
    //int mpi_data[mpi_ints];
    int n = 0;
    for(int i=0;i<16;i++) {
        int v;
        int_from_chars(v,info->board.color[4*i],info->board.color[4*i+1],
                info->board.color[4*i+2],info->board.color[4*i+3]);
        mpi_data[n++] = v;
    }
    for(int i=0;i<16;i++) {
        int v;
        int_from_chars(v,info->board.piece[4*i],info->board.piece[4*i+1],
                info->board.piece[4*i+2],info->board.piece[4*i+3]);
        mpi_data[n++] = v;
    }
    mpi_data[n++] = info->board.hash;
    assert(info->board.depth >=0 && info->board.depth <= 7);
    mpi_data[n++] = info->board.depth;
    mpi_data[n++] = info->board.side;
    mpi_data[n++] = info->board.castle;
    mpi_data[n++] = info->board.ep;
    mpi_data[n++] = info->board.ply;
    mpi_data[n++] = info->board.hply;
    mpi_data[n++] = info->board.fifty;
    mpi_data[n++] = info->alpha;
    mpi_data[n++] = info->beta;
    mpi_data[n++] = pfunc;
    if(windex == -1)
        windex = result_alloc(info->board.hash,info->board.depth);
    assert(windex != -1);
    mpi_data[n++] = windex;
    mpi_data[n++] = info->board.hist_dat.size();
    for(int i=0;i<info->board.hist_dat.size();i++) {
        mpi_data[n++] = info->board.hist_dat[i];
    }

    assert(info->board.depth >= 0);
    MPI_Send(mpi_data,mpi_ints,MPI_INT,
            dest,WORK_ASSIGN_MESSAGE,MPI_COMM_WORLD);
#endif
}

const int MPI_Thread_count = chx_threads_per_proc();

void *do_mpi_thread(void *);

struct MPI_Thread {
    smart_ptr<search_info> info;
    pthread_t thread;
    pthread_mutex_t mut;
    pfunc_v pfunc;
    int windex;
    bool in_use;
    MPI_Thread() : in_use(false) {
        pthread_mutex_init(&mut,NULL);
    }
    bool alloc() {
        pthread_mutex_lock(&mut);
        bool allocated = !in_use;
        if(allocated)
            in_use = true;
        pthread_mutex_unlock(&mut);
        return allocated;
    }
    void free() {
        pthread_mutex_lock(&mut);
        in_use = false;
        pthread_mutex_unlock(&mut);
    }
    void start() {
        pthread_create(&thread,NULL,do_mpi_thread,this);
    }
};

void *do_mpi_thread(void *voidp) {
#ifdef MPI_SUPPORT
    MPI_Thread *mp = (MPI_Thread*)voidp;
    score_t result;
    if(mp->pfunc == search_ab_f) {
        result = search_ab(mp->info->board,mp->info->board.depth,mp->info->alpha,mp->info->beta);
    } else if(mp->pfunc == search_f)
        result = search(mp->info->board,mp->info->board.depth);
    else if(mp->pfunc == qeval_f)
        result = qeval(mp->info->board,mp->info->alpha,mp->info->beta);
    else if(mp->pfunc == strike_f)
        ;
    else
        abort();
    int result_data[2];
    result_data[1] = result;
    result_data[0] = mp->windex;
    assert(mp->windex != -1);
    MPI_Send(result_data,2,MPI_INT,
            0,WORK_COMPLETED,MPI_COMM_WORLD);
    mp->free();
#endif
    pthread_exit(NULL);
    return NULL;
}

void *mpi_worker(void *)
{
    int result_data[2];
#ifdef MPI_SUPPORT
    if(mpi_rank==0) {
        int count = mpi_size -1;
        while(count>0)
        {
            MPI_Status status;
            int err = MPI_Recv(result_data,3,MPI_INT,
                MPI_ANY_SOURCE,WORK_COMPLETED,MPI_COMM_WORLD,&status);
            assert(err == MPI_SUCCESS);
            if(result_data[0] == -1)
                count--;
            else {
                //pthread_mutex_lock(&res_mut);
                //results[result_data[0]][result_data[1]].set_result(result_data[2]);
                results[result_data[0]].set_result(result_data[1]);
                //pthread_mutex_unlock(&res_mut);
            }
        }
    } else {
        init_hash();
        MPI_Thread mpi_threads[MPI_Thread_count];
        while(true)
        {
            int mpi_data[mpi_ints];
            MPI_Status status;
            int err = MPI_Recv(mpi_data,mpi_ints,MPI_INT,
                    0,WORK_ASSIGN_MESSAGE,MPI_COMM_WORLD,&status);
            assert(err == MPI_SUCCESS);
            int n = 0;
            node_t board;
            for(int i=0;i<16;i++) {
                int_to_chars(mpi_data[n++],
                    board.color[4*i],board.color[4*i+1],
                    board.color[4*i+2],board.color[4*i+3]);
            }
            for(int i=0;i<16;i++) {
                int_to_chars(mpi_data[n++],
                    board.piece[4*i],board.piece[4*i+1],
                    board.piece[4*i+2],board.piece[4*i+3]);
            }
            board.hash = mpi_data[n++];
            board.depth = mpi_data[n++];
            if(board.depth == -1) {
                result_data[0] = -1;
                MPI_Send(result_data,2,MPI_INT,
                    0,WORK_COMPLETED,MPI_COMM_WORLD);
                MPI_Finalize();
                exit(0);
            }
            assert(board.depth >= 0);
            board.side =  mpi_data[n++];
            board.castle = mpi_data[n++];
            board.ep = mpi_data[n++];
            board.ply = mpi_data[n++];
            board.hply = mpi_data[n++];
            board.fifty = mpi_data[n++];
            score_t alpha = mpi_data[n++];
            score_t beta = mpi_data[n++];
            pfunc_v func = (pfunc_v)mpi_data[n++];
            int windex = mpi_data[n++];
            int hist_size = mpi_data[n++];

            if(board.hply > 0) {
                //int fifty_array[50];
                /*
                FixedArray<int,50> fifty_array;
                int err = MPI_Recv(fifty_array.ptr(),board.hply,MPI_INT,
                        0,WORK_SUPPLEMENT,MPI_COMM_WORLD,&status);
                assert(err == MPI_SUCCESS);
                */
                board.hist_dat.resize(hist_size);
                for(int i=0;i<hist_size;i++)
                    board.hist_dat[i] = mpi_data[n++];//fifty_array[i];
            }
            
            for(int i=0;i<MPI_Thread_count;i++) {
                if(mpi_threads[i].alloc()) {
                    smart_ptr<search_info> info = new search_info(board);
                    info->beta = beta;
                    info->alpha = alpha;
                    info->depth = board.depth;
                    MPI_Thread *mp = &mpi_threads[i];
                    mp->windex = windex;
                    mp->pfunc = func;
                    mp->info = info;
                    mp->start();
                    break;
                }
            }
        }
    }
#endif
    return NULL;
}

void int_from_chars(int& i1,char c1,char c2,char c3,char c4) {
    i1 = c1 << 8*3 | c2 << 8*2 | c3 << 8 | c4;
}

void int_to_chars(int i1,char& c1,char& c2,char& c3,char& c4) {
    c4 = i1 & 0xFF;
    c3 = (i1 >> 8) & 0xFF;
    c2 = (i1 >> 8*2) & 0xFF;
    c1 = (i1 >> 8*3) & 0xFF;
}
