#include "tos_stream.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void
visit_block (const TOSProgram *program, uint32_t blockIndex, uint8_t *seen,
             uint32_t *postorder, uint32_t *count)
{
  uint32_t succIndex;
  const TOSBlock *block;

  if (program == NULL || seen == NULL || postorder == NULL || count == NULL
      || blockIndex >= program->blockCount || seen[blockIndex] != 0u)
    {
      return;
    }
  seen[blockIndex] = 1u;
  block = &program->blocks[blockIndex];
  for (succIndex = 0u; succIndex < block->successorCount; ++succIndex)
    {
      uint32_t nextIndex = tos_find_block_index (program, block->successors[succIndex]);
      if (nextIndex != UINT32_MAX)
        {
          visit_block (program, nextIndex, seen, postorder, count);
        }
    }
  postorder[(*count)++] = blockIndex;
}

static int
compute_layout_order (const TOSProgram *program, uint32_t *order)
{
  uint8_t *seen;
  uint32_t *postorder;
  uint32_t postCount = 0u;
  uint32_t index;
  uint32_t cursor = 0u;

  if (program == NULL)
    {
      return 0;
    }
  if (program->blockCount == 0u)
    {
      return 1;
    }
  if (order == NULL)
    {
      return 0;
    }
  seen = program->blockCount == 0u ? NULL
                                   : (uint8_t *) calloc (program->blockCount,
                                                         sizeof (*seen));
  postorder = program->blockCount == 0u
                  ? NULL
                  : (uint32_t *) calloc (program->blockCount, sizeof (*postorder));
  if (program->blockCount != 0u && (seen == NULL || postorder == NULL))
    {
      free (seen);
      free (postorder);
      return 0;
    }

  if (program->blockCount != 0u)
    {
      visit_block (program, 0u, seen, postorder, &postCount);
      for (index = 0u; index < program->blockCount; ++index)
        {
          if (seen[index] == 0u)
            {
              visit_block (program, index, seen, postorder, &postCount);
            }
        }
      while (postCount != 0u)
        {
          order[cursor++] = postorder[--postCount];
        }
    }

  free (seen);
  free (postorder);
  return 1;
}

static int
rebuild_instruction_stream (TOSProgram *program, const TOSBlock *sourceBlocks,
                            TOSBlock *targetBlocks, const uint32_t *order)
{
  TOSInst *rebuilt;
  uint32_t instCount = 0u;
  uint32_t layoutIndex;

  rebuilt = program->instrCount == 0u
                ? NULL
                : (TOSInst *) calloc (program->instrCount, sizeof (*rebuilt));
  if (program->instrCount != 0u && rebuilt == NULL)
    {
      return 0;
    }

  for (layoutIndex = 0u; layoutIndex < program->blockCount; ++layoutIndex)
    {
      TOSBlock sourceBlock = sourceBlocks[order[layoutIndex]];
      TOSBlock *targetBlock = &targetBlocks[layoutIndex];
      uint32_t sourceFirst = sourceBlock.firstInstr;
      uint32_t instIndex;

      *targetBlock = sourceBlock;
      targetBlock->layoutIndex = layoutIndex;
      targetBlock->firstInstr = sourceBlock.instrCount == 0u ? TOS_INVALID_INSTR : instCount;
      for (instIndex = 0u; instIndex < sourceBlock.instrCount; ++instIndex)
        {
          TOSInst sourceInst = program->instructions[sourceFirst + instIndex];
          TOSInst *targetInst = &rebuilt[instCount];
          *targetInst = sourceInst;
          targetInst->id = instCount;
          targetInst->order = instIndex;
          ++instCount;
        }
    }

  free (program->instructions);
  program->instructions = rebuilt;
  program->instrCount = instCount;
  return 1;
}

int
tosFinalizeProgram (TOSProgram *program)
{
  uint32_t *order;
  TOSBlock *originalBlocks;

  if (program == NULL)
    {
      return 0;
    }
  if (program->isFinalized && program->orderKind == TOS_ORDER_REVERSE_POSTORDER)
    {
      return tosValidateProgram (program);
    }
  if (program->blockCount == 0u)
    {
      program->orderKind = TOS_ORDER_REVERSE_POSTORDER;
      program->isFinalized = 1;
      return 1;
    }
  order = (uint32_t *) calloc (program->blockCount, sizeof (*order));
  if (order == NULL)
    {
      return 0;
    }
  if (!compute_layout_order (program, order))
    {
      free (order);
      return 0;
    }
  originalBlocks = program->blocks;
  if (program->blockCount != 0u)
    {
      TOSBlock *reordered = (TOSBlock *) calloc (program->blockCount, sizeof (*reordered));
      if (reordered == NULL)
        {
          free (order);
          return 0;
        }
      memcpy (reordered, originalBlocks,
              (size_t) program->blockCount * sizeof (*reordered));
      if (!rebuild_instruction_stream (program, originalBlocks, reordered, order))
        {
          free (reordered);
          free (order);
          return 0;
        }
      program->blocks = reordered;
      free (originalBlocks);
    }
  program->orderKind = TOS_ORDER_REVERSE_POSTORDER;
  program->isFinalized = 1;
  free (order);
  return tosValidateProgram (program);
}

