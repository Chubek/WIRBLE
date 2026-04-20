#ifndef WIRBLE_COMMON_ARENA_H
#define WIRBLE_COMMON_ARENA_H

#include <stddef.h>

typedef struct WirbleArenaBlock
{
  struct WirbleArenaBlock *next;
  size_t capacity;
  size_t used;
  unsigned char data[];
} WirbleArenaBlock;

typedef struct WirbleArena
{
  WirbleArenaBlock *head;
  size_t block_size;
  size_t total_reserved;
} WirbleArena;

void wirble_arena_init (WirbleArena *arena, size_t block_size);
void wirble_arena_reset (WirbleArena *arena);
void wirble_arena_destroy (WirbleArena *arena);

void *wirble_arena_alloc_from (WirbleArena *arena, size_t size,
                               size_t alignment);
void *wirble_arena_calloc_from (WirbleArena *arena, size_t count, size_t size,
                                size_t alignment);
char *wirble_arena_strdup (WirbleArena *arena, const char *value);

size_t wirble_arena_total_reserved (const WirbleArena *arena);
size_t wirble_arena_total_used (const WirbleArena *arena);

void *wirble_arena_alloc (size_t size);

#endif /* WIRBLE_COMMON_ARENA_H */
