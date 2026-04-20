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
typedef struct MALOperand MALOperand;

/* ════════════════════════════════════════════════════════════════════
 *  §1  CORE TYPES
 * ════════════════════════════════════════════════════════════════════ */

typedef uint64_t TOSTraceId;
typedef uint32_t TOSVersionId;
typedef uint32_t TOSInstrId;

#define TOS_INVALID_TRACE ((TOSTraceId) -1)
#define TOS_INVALID_VERSION ((TOSVersionId) -1)
#define TOS_INVALID_INSTR ((TOSInstrId) -1)

typedef enum TOSOrderKind
{
  TOS_ORDER_INPUT = 0,
  TOS_ORDER_REVERSE_POSTORDER
} TOSOrderKind;

typedef struct TOSInst
{
  TOSInstrId id;
  uint32_t blockId;
  uint32_t order;
  const struct MDSInstruction *desc;
  MALOperand *operands;
  uint32_t operandCount;
} TOSInst;

typedef struct TOSBlock
{
  uint32_t id;
  uint32_t layoutIndex;
  TOSInstrId firstInstr;
  uint32_t instrCount;
  uint32_t *successors;
  uint32_t successorCount;
} TOSBlock;

typedef struct TOSProgram
{
  const struct MDSMachine *machine;
  TOSBlock *blocks;
  uint32_t blockCount;
  TOSInst *instructions;
  uint32_t instrCount;
  TOSOrderKind orderKind;
  int isFinalized;
} TOSProgram;

/* ════════════════════════════════════════════════════════════════════
 *  §2  TOS STREAM BUILDING
 * ════════════════════════════════════════════════════════════════════ */

TOSProgram *tosBuildFromMDS (const struct MDSProgram *program);
void tosDestroyProgram (TOSProgram *program);
int tosFinalizeProgram (TOSProgram *program);
int tosValidateProgram (const TOSProgram *program);

/* ════════════════════════════════════════════════════════════════════
 *  §3  TOS CONTEXT
 *  Main handle for the optimization system
 * ════════════════════════════════════════════════════════════════════ */

typedef struct TOSContext
{
  MDB_env *lmdbEnv;
  MDB_dbi peepholeDB;
  MDB_dbi subInstrDB;
  MDB_dbi symbolDB;
  MDB_dbi traceDB;
  MDB_dbi versionDB;
  MDB_dbi profileDB;

  const struct MDSMachine *machine;
  struct TOSRewriteSystem *rewriteSys;
  struct TOSTraceRecorder *traceRecorder;
  struct TOSTraceCache *traceCache;
  struct TOSVersionManager *versionMgr;

  uint64_t peepholesApplied;
  uint64_t subInstrCreated;
  uint64_t tracesCompiled;
  uint64_t versionsGenerated;
} TOSContext;

/* ════════════════════════════════════════════════════════════════════
 *  §4  INITIALIZATION & TEARDOWN
 * ════════════════════════════════════════════════════════════════════ */

typedef struct TOSConfig
{
  const char *lmdbPath;
  size_t lmdbMapSize;
  int enablePeephole;
  int enableSubInstr;
  int enableMultiVersion;
  int enableJIT;
  int enableTracing;
  int maxVersionsPerOp;
  int versionTestIterations;
} TOSConfig;

TOSContext *tos_create (const TOSConfig *cfg,
                        const struct MDSMachine *machine);
void tos_destroy (TOSContext *ctx);

/* ════════════════════════════════════════════════════════════════════
 *  §5  PEEPHOLE OPTIMIZATION
 * ════════════════════════════════════════════════════════════════════ */

typedef struct TOSPeepholePattern
{
  TOSInst *match;
  uint32_t matchLen;
  TOSInst *replace;
  uint32_t replaceLen;
  int costDelta;
  const char *name;
} TOSPeepholePattern;

int tos_add_peephole (TOSContext *ctx, const TOSPeepholePattern *pattern);
int tos_apply_peepholes (TOSContext *ctx, TOSProgram *prog);
int tos_load_peepholes_from_cache (TOSContext *ctx);
int tos_save_peepholes_to_cache (TOSContext *ctx);

