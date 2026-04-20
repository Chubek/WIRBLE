#ifndef WIRBLE_COMMON_STRPOOL_H
#define WIRBLE_COMMON_STRPOOL_H

#include <stddef.h>

typedef struct WirbleStrpoolEntry
{
  struct WirbleStrpoolEntry *next;
  size_t length;
  char text[];
} WirbleStrpoolEntry;

typedef struct WirbleStrpool
{
  WirbleStrpoolEntry **buckets;
  size_t bucket_count;
  size_t size;
} WirbleStrpool;

void wirble_strpool_init (WirbleStrpool *pool);
void wirble_strpool_destroy (WirbleStrpool *pool);
const char *wirble_strpool_intern_with_length (WirbleStrpool *pool,
                                               const char *value,
                                               size_t length);
const char *wirble_strpool_intern (const char *value);
const char *wirble_strpool_intern_from_pool (WirbleStrpool *pool,
                                             const char *value);
size_t wirble_strpool_size (const WirbleStrpool *pool);

#endif /* WIRBLE_COMMON_STRPOOL_H */
