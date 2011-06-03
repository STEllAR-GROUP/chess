#ifndef HERE_HPP
#define HERE_HPP 1
#include "mpi_support.hpp"
#define HERE std::cout << "HERE " << mpi_rank << ": " << __FILE__ << ", " << __LINE__ << std::flush << std::endl;
#define VAR(X) #X << "=" << (X) << " "
#endif
