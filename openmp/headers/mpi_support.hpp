#ifndef MPI_SUPPORT_HPP
#define MPI_SUPPORT_HPP
#ifdef MPI_SUPPORT
#include <mpi.h>
const int task_tag = 5;
#endif
extern int mpi_rank, mpi_size;
#endif
