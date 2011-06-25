////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2011 Steve Brandt and Philip LeBlanc
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file BOOST_LICENSE_1_0.rst or copy at http://www.boost.org/LICENSE_1_0.txt)
////////////////////////////////////////////////////////////////////////////////
#ifndef HERE_HPP
#define HERE_HPP 1
#include "mpi_support.hpp"
#define HERE std::cout << "HERE " << mpi_rank << ": " << __FILE__ << ", " << __LINE__ << std::flush << std::endl;
#define VAR(X) #X << "=" << (X) << " "
#endif
