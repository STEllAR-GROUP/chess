set( chx_SRCS main.cpp board.cpp data.cpp eval.cpp  search.cpp mpi_support.cpp minimax.cpp alphabeta.cpp xboard.cpp hpx_task.cpp)



add_hpx_executable(chx
    ESSENTIAL
    SOURCES ${chx_SRCS}
    )

#add_executable(chx ${chx_SRCS})

target_link_libraries(chx_exe ${EXTRA_LIBS})
