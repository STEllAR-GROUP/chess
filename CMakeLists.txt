# Copyright (c) 2012 Steve Brandt and Philip LeBlanc
# Copyright (c) 2012 Bryce Adelstein-Lelbach
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# TODO: Add install code.

cmake_minimum_required(VERSION 2.8.4 FATAL_ERROR)

project(octopus CXX)

# Project version.
set(CHX_MAJOR_VERSION 0)
set(CHX_MINOR_VERSION 1)
set(CHX_PATCH_LEVEL   0)
set(CHX_SOVERSION     ${CHX_MAJOR_VERSION})
set(CHX_VERSION       "${CHX_MAJOR_VERSION}.${CHX_MINOR_VERSION}.${CHX_PATCH_LEVEL}")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake;${HPX_ROOT}/share/cmake-2.8/Modules)

#find_package(HPX)
if(HPX_FOUND)
  message("HPX was found.")

  include_directories(${HPX_INCLUDE_DIR})
  link_directories(${HPX_LIBRARY_DIR})

  add_definitions(-DHPX_SUPPORT)
else()
  message("Could not find HPX. Configuring without HPX support.")
endif()

find_package(MPI)

# Not implemented
#if(MPI_FOUND)
#  message("MPI was found.")
#  set(CMAKE_CXX_COMPILER "mpicxx")
#
#  add_definitions(-DMPI_SUPPORT)
#else()
#  message("Could not find MPI. Configuring without MPI support.")
#endif()

set(EXTRA_LIBS pthread)

find_package(Readline)

if(READLINE_FOUND)
  message("Readline was found.")

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DREADLINE_SUPPORT")
  set(EXTRA_LIBS ${EXTRA_LIBS} readline)
else()
  message("Could not find Readline. Configuring without Readline support.")
endif()

if(NOT HPX_FOUND)
  find_package(BOOST)

  if(Boost_FOUND)
    message("Boost was found.")

    include_directories(${BOOST_INCLUDE_DIR})
    link_directories(${BOOST_LIBRARY_DIR})

    add_definitions(-DBOOST_SUPPORT)
  else()
    message("Could not find Boost. Configuring without Boost support.")
  endif()
else()
  message("Boost was found.")
  add_definitions(-DBOOST_SUPPORT)
endif()

include_directories("${PROJECT_SOURCE_DIR}/headers")

add_subdirectory(src)

