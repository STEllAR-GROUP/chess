#ifndef HASH_HPP
#define HASH_HPP
#include <stdint.h>
#include <pthread.h>
#include <stdio.h>

typedef uint64_t hash_t;

const int zdepth = 2;
const int N = 8;  // bits for bucket size
const int M = 8;  // bits for table size N+M is total bits that can be indexed by transposition table
const int bucket_size = 1<<N;//pow(2, N);
const int table_size = 1<<M;//pow(2, M);

struct zkey_t {
  hash_t hash;
  int score;
  int depth;
};

struct bucket_t {
  pthread_mutex_t mutex;
  zkey_t table[table_size];
  bucket_t() { pthread_mutex_init(&mutex, NULL); }
  ~bucket_t() {}
  void lock() { pthread_mutex_lock(&mutex); }
  void unlock() { pthread_mutex_unlock(&mutex); }
  zkey_t *get(int s) { return &table[s]; }
  void init() { 
    for(size_t i = 0; i < table_size; ++i)
    {
      table[i].hash = 0;
      table[i].score = 0;
      table[i].depth = 0;
    }
  }
};

#endif
