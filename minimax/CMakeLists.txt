PROJECT( chx )
cmake_minimum_required(VERSION 2.6)
SET (CMAKE_BUILD_TYPE Debug)

include_directories(.)
include_directories ("${PROJECT_SOURCE_DIR}/headers")
include_directories ("${PROJECT_SOURCE_DIR}/liboptlist")
add_subdirectory (liboptlist)
set (EXTRA_LIBS ${EXTRA_LIBS} optlist readline)

add_subdirectory (src)