/* ════════════════════════════════════════════════════════════════════
 *  §6  SUB-INSTRUCTION FUSION
 * ════════════════════════════════════════════════════════════════════ */

typedef struct TOSSubInstr
{
  uint32_t id;
  const char *name;
  TOSInst *component;
  uint32_t componentCount;
  uint64_t frequency;
  int speedup;
  uint8_t instruction;
  uint32_t encodingSize;
} TOSSubInstr;

int tos_register_subinstr (TOSContext *ctx, const TOSSubInstr *si);
int tos_apply_subinstr (TOSContext *ctx, TOSProgram *prog);
int tos_load_subinstr_cache (TOSContext *ctx);
int tos_save_subinstr_cache (TOSContext *ctx);

/* ════════════════════════════════════════════════════════════════════
 *  §7  MULTI-VERSION OPTIMIZATION
 * ════════════════════════════════════════════════════════════════════ */

typedef struct TOSVersionCandidate
{
  TOSVersionId id;
  TOSProgram *program;
  uint64_t measuredCycles;
} TOSVersionCandidate;

typedef struct TOSVersionResult
{
  TOSVersionId bestVersion;
  uint64_t bestCycles;
} TOSVersionResult;

int tos_generate_versions (TOSContext *ctx, const TOSProgram *input,
                           TOSVersionCandidate *out, size_t *outCount);
int tos_benchmark_versions (TOSContext *ctx, TOSVersionCandidate *versions,
                            size_t count);
int tos_select_best_version (TOSContext *ctx,
                             const TOSVersionCandidate *versions,
                             size_t count, TOSVersionResult *result);
int tos_cache_version (TOSContext *ctx, const void *key, size_t keySize,
                       TOSVersionId version);
int tos_lookup_cached_version (TOSContext *ctx, const void *key,
                               size_t keySize, TOSVersionId *outVersion);

/* ════════════════════════════════════════════════════════════════════
 *  §8  SYMBOL CACHE
 * ════════════════════════════════════════════════════════════════════ */

int tos_cache_symbol (TOSContext *ctx, const char *name, uint64_t address);
int tos_lookup_symbol (TOSContext *ctx, const char *name,
                       uint64_t *outAddress);

/* ════════════════════════════════════════════════════════════════════
 *  §9  TRACE JIT
 * ════════════════════════════════════════════════════════════════════ */

typedef struct TOSTrace TOSTrace;
typedef struct TOSTraceVersion TOSTraceVersion;

TOSTraceId tos_trace_begin (TOSContext *ctx);
int tos_trace_append (TOSContext *ctx, TOSTraceId id,
                      const struct WILGraph *fragment);
int tos_trace_end (TOSContext *ctx, TOSTraceId id);
int tos_trace_compile (TOSContext *ctx, TOSTraceId id);
int tos_trace_lookup (TOSContext *ctx, const void *pc, TOSTraceId *outTrace);

/* ════════════════════════════════════════════════════════════════════
 *  §10 OPTIMIZATION PIPELINE
 * ════════════════════════════════════════════════════════════════════ */

int tos_optimize_program (TOSContext *ctx, TOSProgram *prog);
int tos_optimize_module (TOSContext *ctx, struct MALModule *module);

/* ════════════════════════════════════════════════════════════════════
 *  §11 LMDB UTILITY WRAPPERS
 * ════════════════════════════════════════════════════════════════════ */

int tos_lmdb_put (TOSContext *ctx, MDB_dbi db, const void *key,
                  size_t keySize, const void *value, size_t valueSize);
int tos_lmdb_get (TOSContext *ctx, MDB_dbi db, const void *key,
                  size_t keySize, void **value, size_t *valueSize);
int tos_lmdb_del (TOSContext *ctx, MDB_dbi db, const void *key,
                  size_t keySize);

/* ════════════════════════════════════════════════════════════════════
 *  §12 PRINTING / DEBUGGING
 * ════════════════════════════════════════════════════════════════════ */

void tosPrintProgram (const TOSProgram *program);
int tosEmitText (const TOSProgram *program, const char *path);
void tos_dump_stats (const TOSContext *ctx);
void tos_reset_stats (TOSContext *ctx);

WIRBLE_END_DECLS

#endif /* WIRBLE_TOS_H */
