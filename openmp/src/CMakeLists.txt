IF(HPX_FOUND)
set( chx_SRCS 
        main.cpp
        board.cpp 
        data.cpp
        eval.cpp
        search.cpp
        task_component.cpp
        mpi_support.cpp
        minimax.cpp
        alphabeta.cpp
        hpx_task.cpp)

    add_hpx_executable(chx
        ESSENTIAL
        SOURCES ${chx_SRCS}
        )
    target_link_libraries(chx_exe ${EXTRA_LIBS})
ELSE (HPX_FOUND)
set( chx_SRCS 
        main.cpp
        board.cpp 
        data.cpp
        eval.cpp
        search.cpp
        mpi_support.cpp
        minimax.cpp
        alphabeta.cpp)

    add_executable(chx ${chx_SRCS})
    target_link_libraries(chx ${EXTRA_LIBS})
ENDIF (HPX_FOUND)

