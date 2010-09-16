set( chx_SRCS main.cpp board.cpp data.cpp eval.cpp  search.cpp)


add_executable(chx ${chx_SRCS})
target_link_libraries(chx ${EXTRA_LIBS})
