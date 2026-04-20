#ifndef LMDB_H
#define LMDB_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MDB_env MDB_env;
typedef struct MDB_txn MDB_txn;
typedef struct MDB_cursor MDB_cursor;
typedef unsigned int MDB_dbi;
typedef uint32_t mdb_mode_t;

typedef struct MDB_val
{
  size_t mv_size;
  void *mv_data;
} MDB_val;

#ifdef __cplusplus
}
#endif

#endif /* LMDB_H */
