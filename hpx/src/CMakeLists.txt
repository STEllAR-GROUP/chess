set( chx_SRCS main.cpp board.cpp book.cpp data.cpp eval.cpp hash_table.cpp search.cpp)


add_hpx_executable(chx SOURCES ${chx_SRCS} DEPENDENCIES ${EXTRA_LIBS})
