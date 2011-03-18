#ifndef HASH_HPP
#define HASH_HPP
#include <stdint.h>
#include <pthread.h>
#include <stdio.h>

typedef uint64_t hash_t;

struct zkey_t {
  hash_t hash;
  int score;
  int depth;
};

struct bucket_t {
  pthread_mutex_t mutex;
  zkey_t *table;
  size_t size;
  bucket_t(int M) : size(M) { pthread_mutex_init(&mutex, NULL); table = new zkey_t[M]; }
  ~bucket_t() {}
  void lock() { pthread_mutex_lock(&mutex); }
  void unlock() { pthread_mutex_unlock(&mutex); }
  zkey_t *get(int s) { return &table[s]; }
  void init() { 
    for(size_t i = 0; i < size; ++i)
    {
      table[i].hash = 0;
      table[i].score = 0;
      table[i].depth = 0;
    }
  }
};

#endif
