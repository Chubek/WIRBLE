#ifndef WIRBLE_TOS_H
#define WIRBLE_TOS_H

#include <lmdb.h>
#include <stddef.h>
#include <stdint.h>

#include "api-boilerplate.h"

WIRBLE_BEGIN_DECLS

/* Forward declarations */
struct MDSProgram;
struct MDSInstruction;
struct MDSMachine;
struct WILGraph;
struct MALModule;
/* ════════════════════════════════════════════════════════════════════
 *  §1  CORE TYPES
 * ════════════════════════════════════════════════════════════════════ */

typedef uint64_t TOSTraceId;

typedef uint32_t TOSVersionId;

typedef uint32_t TOSInstrId;
#define TOS_INVALID_TRACE ((TOSTraceId) - 1)
#define TOS_INVALID_VERSION ((TOSVersionId) - 1)

/* ════════════════════════════════════════════════════════════════════
 *  §2  TOS CONTEXT
 *  Main handle for the optimization system
 * ════════════════════════════════════════════════════════════════════ */

typedef struct TOSContext
{ /* LMDB environment for persistent caching */
  MDB_env *lmdbEnv;
  MDB_dbi peepholeDB; /* Cached peephole patterns */
  MDB_dbi subInstrDB; /* Fused sub-instructions */
  MDB_dbi symbolDB;   /* Symbol address cache */
  MDB_dbi traceDB;    /* JIT trace cache */
  MDB_dbi versionDB;  /* Multi-version optimization results */
  MDB_dbi profileDB;  /* Runtime profiling data */

  /* Target machine model */
  const struct MDSMachine *machine;
  /* Rewrite system */
  struct TOSRewriteSystem *rewriteSys;
  /* JIT infrastructure */
  struct TOSTraceRecorder *traceRecorder;
  struct TOSTraceCache *traceCache;
  /* Multi-versioning */
  struct TOSVersionManager *versionMgr;
  /* Statistics */
  uint64_t peepholesApplied;
  uint64_t subInstrCreated;
  uint64_t tracesCompiled;
  uint64_t versionsGenerated;
} TOSContext;
/* ════════════════════════════════════════════════════════════════════
 *  §3  INITIALIZATION & TEARDOWN
 * ════════════════════════════════════════════════════════════════════ */

typedef struct TOSConfig
{
  const char *lmdbPath; /* Path to LMDB database directory */
  size_t lmdbMapSize;   /* LMDB map size (default: 1GB) */
  int enablePeephole;
  int enableSubInstr;
  int enableMultiVersion;
  int enableJIT;
  int enableTracing;
  int maxVersionsPerOp;      /* Max versions to test (default: 4) */
  int versionTestIterations; /* Benchmark iterations per version */
} TOSConfig;
TOSContext *tos_create (const TOSConfig *cfg,
                        const struct MDSMachine *machine);
void tos_destroy (TOSContext *ctx);
/* ════════════════════════════════════════════════════════════════════
 *  §4  PEEPHOLE OPTIMIZATION
 *  Pattern-based target code rewriting
 * ════════════════════════════════════════════════════════════════════ */

typedef struct TOSPeepholePattern
{
  struct MDSInstruction *match; /* Pattern to match */
  uint32_t matchLen;
  struct MDSInstruction *replace; /* Replacement sequence */
  uint32_t replaceLen;
  int costDelta; /* Estimated cost improvement */
  const char *name;
} TOSPeepholePattern;
/* Add peephole pattern programmatically */
int tos_add_peephole (TOSContext *ctx, const TOSPeepholePattern *pattern);
/* Apply peepholes to target program */
int tos_apply_peepholes (TOSContext *ctx, struct MDSProgram *prog);
/* Load peephole patterns from cache */
int tos_load_peepholes_from_cache (TOSContext *ctx);
/* Save peephole patterns to cache */
int tos_save_peepholes_to_cache (TOSContext *ctx);
/* ════════════════════════════════════════════════════════════════════
 *  §5  SUB-INSTRUCTION FUSION
 *  Combine frequently co-occurring instructions into fused ops
 * ════════════════════════════════════════════════════════════════════ */

typedef struct TOSSubInstr
{
  uint32_t id;
  const char *name;
  struct MDSInstruction *component; /* Component instructions */
  uint32_t componentCount;
  uint64_t frequency; /* How often this pattern occurs */
  int speedup;        /* Measured speedup percentage */
  uint8_t instruction;
  uint32_t encodingSize;
} TOSSubInstr;
/* Register a new fused instruction pattern */

