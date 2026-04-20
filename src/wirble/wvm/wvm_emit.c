#include "wvm_encoder.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static int
wvm_buffer_grow (WVMBuffer *buf, size_t extra)
{
  uint8_t *grown;
  size_t needed;
  size_t newCap;

  if (buf == NULL)
    {
      return 0;
    }
  needed = buf->len + extra;
  if (needed <= buf->cap)
    {
      return 1;
    }
  newCap = buf->cap == 0u ? 64u : buf->cap;
  while (newCap < needed)
    {
      newCap *= 2u;
    }
  grown = (uint8_t *) realloc (buf->data, newCap);
  if (grown == NULL)
    {
      return 0;
    }
  buf->data = grown;
  buf->cap = newCap;
  return 1;
}

WVMBuffer
wvmBufferCreate (WVMState *vm, size_t initialCap)
{
  WVMBuffer buf;

  (void) vm;
  buf.data = NULL;
  buf.len = 0u;
  buf.cap = 0u;
  if (initialCap != 0u)
    {
      buf.data = (uint8_t *) malloc (initialCap);
      if (buf.data != NULL)
        {
          buf.cap = initialCap;
        }
    }
  return buf;
}

void
wvmBufferDestroy (WVMState *vm, WVMBuffer *buf)
{
  (void) vm;
  if (buf == NULL)
    {
      return;
    }
  free (buf->data);
  buf->data = NULL;
  buf->len = 0u;
  buf->cap = 0u;
}

#define WVM_WRITE_SCALAR(NAME, TYPE)                                           \
  void NAME (WVMBuffer *buf, TYPE val)                                         \
  {                                                                            \
    if (wvm_buffer_grow (buf, sizeof (val)))                                   \
      {                                                                        \
        memcpy (buf->data + buf->len, &val, sizeof (val));                     \
        buf->len += sizeof (val);                                              \
      }                                                                        \
  }
WVM_WRITE_SCALAR (wvmBufferWriteU8, uint8_t)
WVM_WRITE_SCALAR (wvmBufferWriteU16, uint16_t)
WVM_WRITE_SCALAR (wvmBufferWriteU32, uint32_t)
WVM_WRITE_SCALAR (wvmBufferWriteU64, uint64_t)
WVM_WRITE_SCALAR (wvmBufferWriteI32, int32_t)
WVM_WRITE_SCALAR (wvmBufferWriteF64, double)
#undef WVM_WRITE_SCALAR

void
wvmBufferWriteBytes (WVMBuffer *buf, const uint8_t *data, size_t len)
{
  if (buf == NULL || (len != 0u && data == NULL) || !wvm_buffer_grow (buf, len))
    {
      return;
    }
  memcpy (buf->data + buf->len, data, len);
  buf->len += len;
}

void
wvmBufferWriteString (WVMBuffer *buf, const char *str)
{
  uint32_t len = str == NULL ? 0u : (uint32_t) strlen (str);
  wvmBufferWriteU32 (buf, len);
  if (len != 0u)
    {
      wvmBufferWriteBytes (buf, (const uint8_t *) str, len);
    }
}

WVMReader
wvmReaderCreate (const uint8_t *data, size_t len)
{
  WVMReader reader;
  reader.data = data;
  reader.len = len;
  reader.pos = 0u;
  return reader;
}

static int
wvm_reader_pull (WVMReader *r, void *out, size_t len)
{
  if (r == NULL || out == NULL || r->pos + len > r->len)
    {
      return 0;
    }
  memcpy (out, r->data + r->pos, len);
  r->pos += len;
  return 1;
}

#define WVM_READ_SCALAR(NAME, TYPE)                                            \
  TYPE NAME (WVMReader *r)                                                     \
  {                                                                            \
    TYPE value = (TYPE) 0;                                                     \
    (void) wvm_reader_pull (r, &value, sizeof (value));                        \
    return value;                                                              \
  }
WVM_READ_SCALAR (wvmReaderReadU8, uint8_t)
WVM_READ_SCALAR (wvmReaderReadU16, uint16_t)
WVM_READ_SCALAR (wvmReaderReadU32, uint32_t)
WVM_READ_SCALAR (wvmReaderReadU64, uint64_t)
WVM_READ_SCALAR (wvmReaderReadI32, int32_t)
WVM_READ_SCALAR (wvmReaderReadF64, double)
#undef WVM_READ_SCALAR

int
wvmReaderReadBytes (WVMReader *r, uint8_t *out, size_t len)
{
  return wvm_reader_pull (r, out, len);
}