int
tosValidateProgram (const TOSProgram *program)
{
  uint32_t blockIndex;
  uint32_t instCursor = 0u;

  if (program == NULL)
    {
      return 0;
    }
  for (blockIndex = 0u; blockIndex < program->blockCount; ++blockIndex)
    {
      const TOSBlock *block = &program->blocks[blockIndex];
      uint32_t instIndex;
      if (block->layoutIndex != blockIndex)
        {
          return 0;
        }
      if (block->instrCount == 0u)
        {
          if (block->firstInstr != TOS_INVALID_INSTR)
            {
              return 0;
            }
          continue;
        }
      if (block->firstInstr != instCursor)
        {
          return 0;
        }
      for (instIndex = 0u; instIndex < block->instrCount; ++instIndex)
        {
          const TOSInst *inst = &program->instructions[instCursor + instIndex];
          if (inst->id != instCursor + instIndex || inst->blockId != block->id
              || inst->order != instIndex || inst->desc == NULL)
            {
              return 0;
            }
        }
      instCursor += block->instrCount;
    }
  return instCursor == program->instrCount;
}

TOSContext *
tos_create (const TOSConfig *cfg, const struct MDSMachine *machine)
{
  TOSContext *ctx = (TOSContext *) calloc (1u, sizeof (*ctx));

  if (ctx == NULL)
    {
      return NULL;
    }
  ctx->machine = machine;
  if (cfg != NULL)
    {
      (void) cfg;
    }
  return ctx;
}

void
tos_destroy (TOSContext *ctx)
{
  free (ctx);
}

int
tos_add_peephole (TOSContext *ctx, const TOSPeepholePattern *pattern)
{
  return ctx != NULL && pattern != NULL;
}

int
tos_apply_peepholes (TOSContext *ctx, TOSProgram *prog)
{
  if (ctx == NULL || prog == NULL)
    {
      return 0;
    }
  ++ctx->peepholesApplied;
  return tosFinalizeProgram (prog);
}

int
tos_load_peepholes_from_cache (TOSContext *ctx)
{
  return ctx != NULL;
}

int
tos_save_peepholes_to_cache (TOSContext *ctx)
{
  return ctx != NULL;
}

int
tos_register_subinstr (TOSContext *ctx, const TOSSubInstr *si)
{
  return ctx != NULL && si != NULL;
}

int
tos_apply_subinstr (TOSContext *ctx, TOSProgram *prog)
{
  if (ctx == NULL || prog == NULL)
    {
      return 0;
    }
  ++ctx->subInstrCreated;
  return tosFinalizeProgram (prog);
}

int
tos_load_subinstr_cache (TOSContext *ctx)
{
  return ctx != NULL;
}

int
tos_save_subinstr_cache (TOSContext *ctx)
{
  return ctx != NULL;
}

int
tos_generate_versions (TOSContext *ctx, const TOSProgram *input,
                       TOSVersionCandidate *out, size_t *outCount)
{
  if (ctx == NULL || input == NULL || outCount == NULL)
    {
      return 0;
    }
  if (out != NULL && *outCount != 0u)
    {
      out[0].id = 0u;
      out[0].program = tos_clone_program (input);
      if (out[0].program == NULL)
        {
          return 0;
        }
      out[0].measuredCycles = input->instrCount;
      *outCount = 1u;
      ++ctx->versionsGenerated;
      return 1;
    }
  *outCount = 1u;
  return 1;
}

int
tos_benchmark_versions (TOSContext *ctx, TOSVersionCandidate *versions, size_t count)
{
  size_t index;

  if (ctx == NULL || versions == NULL)
    {
      return 0;
    }
  for (index = 0u; index < count; ++index)
    {
      versions[index].measuredCycles = versions[index].program == NULL
                                           ? UINT64_MAX
                                           : versions[index].program->instrCount;
    }
  return 1;
}

int
tos_select_best_version (TOSContext *ctx, const TOSVersionCandidate *versions,
                          size_t count, TOSVersionResult *result)
{
  size_t index;
  uint64_t bestCycles;
  TOSVersionId bestId;

  if (ctx == NULL || versions == NULL || count == 0u || result == NULL)
    {
      return 0;
    }
  bestCycles = versions[0].measuredCycles;
  bestId = versions[0].id;
  for (index = 1u; index < count; ++index)
    {
      if (versions[index].measuredCycles < bestCycles)
        {
          bestCycles = versions[index].measuredCycles;
          bestId = versions[index].id;
        }
    }
  result->bestVersion = bestId;
  result->bestCycles = bestCycles;
  return 1;
}

