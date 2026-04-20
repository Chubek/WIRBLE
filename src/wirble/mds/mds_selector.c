#include <wirble/wirble-mal.h>
#include <wirble/wirble-mds.h>

#include <stdlib.h>
#include <string.h>

static MALOperand
mds_copy_operand (const MALOperand *operand)
{
  MALOperand copy;
  memset (&copy, 0, sizeof (copy));
  if (operand != NULL)
    {
      copy = *operand;
    }
  return copy;
}

static int
mds_append_mds_operand (MALOperand **items, uint32_t *count, MALOperand operand)
{
  MALOperand *grown;

  grown = (MALOperand *) realloc (*items, (size_t) (*count + 1u) * sizeof (**items));
  if (grown == NULL)
    {
      return 0;
    }
  *items = grown;
  (*items)[(*count)++] = operand;
  return 1;
}

static int
mds_append_instruction (MDSBasicBlock *block, MDSInst inst)
{
  MDSInst *grown;

  grown = (MDSInst *) realloc (block->instructions,
                               (size_t) (block->instrCount + 1u)
                                   * sizeof (*block->instructions));
  if (grown == NULL)
    {
      return 0;
    }
  block->instructions = grown;
  block->instructions[block->instrCount++] = inst;
  return 1;
}

static const MDSInstruction *
mds_desc_from_pattern (const MDSInstrSelector *selector, const MDSPattern *pattern)
{
  MDSInstrId id;

  if (selector == NULL || selector->machine == NULL || pattern == NULL
      || pattern->emitInstrCount == 0u || selector->machine->instrById == NULL)
    {
      return NULL;
    }
  id = pattern->emitInstrs[0];
  if (id >= selector->machine->instructionCount)
    {
      return NULL;
    }
  return selector->machine->instrById[id];
}

static int
mds_build_selected_inst (const MDSInstrSelector *selector, const MALInst *inst,
                         MDSInst *out)
{
  const MDSPattern *pattern;
  const MDSInstruction *desc;
  uint32_t index;

  if (selector == NULL || inst == NULL || out == NULL)
    {
      return 0;
    }
  pattern = mdsSelectBestPattern (selector, inst);
  desc = mds_desc_from_pattern (selector, pattern);
  if (desc == NULL)
    {
      return 0;
    }
  memset (out, 0, sizeof (*out));
  out->desc = desc;
  if (inst->dst != MAL_INVALID_REG && desc->operndCount != 0u
      && desc->operands[0].isOutput)
    {
      MALOperand dst = { .kind = MAL_OPND_REG, .type = inst->type, .reg = inst->dst };
      if (!mds_append_mds_operand (&out->operands, &out->operandCount, dst))
        {
          return 0;
        }
    }
  for (index = 0u; index < inst->operandCount; ++index)
    {
      if (!mds_append_mds_operand (&out->operands, &out->operandCount,
                                   mds_copy_operand (&inst->operands[index])))
        {
          free (out->operands);
          out->operands = NULL;
          out->operandCount = 0u;
          return 0;
        }
    }
  return 1;
}

static void
mds_copy_successors (const MALBlock *src, MDSBasicBlock *dst)
{
  if (src == NULL || dst == NULL || src->succCount == 0u)
    {
      return;
    }
  dst->successors = (uint32_t *) calloc (src->succCount, sizeof (*dst->successors));
  if (dst->successors == NULL)
    {
      return;
    }
  memcpy (dst->successors, src->succs,
          (size_t) src->succCount * sizeof (*dst->successors));
  dst->successorCount = src->succCount;
}

MDSProgram *
mdsLowerFromMAL (const MALModule *module, const MDSInstrSelector *selector,
                 const MDSMachine *machine)
{
  const MDSMachine *resolvedMachine = machine;
  MDSInstrSelector *ownedSelector = NULL;
  MDSProgram *program;
  const MALFunction *fn;
  uint32_t blockCount = 0u;
  uint32_t blockIndex = 0u;

  if (module == NULL)
    {
      return NULL;
    }
  if (resolvedMachine == NULL)
    {
      resolvedMachine = selector != NULL ? selector->machine : module->mdsMachine;
    }
  if (resolvedMachine == NULL)
    {
      return NULL;
    }
  if (selector == NULL)
    {
      ownedSelector = mdsCreateSelector (resolvedMachine);
      selector = ownedSelector;
    }
  if (selector == NULL)
    {
      return NULL;
    }

  program = (MDSProgram *) calloc (1u, sizeof (*program));
  if (program == NULL)
    {
      mdsDestroySelector (ownedSelector);
      return NULL;
    }
  for (fn = module->functions; fn != NULL; fn = fn->next)
    {
      const MALBlock *block;
      for (block = fn->blocks; block != NULL; block = block->next)
        {
          ++blockCount;
        }
    }
  program->machine = resolvedMachine;
  program->blockCount = blockCount;
  program->blocks = blockCount == 0u
                        ? NULL
                        : (MDSBasicBlock *) calloc (blockCount,
                                                    sizeof (*program->blocks));
  if (blockCount != 0u && program->blocks == NULL)
    {
      free (program);
      mdsDestroySelector (ownedSelector);
      return NULL;
    }

  for (fn = module->functions; fn != NULL; fn = fn->next)
    {
      const MALBlock *block;
      for (block = fn->blocks; block != NULL; block = block->next)
        {
          const MALInst *inst;
          MDSBasicBlock *mdsBlock = &program->blocks[blockIndex];
          mdsBlock->id = block->id;
          mds_copy_successors (block, mdsBlock);
          for (inst = block->first; inst != NULL; inst = inst->next)
            {
              MDSInst lowered;
              if (!mds_build_selected_inst (selector, inst, &lowered)
                  || !mds_append_instruction (mdsBlock, lowered))
                {
                  mdsDestroyProgram (program);
                  mdsDestroySelector (ownedSelector);
                  return NULL;
                }
            }
          ++blockIndex;
        }
    }

  mdsDestroySelector (ownedSelector);
  return program;
}
