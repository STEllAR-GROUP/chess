# - Finds OpenMP support
# This module can be used to detect OpenMP support in a compiler.
# If the compiler supports OpenMP, the flags required to compile with
# openmp support are set.  
#
# The following variables are set:
#   OpenMP_CXX_FLAGS - flags to add to the CXX compiler for OpenMP support
#   OPENMP_FOUND - true if openmp is detected
#
# Supported compilers can be found at http://openmp.org/wp/openmp-compilers/

# Copyright 2008, 2009 <André Rigland Brodtkorb> Andre.Brodtkorb@ifi.uio.no
#
# Redistribution AND use is allowed according to the terms of the New 
# BSD license. 
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.


# Modified by Marc-Andre Gardner
# 31/07/2009
# Fix unset uses

include(CheckCXXSourceCompiles)
include(FindPackageHandleStandardArgs)

message(STATUS "Compiler is ${CMAKE_C_COMPILER_ID}")

set(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS TRUE)
if(${CMAKE_C_COMPILER_ID} STREQUAL "Intel")
    set(OpenMP_C_FLAGS "-openmp")
    set(OpenMP_LINK_FLAGS "-liomp5 -lpthread")
else()
    set(OpenMP_C_FLAGS "-fopenmp")
    set(OpenMP_LINK_FLAGS "-lgomp")
endif() 
if(${CMAKE_CXX_COMPILER_ID} STREQUAL "Intel")
    set(OpenMP_CXX_FLAGS "-openmp")
    set(OpenMP_LINK_FLAGS "-liomp5 -lpthread")
else()
    set(OpenMP_CXX_FLAGS "-fopenmp")
    set(OpenMP_LINK_FLAGS "-lgomp")
endif() 
