set( chx_SRCS main.cpp board.cpp data.cpp eval.cpp  search.cpp)


add_executable(chx ${chx_SRCS})

set_target_properties(chx PROPERTIES 
                                COMPILE_FLAGS "${OpenMP_CXX_FLAGS}"
                                LINK_FLAGS "${OpenMP_LINK_FLAGS}")

target_link_libraries(chx ${EXTRA_LIBS})
