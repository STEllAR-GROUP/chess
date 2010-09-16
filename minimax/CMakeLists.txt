PROJECT( chx )
cmake_minimum_required(VERSION 2.8)
SET (CMAKE_BUILD_TYPE Debug)

include_directories ("${PROJECT_SOURCE_DIR}/headers")

include_directories ("${PROJECT_SOURCE_DIR}/liboptlist")
add_subdirectory (liboptlist)
set (EXTRA_LIBS ${EXTRA_LIBS} optlist)

add_subdirectory (src)


