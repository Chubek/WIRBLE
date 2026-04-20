#include "strpool.h"

#include <stdlib.h>
#include <string.h>

#define WIRBLE_STRPOOL_DEFAULT_BUCKETS 128u

static unsigned long
wirble_strpool_hash_bytes (const char *value, size_t length)
{
  unsigned long hash;
  size_t i;

  hash = 1469598103934665603ul;
  for (i = 0u; i < length; ++i)
    {
      hash ^= (unsigned long) (unsigned char) value[i];
      hash *= 1099511628211ul;
    }

  return hash;
}

static int
wirble_strpool_grow (WirbleStrpool *pool, size_t bucket_count)
{
  WirbleStrpoolEntry **buckets;
  WirbleStrpoolEntry **old_buckets;
  size_t old_bucket_count;
  size_t i;

  buckets
      = (WirbleStrpoolEntry **) calloc (bucket_count, sizeof (*buckets));
  if (buckets == NULL)
    {
      return 0;
    }

  old_buckets = pool->buckets;
  old_bucket_count = pool->bucket_count;

  pool->buckets = buckets;
  pool->bucket_count = bucket_count;

  for (i = 0u; i < old_bucket_count; ++i)
    {
      WirbleStrpoolEntry *entry;

      entry = old_buckets[i];
      while (entry != NULL)
        {
          WirbleStrpoolEntry *next;
          size_t slot;

          next = entry->next;
          slot
              = wirble_strpool_hash_bytes (entry->text, entry->length)
                % pool->bucket_count;
          entry->next = pool->buckets[slot];
          pool->buckets[slot] = entry;
          entry = next;
        }
    }

  free (old_buckets);
  return 1;
}

void
wirble_strpool_init (WirbleStrpool *pool)
{
  if (pool == NULL)
    {
      return;
    }

  pool->buckets = NULL;
  pool->bucket_count = 0u;
  pool->size = 0u;
}

void
wirble_strpool_destroy (WirbleStrpool *pool)
{
  size_t i;

  if (pool == NULL)
    {
      return;
    }

  for (i = 0u; i < pool->bucket_count; ++i)
    {
      WirbleStrpoolEntry *entry;

      entry = pool->buckets[i];
      while (entry != NULL)
        {
          WirbleStrpoolEntry *next;

          next = entry->next;
          free (entry);
          entry = next;
        }
    }

  free (pool->buckets);
  pool->buckets = NULL;
  pool->bucket_count = 0u;
  pool->size = 0u;
}

const char *
wirble_strpool_intern_with_length (WirbleStrpool *pool, const char *value,
                                   size_t length)
{
  unsigned long hash;
  size_t slot;
  WirbleStrpoolEntry *entry;

  if (pool == NULL || value == NULL)
    {
      return NULL;
    }

  if (pool->bucket_count == 0u
      && !wirble_strpool_grow (pool, WIRBLE_STRPOOL_DEFAULT_BUCKETS))
    {
      return NULL;
    }

  if (pool->size >= pool->bucket_count
      && !wirble_strpool_grow (pool, pool->bucket_count * 2u))
    {
      return NULL;
    }

  hash = wirble_strpool_hash_bytes (value, length);
  slot = hash % pool->bucket_count;

  for (entry = pool->buckets[slot]; entry != NULL; entry = entry->next)
    {
      if (entry->length == length && memcmp (entry->text, value, length) == 0)
        {
          return entry->text;
        }
    }

  entry = (WirbleStrpoolEntry *) malloc (sizeof (*entry) + length + 1u);
  if (entry == NULL)
    {
      return NULL;
    }

  entry->next = pool->buckets[slot];
  entry->length = length;
  memcpy (entry->text, value, length);
  entry->text[length] = '\0';
  pool->buckets[slot] = entry;
  ++pool->size;

  return entry->text;
}

const char *
wirble_strpool_intern_from_pool (WirbleStrpool *pool, const char *value)
{
  if (value == NULL)
    {
      return NULL;
    }

  return wirble_strpool_intern_with_length (pool, value, strlen (value));
}

const char *
wirble_strpool_intern (const char *value)
{
  static WirbleStrpool global_pool;
  static int initialized;

  if (!initialized)
    {
      wirble_strpool_init (&global_pool);
      initialized = 1;
    }

  return wirble_strpool_intern_from_pool (&global_pool, value);
}

size_t
wirble_strpool_size (const WirbleStrpool *pool)
{
  return pool == NULL ? 0u : pool->size;
}
