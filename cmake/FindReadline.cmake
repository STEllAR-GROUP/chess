# Copyright (c) 2012 Steve Brandt and Philip LeBlanc
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

find_path(READLINE_INCLUDE_DIR readline/readline.h)
find_library(READLINE_LIBRARY NAMES readline) 

if(READLINE_INCLUDE_DIR AND READLINE_LIBRARY)
   set(READLINE_FOUND TRUE)
endif(READLINE_INCLUDE_DIR AND READLINE_LIBRARY)

if(READLINE_FOUND)
   if(NOT Readline_FIND_QUIETLY)
      message(STATUS "Found GNU readline: ${READLINE_LIBRARY}")
   endif(NOT Readline_FIND_QUIETLY)
else()
   if(Readline_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find GNU readline")
   endif()
endif()

