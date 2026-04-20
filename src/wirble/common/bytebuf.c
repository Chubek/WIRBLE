#include "bytebuf.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void
wirble_bytebuf_init (WirbleByteBuf *buffer)
{
  if (buffer == NULL)
    {
      return;
    }

  buffer->data = NULL;
  buffer->size = 0u;
  buffer->capacity = 0u;
}

void
wirble_bytebuf_destroy (WirbleByteBuf *buffer)
{
  if (buffer == NULL)
    {
      return;
    }

  free (buffer->data);
  buffer->data = NULL;
  buffer->size = 0u;
  buffer->capacity = 0u;
}

void
wirble_bytebuf_clear (WirbleByteBuf *buffer)
{
  if (buffer != NULL)
    {
      buffer->size = 0u;
    }
}

int
wirble_bytebuf_reserve (WirbleByteBuf *buffer, size_t capacity)
{
  unsigned char *new_data;
  size_t new_capacity;

  if (buffer == NULL)
    {
      return 0;
    }

  if (capacity <= buffer->capacity)
    {
      return 1;
    }

  new_capacity = buffer->capacity == 0u ? 64u : buffer->capacity;
  while (new_capacity < capacity)
    {
      new_capacity *= 2u;
    }

  new_data = (unsigned char *) realloc (buffer->data, new_capacity);
  if (new_data == NULL)
    {
      return 0;
    }

  buffer->data = new_data;
  buffer->capacity = new_capacity;
  return 1;
}

int
wirble_bytebuf_append (WirbleByteBuf *buffer, const void *data, size_t size)
{
  if (buffer == NULL || data == NULL || size == 0u)
    {
      return size == 0u;
    }

  if (!wirble_bytebuf_reserve (buffer, buffer->size + size))
    {
      return 0;
    }

  memcpy (buffer->data + buffer->size, data, size);
  buffer->size += size;
  return 1;
}

int
wirble_bytebuf_append_byte (WirbleByteBuf *buffer, uint8_t byte)
{
  return wirble_bytebuf_append (buffer, &byte, sizeof (byte));
}

int
wirble_bytebuf_append_cstr (WirbleByteBuf *buffer, const char *text)
{
  if (text == NULL)
    {
      return 0;
    }

  return wirble_bytebuf_append (buffer, text, strlen (text));
}

int
wirble_bytebuf_read_file (WirbleByteBuf *buffer, const char *path)
{
  FILE *file;
  unsigned char chunk[4096];
  size_t read_count;

  if (buffer == NULL || path == NULL)
    {
      return 0;
    }

  file = fopen (path, "rb");
  if (file == NULL)
    {
      return 0;
    }

  wirble_bytebuf_clear (buffer);
  while ((read_count = fread (chunk, 1u, sizeof (chunk), file)) != 0u)
    {
      if (!wirble_bytebuf_append (buffer, chunk, read_count))
        {
          fclose (file);
          return 0;
        }
    }

  fclose (file);
  return 1;
}

size_t
wirble_bytebuf_size (void)
{
  return 0u;
}
