PROJECT( chx )
cmake_minimum_required(VERSION 2.6)
SET (CMAKE_BUILD_TYPE Debug)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})
find_package(Readline)

IF (READLINE_FOUND)
	SET( CMAKE_CXX_FLAGS "-D READLINE_SUPPORT")
	set (EXTRA_LIBS ${EXTRA_LIBS} readline)
ELSE (READLINE_FOUND)
	MESSAGE(STATUS "Disabling GNU Readline: Not found")
ENDIF (READLINE_FOUND)

include_directories ("${PROJECT_SOURCE_DIR}/headers")
include_directories ("${PROJECT_SOURCE_DIR}/liboptlist")
include_directories ("${PROJECT_SOURCE_DIR}/libini")
add_subdirectory (liboptlist)
add_subdirectory (libini)
set (EXTRA_LIBS ${EXTRA_LIBS} optlist ncurses ini pthread)

add_subdirectory (src)
