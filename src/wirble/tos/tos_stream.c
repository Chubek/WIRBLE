#include "tos_stream.h"

#include <stdlib.h>
#include <string.h>

static int
append_instruction (TOSProgram *program, const MDSInst *inst, uint32_t blockId,
                    uint32_t order)
{
  TOSInst *grown;
  TOSInst *slot;

  grown = (TOSInst *) realloc (program->instructions,
                               (size_t) (program->instrCount + 1u)
                                   * sizeof (*program->instructions));
  if (grown == NULL)
    {
      return 0;
    }
  program->instructions = grown;
  slot = &program->instructions[program->instrCount];
  memset (slot, 0, sizeof (*slot));
  slot->id = program->instrCount;
  slot->blockId = blockId;
  slot->order = order;
  slot->desc = inst->desc;
  slot->operandCount = inst->operandCount;
  if (!tos_clone_operands (&slot->operands, inst->operands, inst->operandCount))
    {
      return 0;
    }
  ++program->instrCount;
  return 1;
}

static int
copy_block_metadata (TOSBlock *dst, const MDSBasicBlock *src, uint32_t layoutIndex)
{
  memset (dst, 0, sizeof (*dst));
  dst->id = src->id;
  dst->layoutIndex = layoutIndex;
  dst->firstInstr = TOS_INVALID_INSTR;
  dst->instrCount = 0u;
  if (src->successorCount != 0u)
    {
      dst->successors = (uint32_t *) calloc (src->successorCount,
                                             sizeof (*dst->successors));
      if (dst->successors == NULL)
        {
          return 0;
        }
      memcpy (dst->successors, src->successors,
              (size_t) src->successorCount * sizeof (*dst->successors));
      dst->successorCount = src->successorCount;
    }
  return 1;
}

static int
emit_block_instructions (TOSProgram *program, const MDSBasicBlock *block,
                         uint32_t blockIndex)
{
  uint32_t instIndex;
  TOSBlock *tosBlock = &program->blocks[blockIndex];

  if (block->instrCount == 0u)
    {
      tosBlock->firstInstr = TOS_INVALID_INSTR;
      tosBlock->instrCount = 0u;
      return 1;
    }
  tosBlock->firstInstr = program->instrCount;
  tosBlock->instrCount = block->instrCount;
  for (instIndex = 0u; instIndex < block->instrCount; ++instIndex)
    {
      if (!append_instruction (program, &block->instructions[instIndex], block->id,
                               instIndex))
        {
          return 0;
        }
    }
  return 1;
}

int
tos_clone_operands (MALOperand **outOperands, const MALOperand *operands,
                    uint32_t operandCount)
{
  MALOperand *copy;

  if (outOperands == NULL)
    {
      return 0;
    }
  *outOperands = NULL;
  if (operandCount == 0u)
    {
      return 1;
    }
  copy = (MALOperand *) calloc (operandCount, sizeof (*copy));
  if (copy == NULL)
    {
      return 0;
    }
  memcpy (copy, operands, (size_t) operandCount * sizeof (*copy));
  *outOperands = copy;
  return 1;
}

uint32_t
tos_find_block_index (const TOSProgram *program, uint32_t blockId)
{
  uint32_t index;

  if (program == NULL)
    {
      return UINT32_MAX;
    }
  for (index = 0u; index < program->blockCount; ++index)
    {
      if (program->blocks[index].id == blockId)
        {
          return index;
        }
    }
  return UINT32_MAX;
}

TOSProgram *
tosBuildFromMDS (const MDSProgram *program)
{
  TOSProgram *tos;
  uint32_t blockIndex;

  if (program == NULL)
    {
      return NULL;
    }
  tos = (TOSProgram *) calloc (1u, sizeof (*tos));
  if (tos == NULL)
    {
      return NULL;
    }
  tos->machine = program->machine;
  tos->blockCount = program->blockCount;
  tos->orderKind = TOS_ORDER_INPUT;
  if (program->blockCount != 0u)
    {
      tos->blocks = (TOSBlock *) calloc (program->blockCount, sizeof (*tos->blocks));
      if (tos->blocks == NULL)
        {
          free (tos);
          return NULL;
        }
    }
  for (blockIndex = 0u; blockIndex < program->blockCount; ++blockIndex)
    {
      if (!copy_block_metadata (&tos->blocks[blockIndex], &program->blocks[blockIndex],
                                blockIndex)
          || !emit_block_instructions (tos, &program->blocks[blockIndex], blockIndex))
        {
          tosDestroyProgram (tos);
          return NULL;
        }
    }
  tos->isFinalized = 0;
  return tos;
}

void
tosDestroyProgram (TOSProgram *program)
{
  uint32_t blockIndex;
  uint32_t instIndex;

  if (program == NULL)
    {
      return;
    }
  for (instIndex = 0u; instIndex < program->instrCount; ++instIndex)
    {
      free (program->instructions[instIndex].operands);
    }
  for (blockIndex = 0u; blockIndex < program->blockCount; ++blockIndex)
    {
      free (program->blocks[blockIndex].successors);
    }
  free (program->instructions);
  free (program->blocks);
  free (program);
}

TOSProgram *
tos_clone_program (const TOSProgram *program)
{
  TOSProgram *copy;
  uint32_t blockIndex;
  uint32_t instIndex;

  if (program == NULL)
    {
      return NULL;
    }
  copy = (TOSProgram *) calloc (1u, sizeof (*copy));
  if (copy == NULL)
    {
      return NULL;
    }
  *copy = *program;
  copy->blocks = NULL;
  copy->instructions = NULL;
  if (program->blockCount != 0u)
    {
      copy->blocks = (TOSBlock *) calloc (program->blockCount, sizeof (*copy->blocks));
      if (copy->blocks == NULL)
        {
          free (copy);
          return NULL;
        }
      for (blockIndex = 0u; blockIndex < program->blockCount; ++blockIndex)
        {
          copy->blocks[blockIndex] = program->blocks[blockIndex];
          copy->blocks[blockIndex].successors = NULL;
          if (program->blocks[blockIndex].successorCount != 0u)
            {
              copy->blocks[blockIndex].successors =
                  (uint32_t *) calloc (program->blocks[blockIndex].successorCount,
                                       sizeof (*copy->blocks[blockIndex].successors));
              if (copy->blocks[blockIndex].successors == NULL)
                {
                  tosDestroyProgram (copy);
                  return NULL;
                }
              memcpy (copy->blocks[blockIndex].successors,
                      program->blocks[blockIndex].successors,
                      (size_t) program->blocks[blockIndex].successorCount
                          * sizeof (*copy->blocks[blockIndex].successors));
            }
        }
    }
  if (program->instrCount != 0u)
    {
      copy->instructions =
          (TOSInst *) calloc (program->instrCount, sizeof (*copy->instructions));
      if (copy->instructions == NULL)
        {
          tosDestroyProgram (copy);
          return NULL;
        }
      for (instIndex = 0u; instIndex < program->instrCount; ++instIndex)
        {
          copy->instructions[instIndex] = program->instructions[instIndex];
          copy->instructions[instIndex].operands = NULL;
          if (!tos_clone_operands (&copy->instructions[instIndex].operands,
                                   program->instructions[instIndex].operands,
                                   program->instructions[instIndex].operandCount))
            {
              tosDestroyProgram (copy);
              return NULL;
            }
        }
    }
  return copy;
}
