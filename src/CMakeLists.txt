# Copyright (c) 2012 Steve Brandt and Philip LeBlanc
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(sources
    main.cpp
    board.cpp 
    data.cpp
    eval.cpp
    search.cpp
    minimax.cpp
    log_board.cpp
    timer.cpp
    alphabeta.cpp)

if(HPX_FOUND)
  set(sources ${sources}
      #task_component.cpp Not used
      hpx_task.cpp)
 
  add_hpx_executable(chx
    ESSENTIAL
    COMPONENT_DEPENDENCIES iostreams
    SOURCES ${sources})
elseif(MPI_FOUND)

  add_executable(chx ${sources})
  target_link_libraries(chx ${EXTRA_LIBS})
else()
  add_executable(chx ${sources})
  target_link_libraries(chx ${EXTRA_LIBS})
endif()

