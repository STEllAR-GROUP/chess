set( chx_SRCS main.cpp board.cpp data.cpp eval.cpp  search.cpp)


add_executable(chx ${chx_SRCS})
set_target_properties(chx PROPERTIES 
                                COMPILE_FLAGS -fopenmp
                                LINK_FLAGS -lgomp)
target_link_libraries(chx ${EXTRA_LIBS})
