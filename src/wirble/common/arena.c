#include "arena.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define WIRBLE_ARENA_DEFAULT_BLOCK_SIZE 4096u

static size_t
wirble_max_size (size_t lhs, size_t rhs)
{
  return lhs > rhs ? lhs : rhs;
}

static size_t
wirble_align_up (size_t value, size_t alignment)
{
  size_t mask;

  if (alignment == 0u)
    {
      return value;
    }

  mask = alignment - 1u;
  return (value + mask) & ~mask;
}

static WirbleArenaBlock *
wirble_arena_block_create (size_t capacity)
{
  WirbleArenaBlock *block;

  block = (WirbleArenaBlock *) malloc (sizeof (*block) + capacity);
  if (block == NULL)
    {
      return NULL;
    }

  block->next = NULL;
  block->capacity = capacity;
  block->used = 0u;
  return block;
}

void
wirble_arena_init (WirbleArena *arena, size_t block_size)
{
  if (arena == NULL)
    {
      return;
    }

  arena->head = NULL;
  arena->block_size
      = block_size == 0u ? WIRBLE_ARENA_DEFAULT_BLOCK_SIZE : block_size;
  arena->total_reserved = 0u;
}

void
wirble_arena_reset (WirbleArena *arena)
{
  WirbleArenaBlock *block;
  WirbleArenaBlock *next;

  if (arena == NULL)
    {
      return;
    }

  block = arena->head;
  while (block != NULL)
    {
      next = block->next;
      free (block);
      block = next;
    }

  arena->head = NULL;
  arena->total_reserved = 0u;
}

void
wirble_arena_destroy (WirbleArena *arena)
{
  wirble_arena_reset (arena);
}

void *
wirble_arena_alloc_from (WirbleArena *arena, size_t size, size_t alignment)
{
  WirbleArenaBlock *block;
  size_t start;
  size_t capacity;

  if (arena == NULL || size == 0u)
    {
      return NULL;
    }

  if (alignment == 0u)
    {
      alignment = sizeof (void *);
    }

  block = arena->head;
  if (block != NULL)
    {
      start = wirble_align_up (block->used, alignment);
      if (start + size <= block->capacity)
        {
          block->used = start + size;
          return block->data + start;
        }
    }

  capacity = wirble_max_size (arena->block_size, size + alignment);
  block = wirble_arena_block_create (capacity);
  if (block == NULL)
    {
      return NULL;
    }

  block->next = arena->head;
  arena->head = block;
  arena->total_reserved += capacity;

  start = wirble_align_up (0u, alignment);
  block->used = start + size;
  return block->data + start;
}

void *
wirble_arena_calloc_from (WirbleArena *arena, size_t count, size_t size,
                          size_t alignment)
{
  void *ptr;
  size_t total_size;

  if (count == 0u || size == 0u)
    {
      return NULL;
    }

  total_size = count * size;
  if (size != 0u && total_size / size != count)
    {
      return NULL;
    }

  ptr = wirble_arena_alloc_from (arena, total_size, alignment);
  if (ptr != NULL)
    {
      memset (ptr, 0, total_size);
    }

  return ptr;
}

char *
wirble_arena_strdup (WirbleArena *arena, const char *value)
{
  char *copy;
  size_t length;

  if (arena == NULL || value == NULL)
    {
      return NULL;
    }

  length = strlen (value) + 1u;
  copy = (char *) wirble_arena_alloc_from (arena, length, sizeof (char));
  if (copy != NULL)
    {
      memcpy (copy, value, length);
    }

  return copy;
}

size_t
wirble_arena_total_reserved (const WirbleArena *arena)
{
  return arena == NULL ? 0u : arena->total_reserved;
}

size_t
wirble_arena_total_used (const WirbleArena *arena)
{
  const WirbleArenaBlock *block;
  size_t total;

  if (arena == NULL)
    {
      return 0u;
    }

  total = 0u;
  for (block = arena->head; block != NULL; block = block->next)
    {
      total += block->used;
    }

  return total;
}

void *
wirble_arena_alloc (size_t size)
{
  return calloc (1u, size);
}
