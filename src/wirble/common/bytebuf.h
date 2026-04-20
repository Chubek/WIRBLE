#ifndef WIRBLE_COMMON_BYTEBUF_H
#define WIRBLE_COMMON_BYTEBUF_H

#include <stddef.h>
#include <stdint.h>

typedef struct WirbleByteBuf
{
  unsigned char *data;
  size_t size;
  size_t capacity;
} WirbleByteBuf;

void wirble_bytebuf_init (WirbleByteBuf *buffer);
void wirble_bytebuf_destroy (WirbleByteBuf *buffer);
void wirble_bytebuf_clear (WirbleByteBuf *buffer);

int wirble_bytebuf_reserve (WirbleByteBuf *buffer, size_t capacity);
int wirble_bytebuf_append (WirbleByteBuf *buffer, const void *data,
                           size_t size);
int wirble_bytebuf_append_byte (WirbleByteBuf *buffer, uint8_t byte);
int wirble_bytebuf_append_cstr (WirbleByteBuf *buffer, const char *text);
int wirble_bytebuf_read_file (WirbleByteBuf *buffer, const char *path);

size_t wirble_bytebuf_size (void);

#endif /* WIRBLE_COMMON_BYTEBUF_H */
