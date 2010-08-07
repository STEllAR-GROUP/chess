PROJECT( chx )
cmake_minimum_required(VERSION 2.8)
SET (CMAKE_BUILD_TYPE Debug)

set(HPX_MAJOR_VERSION 0)
set(HPX_MINOR_VERSION 2)
set(HPX_PATCH_LEVEL   0)
set(HPX_VERSION "${HPX_MAJOR_VERSION}.${HPX_MINOR_VERSION}.${HPX_PATCH_LEVEL}")
set(HPX_SOVERSION ${HPX_MAJOR_VERSION})

include($ENV{HPX_ROOT}/share/cmake/Hpx.cmake)
include($ENV{BOOST})

include_directories ("${PROJECT_SOURCE_DIR}/headers")

add_subdirectory (src)


