#ifndef BOOK_H
#define BOOK_H
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "node.hpp"
#include "defs.hpp"
#include "data.hpp"
#include "main.hpp"

void open_book();
void close_book();
int book_move(std::vector<gen_t>& workq, node_t& board);
BOOL book_match(char *s1, char *s2);

#endif