const char *
wvmReaderReadString (WVMReader *r, uint32_t *outLen)
{
  uint32_t len;
  char *text;

  len = wvmReaderReadU32 (r);
  if (outLen != NULL)
    {
      *outLen = len;
    }
  text = (char *) malloc ((size_t) len + 1u);
  if (text == NULL)
    {
      return NULL;
    }
  if (len != 0u && !wvmReaderReadBytes (r, (uint8_t *) text, len))
    {
      free (text);
      return NULL;
    }
  text[len] = '\0';
  return text;
}

int
wvmReaderEOF (const WVMReader *r)
{
  return r == NULL || r->pos >= r->len;
}

int
wvmSerializePrototype (const WVMPrototype *proto, WVMBuffer *buf)
{
  uint32_t index;

  if (proto == NULL || buf == NULL)
    {
      return 0;
    }
  wvmBufferWriteString (buf, proto->name);
  wvmBufferWriteString (buf, proto->source);
  wvmBufferWriteU16 (buf, proto->regCount);
  wvmBufferWriteU16 (buf, proto->paramCount);
  wvmBufferWriteU8 (buf, proto->isVararg);
  wvmBufferWriteU8 (buf, 0u);
  wvmBufferWriteU32 (buf, proto->instrCount);
  wvmBufferWriteU32 (buf, proto->constCount);
  wvmBufferWriteU32 (buf, proto->strCount);
  wvmBufferWriteU32 (buf, proto->protoCount);
  for (index = 0u; index < proto->instrCount; ++index)
    {
      wvmBufferWriteU64 (buf, proto->instrs[index]);
    }
  for (index = 0u; index < proto->constCount; ++index)
    {
      wvmBufferWriteU64 (buf, proto->consts[index]);
    }
  wvmBufferWriteU32 (buf, proto->lineMap == NULL ? 0u : proto->instrCount);
  for (index = 0u; proto->lineMap != NULL && index < proto->instrCount; ++index)
    {
      wvmBufferWriteU32 (buf, proto->lineMap[index]);
    }
  return 1;
}

WVMPrototype *
wvmDeserializePrototype (WVMState *vm, WVMReader *reader)
{
  WVMPrototype *proto;
  uint32_t instrCount;
  uint32_t constCount;
  uint32_t strCount;
  uint32_t protoCount;
  uint32_t lineCount;
  uint32_t index;

  if (reader == NULL)
    {
      return NULL;
    }
  proto = (WVMPrototype *) calloc (1u, sizeof (*proto));
  if (proto == NULL)
    {
      return NULL;
    }
  proto->header.type = WVM_OBJ_FUNCTION;
  proto->name = wvmReaderReadString (reader, NULL);
  proto->source = wvmReaderReadString (reader, NULL);
  proto->regCount = wvmReaderReadU16 (reader);
  proto->paramCount = wvmReaderReadU16 (reader);
  proto->isVararg = wvmReaderReadU8 (reader);
  (void) wvmReaderReadU8 (reader);
  instrCount = wvmReaderReadU32 (reader);
  constCount = wvmReaderReadU32 (reader);
  strCount = wvmReaderReadU32 (reader);
  protoCount = wvmReaderReadU32 (reader);
  proto->instrCount = instrCount;
  proto->constCount = constCount;
  proto->strCount = strCount;
  proto->protoCount = protoCount;
  proto->instrs = instrCount == 0u ? NULL
                                   : (WVMInstr *) calloc (instrCount, sizeof (*proto->instrs));
  proto->consts = constCount == 0u ? NULL
                                   : (WVMValue *) calloc (constCount, sizeof (*proto->consts));
  if ((instrCount != 0u && proto->instrs == NULL)
      || (constCount != 0u && proto->consts == NULL))
    {
      wvmPrototypeDestroyInternal (vm, proto);
      return NULL;
    }
  for (index = 0u; index < instrCount; ++index)
    {
      proto->instrs[index] = wvmReaderReadU64 (reader);
    }
  for (index = 0u; index < constCount; ++index)
    {
      proto->consts[index] = wvmReaderReadU64 (reader);
    }
  lineCount = wvmReaderReadU32 (reader);
  if (lineCount != 0u)
    {
      proto->lineMap = (uint32_t *) calloc (lineCount, sizeof (*proto->lineMap));
      if (proto->lineMap == NULL)
        {
          wvmPrototypeDestroyInternal (vm, proto);
          return NULL;
        }
      for (index = 0u; index < lineCount; ++index)
        {
          proto->lineMap[index] = wvmReaderReadU32 (reader);
        }
    }
  return proto;
}

