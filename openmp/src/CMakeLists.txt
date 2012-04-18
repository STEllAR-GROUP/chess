set( chx_SRCS 
        main.cpp
        board.cpp 
        data.cpp
        eval.cpp
        search.cpp
        mpi_support.cpp
        minimax.cpp
        alphabeta.cpp
        hpx_task.cpp)

IF(HPX_FOUND)
    add_hpx_executable(chx
        ESSENTIAL
        SOURCES ${chx_SRCS}
        )
ELSE (HPX_FOUND)
    add_executable(chx ${chx_SRCS})
ENDIF (HPX_FOUND)

target_link_libraries(chx_exe ${EXTRA_LIBS})