int
tos_cache_version (TOSContext *ctx, const void *key, size_t keySize,
                    TOSVersionId version)
{
  (void) version;
  return ctx != NULL && key != NULL && keySize != 0u;
}

int
tos_lookup_cached_version (TOSContext *ctx, const void *key, size_t keySize,
                            TOSVersionId *outVersion)
{
  if (ctx == NULL || key == NULL || keySize == 0u || outVersion == NULL)
    {
      return 0;
    }
  *outVersion = TOS_INVALID_VERSION;
  return 1;
}

int
tos_cache_symbol (TOSContext *ctx, const char *name, uint64_t address)
{
  (void) address;
  return ctx != NULL && name != NULL;
}

int
tos_lookup_symbol (TOSContext *ctx, const char *name, uint64_t *outAddress)
{
  if (ctx == NULL || name == NULL || outAddress == NULL)
    {
      return 0;
    }
  *outAddress = 0u;
  return 1;
}

TOSTraceId
tos_trace_begin (TOSContext *ctx)
{
  return ctx == NULL ? TOS_INVALID_TRACE : 0u;
}

int
tos_trace_append (TOSContext *ctx, TOSTraceId id, const struct WILGraph *fragment)
{
  return ctx != NULL && id != TOS_INVALID_TRACE && fragment != NULL;
}

int
tos_trace_end (TOSContext *ctx, TOSTraceId id)
{
  return ctx != NULL && id != TOS_INVALID_TRACE;
}

int
tos_trace_compile (TOSContext *ctx, TOSTraceId id)
{
  if (ctx == NULL || id == TOS_INVALID_TRACE)
    {
      return 0;
    }
  ++ctx->tracesCompiled;
  return 1;
}

int
tos_trace_lookup (TOSContext *ctx, const void *pc, TOSTraceId *outTrace)
{
  if (ctx == NULL || pc == NULL || outTrace == NULL)
    {
      return 0;
    }
  *outTrace = TOS_INVALID_TRACE;
  return 1;
}

int
tos_optimize_program (TOSContext *ctx, TOSProgram *prog)
{
  if (ctx == NULL || prog == NULL)
    {
      return 0;
    }
  if (!tosFinalizeProgram (prog))
    {
      return 0;
    }
  return tos_apply_subinstr (ctx, prog);
}

int
tos_optimize_module (TOSContext *ctx, struct MALModule *module)
{
  MDSInstrSelector *selector = NULL;
  MDSProgram *mdsProgram;
  TOSProgram *tosProgram;
  int ok;

  if (ctx == NULL || module == NULL)
    {
      return 0;
    }
  if (ctx->machine == NULL)
    {
      return 0;
    }
  selector = mdsCreateSelector (ctx->machine);
  if (selector == NULL)
    {
      return 0;
    }
  mdsProgram = malLowerToMDS (module, selector);
  if (mdsProgram == NULL)
    {
      mdsDestroySelector (selector);
      return 0;
    }
  tosProgram = tosBuildFromMDS (mdsProgram);
  ok = tosProgram != NULL && tos_optimize_program (ctx, tosProgram);
  tosDestroyProgram (tosProgram);
  mdsDestroyProgram (mdsProgram);
  mdsDestroySelector (selector);
  return ok;
}

int
tos_lmdb_put (TOSContext *ctx, MDB_dbi db, const void *key, size_t keySize,
               const void *value, size_t valueSize)
{
  (void) db;
  return ctx != NULL && key != NULL && keySize != 0u && value != NULL
         && valueSize != 0u;
}

int
tos_lmdb_get (TOSContext *ctx, MDB_dbi db, const void *key, size_t keySize,
               void **value, size_t *valueSize)
{
  (void) db;
  if (ctx == NULL || key == NULL || keySize == 0u || value == NULL
      || valueSize == NULL)
    {
      return 0;
    }
  *value = NULL;
  *valueSize = 0u;
  return 1;
}

int
tos_lmdb_del (TOSContext *ctx, MDB_dbi db, const void *key, size_t keySize)
{
  (void) db;
  return ctx != NULL && key != NULL && keySize != 0u;
}

void
tos_dump_stats (const TOSContext *ctx)
{
  if (ctx == NULL)
    {
      return;
    }
  printf ("tos stats: peepholes=%llu subinstr=%llu traces=%llu versions=%llu\n",
          (unsigned long long) ctx->peepholesApplied,
          (unsigned long long) ctx->subInstrCreated,
          (unsigned long long) ctx->tracesCompiled,
          (unsigned long long) ctx->versionsGenerated);
}

void
tos_reset_stats (TOSContext *ctx)
{
  if (ctx == NULL)
    {
      return;
    }
  ctx->peepholesApplied = 0u;
  ctx->subInstrCreated = 0u;
  ctx->tracesCompiled = 0u;
  ctx->versionsGenerated = 0u;
}