int tos_register_subinstr (TOSContext *ctx, const TOSSubInstr *si);
/* Scan program and fuse matching sequences */

int tos_apply_subinstr (TOSContext *ctx, struct MDSProgram *prog);
/* Cache management */

int tos_load_subinstr_cache (TOSContext *ctx);
int tos_save_subinstr_cache (TOSContext *ctx);
/* ════════════════════════════════════════════════════════════════════

    §6 MULTI-VERSION OPTIMIZATION
    Generate several versions and select the best-performing one
    ════════════════════════════════════════════════════════════════════ */

typedef struct TOSVersionCandidate
{
  TOSVersionId id;
  struct MDSProgram *program;
  uint64_t measuredCycles;
} TOSVersionCandidate;

typedef struct TOSVersionResult
{
  TOSVersionId bestVersion;
  uint64_t bestCycles;
} TOSVersionResult;
/* Generate alternative versions of a program */

int tos_generate_versions (
    TOSContext *ctx, const struct MDSProgram *input, TOSVersionCandidate *out,
    size_t *outCount);
/* Benchmark candidate versions */

int tos_benchmark_versions (
    TOSContext *ctx, TOSVersionCandidate *versions, size_t count);
/* Select the fastest version */

int tos_select_best_version (
    TOSContext *ctx, const TOSVersionCandidate *versions, size_t count,
    TOSVersionResult *result);
/* Cache best version result */

int tos_cache_version (
    TOSContext *ctx, const void *key, size_t keySize, TOSVersionId version);
/* Lookup cached version */

int tos_lookup_cached_version (
    TOSContext *ctx, const void *key, size_t keySize,
    TOSVersionId *outVersion);
/* ════════════════════════════════════════════════════════════════════

    §7 SYMBOL CACHE
    Persistent address lookup using LMDB
    ════════════════════════════════════════════════════════════════════ */

int tos_cache_symbol (
    TOSContext *ctx, const char *name, uint64_t address);
int tos_lookup_symbol (
    TOSContext *ctx, const char *name, uint64_t *outAddress);
/* ════════════════════════════════════════════════════════════════════

    §8 TRACE JIT
    Recording and compiling hot execution traces
    ════════════════════════════════════════════════════════════════════ */

typedef struct TOSTrace TOSTrace;

typedef struct TOSTraceVersion TOSTraceVersion;
/* Begin recording a trace */

TOSTraceId tos_trace_begin (TOSContext *ctx);
/* Append WIL fragment to trace */

int tos_trace_append (
    TOSContext *ctx, TOSTraceId id, const struct WILGraph *fragment);
/* End trace recording */

int tos_trace_end (TOSContext *ctx, TOSTraceId id);
/* Compile trace into optimized code */

int tos_trace_compile (TOSContext *ctx, TOSTraceId id);
/* Lookup compiled trace */

int tos_trace_lookup (
    TOSContext *ctx, const void *pc, TOSTraceId *outTrace);
/* ════════════════════════════════════════════════════════════════════

    §9 OPTIMIZATION PIPELINE
    ════════════════════════════════════════════════════════════════════ */

/* Run all enabled target optimizations */

int tos_optimize_program (TOSContext *ctx, struct MDSProgram *prog);
/* Optimize full MAL module (lowering pipeline entry) */

int tos_optimize_module (TOSContext *ctx, struct MALModule *module);
/* ════════════════════════════════════════════════════════════════════

    §10 LMDB UTILITY WRAPPERS
    Simplified helpers for cache operations
    ════════════════════════════════════════════════════════════════════ */

int tos_lmdb_put (
    TOSContext *ctx, MDB_dbi db, const void *key, size_t keySize,
    const void *value, size_t valueSize);
int tos_lmdb_get (
    TOSContext *ctx, MDB_dbi db, const void *key, size_t keySize, void **value,
    size_t *valueSize);
int tos_lmdb_del (
    TOSContext *ctx, MDB_dbi db, const void *key, size_t keySize);
/* ════════════════════════════════════════════════════════════════════

    §11 DEBUGGING / STATISTICS
    ════════════════════════════════════════════════════════════════════ */

void tos_dump_stats (const TOSContext *ctx);
void tos_reset_stats (TOSContext *ctx);

WIRBLE_END_DECLS

#endif /* WIRBLE_TOS_H */
