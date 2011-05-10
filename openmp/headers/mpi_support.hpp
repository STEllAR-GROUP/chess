#ifndef MPI_SUPPORT_HPP
#define MPI_SUPPORT_HPP
#ifdef MPI_SUPPORT
#include <mpi.h>
extern int mpi_rank, mpi_size;
const int task_tag = 5;
#endif
#endif
