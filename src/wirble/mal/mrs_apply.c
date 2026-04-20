#include "mal_ir.h"

#include <stdlib.h>

static void
mal_replace_inst (MALBlock *block, MALInst *oldInst, MALInst *newInst)
{
  if (block == NULL || oldInst == NULL || newInst == NULL)
    {
      return;
    }
  newInst->prev = oldInst->prev;
  newInst->next = oldInst->next;
  if (oldInst->prev != NULL)
    {
      oldInst->prev->next = newInst;
    }
  else
    {
      block->first = newInst;
    }
  if (oldInst->next != NULL)
    {
      oldInst->next->prev = newInst;
    }
  else
    {
      block->last = newInst;
    }
  free (oldInst->operands);
  free ((char *) oldInst->comment);
  free (oldInst);
}

int
malApplyOnePeephole (MALRewriteSystem *mrs, MALFunction *fn)
{
  MALBlock *block;
  uint32_t ruleIndex;

  if (mrs == NULL || fn == NULL)
    {
      return 0;
    }
  for (block = fn->blocks; block != NULL; block = block->next)
    {
      MALInst *inst = block->first;
      while (inst != NULL)
        {
          MALInst *next = inst->next;
          for (ruleIndex = 0u; ruleIndex < mrs->ruleCount; ++ruleIndex)
            {
              MALOperand bindings[8];
              const char *bindingNames[8];
              uint32_t bindingCount = 0u;
              MALInst *replacement;
              if (!mal_mrs_match_rule (&mrs->rules[ruleIndex], inst, bindings, 8u,
                                       bindingNames, &bindingCount))
                {
                  continue;
                }
              replacement = mal_mrs_build_replacement (&mrs->rules[ruleIndex], inst,
                                                       bindings, bindingNames,
                                                       bindingCount);
              if (replacement == NULL)
                {
                  continue;
                }
              mal_replace_inst (block, inst, replacement);
              return 1;
            }
          inst = next;
        }
    }
  return 0;
}

int
malApplyPeepholeToFixpoint (MALRewriteSystem *mrs, MALFunction *fn)
{
  int changed = 0;
  while (malApplyOnePeephole (mrs, fn))
    {
      changed = 1;
    }
  return changed;
}