int
wvmWriteObjectMemory (WVMState *vm, const WVMPrototype *proto, uint8_t **outBuf,
                      size_t *outLen)
{
  WVMBuffer buf;
  WVMFileHeader header;

  if (outBuf == NULL || outLen == NULL || !wvmVerifyPrototype (vm, proto))
    {
      return 0;
    }
  buf = wvmBufferCreate (vm, 256u);
  memset (&header, 0, sizeof (header));
  header.magic = WVM_MAGIC_WVMO;
  header.versionMajor = WVM_FORMAT_VERSION_MAJOR;
  header.versionMinor = WVM_FORMAT_VERSION_MINOR;
  header.sectionCount = 1u;
  header.timestamp = (uint64_t) time (NULL);
  wvmBufferWriteBytes (&buf, (const uint8_t *) &header, sizeof (header));
  if (!wvmSerializePrototype (proto, &buf))
    {
      wvmBufferDestroy (vm, &buf);
      return 0;
    }
  *outBuf = buf.data;
  *outLen = buf.len;
  return 1;
}

int
wvmWriteObject (WVMState *vm, const WVMPrototype *proto, const char *path)
{
  uint8_t *buffer;
  size_t len;
  FILE *out;

  if (path == NULL || !wvmWriteObjectMemory (vm, proto, &buffer, &len))
    {
      return 0;
    }
  out = fopen (path, "wb");
  if (out == NULL)
    {
      free (buffer);
      return 0;
    }
  fwrite (buffer, 1u, len, out);
  fclose (out);
  free (buffer);
  return 1;
}

WVMPrototype *
wvmReadObjectMemory (WVMState *vm, const uint8_t *data, size_t len)
{
  WVMReader reader;
  WVMFileHeader header;
  WVMPrototype *proto;

  if (data == NULL || len < sizeof (header))
    {
      wvmSetErrorf (vm, "bytecode buffer is too small");
      return NULL;
    }
  reader = wvmReaderCreate (data, len);
  if (!wvmReaderReadBytes (&reader, (uint8_t *) &header, sizeof (header))
      || header.magic != WVM_MAGIC_WVMO)
    {
      wvmSetErrorf (vm, "invalid WVM object header");
      return NULL;
    }
  proto = wvmDeserializePrototype (vm, &reader);
  if (proto == NULL || !wvmVerifyPrototype (vm, proto))
    {
      wvmPrototypeDestroyInternal (vm, proto);
      return NULL;
    }
  return proto;
}

WVMPrototype *
wvmReadObject (WVMState *vm, const char *path)
{
  FILE *in;
  long size;
  uint8_t *buffer;
  WVMPrototype *proto;

  if (path == NULL)
    {
      return NULL;
    }
  in = fopen (path, "rb");
  if (in == NULL)
    {
      return NULL;
    }
  fseek (in, 0L, SEEK_END);
  size = ftell (in);
  rewind (in);
  if (size < 0L)
    {
      fclose (in);
      return NULL;
    }
  buffer = (uint8_t *) malloc ((size_t) size);
  if (buffer == NULL)
    {
      fclose (in);
      return NULL;
    }
  if (fread (buffer, 1u, (size_t) size, in) != (size_t) size)
    {
      fclose (in);
      free (buffer);
      return NULL;
    }
  fclose (in);
  proto = wvmReadObjectMemory (vm, buffer, (size_t) size);
  free (buffer);
  return proto;
}

WVMPrototype *
wvmLoadBytecode (WVMState *vm, const uint8_t *data, size_t len)
{
  return wvmReadObjectMemory (vm, data, len);
}

WVMPrototype *
wvmLoadFile (WVMState *vm, const char *path)
{
  return wvmReadObject (vm, path);
}

WVMPrototype *
wvmLoadString (WVMState *vm, const char *source, const char *name)
{
  (void) vm;
  (void) source;
  (void) name;
  return NULL;
}

int
wvmVerifyFile (WVMState *vm, const char *path)
{
  WVMPrototype *proto = wvmReadObject (vm, path);
  if (proto == NULL)
    {
      return 0;
    }
  wvmPrototypeDestroyInternal (vm, proto);
  return 1;
}

int
wvmVerifyChecksum (const WVMFileHeader *header, const uint8_t *payload,
                   size_t payloadLen)
{
  (void) header;
  (void) payload;
  (void) payloadLen;
  return 1;
}
