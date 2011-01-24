PROJECT( chx )
cmake_minimum_required(VERSION 2.6)
SET (CMAKE_BUILD_TYPE Debug)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})
find_package(Readline)

IF (READLINE_FOUND)
	SET( CMAKE_CXX_FLAGS "-D READLINE_SUPPORT")
ELSE (READLINE_FOUND)
	MESSAGE(STATUS "Disabling GNU Readline: Not found")
ENDIF (READLINE_FOUND)

include_directories ("${PROJECT_SOURCE_DIR}/headers")
include_directories ("${PROJECT_SOURCE_DIR}/liboptlist")
add_subdirectory (liboptlist)
set (EXTRA_LIBS ${EXTRA_LIBS} optlist readline)

add_subdirectory (src)
