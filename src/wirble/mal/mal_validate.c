#include "mal_ir.h"

#include <stdlib.h>

static MALBlock *
mal_block_by_id (MALFunction *fn, MALBlockId id)
{
  MALBlock *block;
  for (block = fn == NULL ? NULL : fn->blocks; block != NULL; block = block->next)
    {
      if (block->id == id)
        {
          return block;
        }
    }
  return NULL;
}

static int
mal_append_block_id (MALBlockId **items, uint32_t *count, MALBlockId value)
{
  MALBlockId *grown;
  grown = (MALBlockId *) realloc (*items, (size_t) (*count + 1u) * sizeof (**items));
  if (grown == NULL)
    {
      return 0;
    }
  *items = grown;
  (*items)[(*count)++] = value;
  return 1;
}

static void
mal_block_add_edge (MALBlock *from, MALBlock *to)
{
  if (from == NULL || to == NULL)
    {
      return;
    }
  if (!mal_append_block_id (&from->succs, &from->succCount, to->id))
    {
      return;
    }
  (void) mal_append_block_id (&to->preds, &to->predCount, from->id);
}

void
mal_compute_cfg (MALFunction *fn)
{
  MALBlock *block;
  for (block = fn == NULL ? NULL : fn->blocks; block != NULL; block = block->next)
    {
      MALInst *term = block->last;
      free (block->preds);
      free (block->succs);
      block->preds = NULL;
      block->succs = NULL;
      block->predCount = 0u;
      block->succCount = 0u;
      if (term == NULL)
        {
          continue;
        }
      if (term->opcode == MAL_OP_BR && term->operandCount >= 1u
          && term->operands[0].kind == MAL_OPND_BLOCK)
        {
          mal_block_add_edge (block, mal_block_by_id (fn, term->operands[0].block));
        }
      else if (term->opcode == MAL_OP_CBR && term->operandCount >= 3u)
        {
          mal_block_add_edge (block, mal_block_by_id (fn, term->operands[1].block));
          mal_block_add_edge (block, mal_block_by_id (fn, term->operands[2].block));
        }
      else if (!mal_inst_is_terminator (term) && block->next != NULL)
        {
          mal_block_add_edge (block, block->next);
        }
    }
}

void
mal_compute_dominators (MALFunction *fn)
{
  MALBlock *block;
  MALBlock *prev = NULL;
  for (block = fn == NULL ? NULL : fn->blocks; block != NULL; block = block->next)
    {
      free (block->domChildren);
      block->domChildren = NULL;
      block->domChildCount = 0u;
      block->domParent = prev == NULL ? MAL_INVALID_BLOCK : prev->id;
      if (prev != NULL)
        {
          MALBlockId *grown = (MALBlockId *) realloc (
              prev->domChildren,
              (size_t) (prev->domChildCount + 1u) * sizeof (*prev->domChildren));
          if (grown != NULL)
            {
              prev->domChildren = grown;
              prev->domChildren[prev->domChildCount++] = block->id;
            }
        }
      prev = block;
    }
}

void
mal_compute_liveness (MALFunction *fn)
{
  MALBlock *block;
  uint32_t regCount;
  size_t bytes;

  if (fn == NULL)
    {
      return;
    }
  regCount = fn->nextSSAReg;
  bytes = regCount == 0u ? 0u : (size_t) regCount;
  for (block = fn->blocks; block != NULL; block = block->next)
    {
      free (block->liveIn);
      free (block->liveOut);
      block->liveIn = bytes == 0u ? NULL : (uint8_t *) calloc (bytes, 1u);
      block->liveOut = bytes == 0u ? NULL : (uint8_t *) calloc (bytes, 1u);
    }
}

int
mal_verify_ssa (MALFunction *fn)
{
  MALBlock *block;
  uint8_t *defined;
  MALReg reg;

  if (fn == NULL)
    {
      return 0;
    }
  defined = (uint8_t *) calloc ((size_t) (fn->nextSSAReg == 0u ? 1u : fn->nextSSAReg), 1u);
  if (defined == NULL)
    {
      return 0;
    }
  for (reg = 0u; reg < fn->paramCount && reg < fn->nextSSAReg; ++reg)
    {
      defined[reg] = 1u;
    }
  for (block = fn->blocks; block != NULL; block = block->next)
    {
      MALInst *inst;
      for (inst = block->first; inst != NULL; inst = inst->next)
        {
          uint32_t i;
          for (i = 0u; i < inst->operandCount; ++i)
            {
              if (inst->operands[i].kind == MAL_OPND_REG)
                {
                  if (inst->operands[i].reg >= fn->nextSSAReg
                      || !defined[inst->operands[i].reg])
                    {
                      free (defined);
                      return 0;
                    }
                }
            }
          if (inst->dst != MAL_INVALID_REG)
            {
              if (inst->dst >= fn->nextSSAReg || defined[inst->dst])
                {
                  free (defined);
                  return 0;
                }
              defined[inst->dst] = 1u;
            }
        }
    }
  free (defined);
  return 1;
}

void mal_dce (MALFunction *fn)
{ (void) fn; }
void mal_const_fold (MALFunction *fn)
{ (void) fn; }
void mal_copy_prop (MALFunction *fn)
{ (void) fn; }
void mal_local_cse (MALFunction *fn)
{ (void) fn; }
void malScheduleLocal (MALFunction *fn)
{ (void) fn; }
void malScheduleGlobal (MALFunction *fn)
{ (void) fn; }
void mal_generate_cfg_dot (const MALFunction *fn, const char *path)
{ (void) fn; (void) path; }
void mal_generate_dom_dot (const MALFunction *fn, const char *path)
{ (void) fn; (void) path; }
