#include "mal_ir.h"

#include <stdlib.h>
#include <string.h>

MALOperand
mal_operand_reg (MALReg reg, MALType type)
{
  MALOperand operand;
  operand.kind = MAL_OPND_REG;
  operand.type = type;
  operand.reg = reg;
  return operand;
}

MALOperand
mal_operand_imm_int (int64_t value, MALType type)
{
  MALOperand operand;
  operand.kind = MAL_OPND_IMM_INT;
  operand.type = type;
  operand.imm_i = value;
  return operand;
}

MALOperand
mal_operand_imm_float (double value, MALType type)
{
  MALOperand operand;
  operand.kind = MAL_OPND_IMM_FLOAT;
  operand.type = type;
  operand.imm_f = value;
  return operand;
}

MALOperand
mal_operand_block (MALBlockId block)
{
  MALOperand operand;
  operand.kind = MAL_OPND_BLOCK;
  operand.type = MAL_TYPE_VOID;
  operand.block = block;
  return operand;
}

int
mal_operand_equal (const MALOperand *lhs, const MALOperand *rhs)
{
  if (lhs == NULL || rhs == NULL || lhs->kind != rhs->kind || lhs->type != rhs->type)
    {
      return 0;
    }
  switch (lhs->kind)
    {
    case MAL_OPND_REG: return lhs->reg == rhs->reg;
    case MAL_OPND_IMM_INT: return lhs->imm_i == rhs->imm_i;
    case MAL_OPND_IMM_FLOAT: return lhs->imm_f == rhs->imm_f;
    case MAL_OPND_BLOCK: return lhs->block == rhs->block;
    case MAL_OPND_GLOBAL: return lhs->globalName != NULL && rhs->globalName != NULL && strcmp (lhs->globalName, rhs->globalName) == 0;
    case MAL_OPND_FUNC: return lhs->funcName != NULL && rhs->funcName != NULL && strcmp (lhs->funcName, rhs->funcName) == 0;
    case MAL_OPND_TYPE: return lhs->asType == rhs->asType;
    case MAL_OPND_VECDESC:
      return lhs->vec.elementType == rhs->vec.elementType && lhs->vec.count == rhs->vec.count;
    }
  return 0;
}

int
mal_operand_is_zero (const MALOperand *operand)
{
  return operand != NULL
         && ((operand->kind == MAL_OPND_IMM_INT && operand->imm_i == 0)
             || (operand->kind == MAL_OPND_IMM_FLOAT && operand->imm_f == 0.0));
}

int
mal_operand_is_one (const MALOperand *operand)
{
  return operand != NULL
         && ((operand->kind == MAL_OPND_IMM_INT && operand->imm_i == 1)
             || (operand->kind == MAL_OPND_IMM_FLOAT && operand->imm_f == 1.0));
}

void
mal_block_insert_end (MALBlock *b, MALInst *inst)
{
  if (b == NULL || inst == NULL)
    {
      return;
    }
  if (b->first == NULL)
    {
      b->first = inst;
      b->last = inst;
      return;
    }
  inst->prev = b->last;
  b->last->next = inst;
  b->last = inst;
}

void
mal_inst_remove (MALBlock *b, MALInst *inst)
{
  if (b == NULL || inst == NULL)
    {
      return;
    }
  if (inst->prev != NULL)
    {
      inst->prev->next = inst->next;
    }
  else
    {
      b->first = inst->next;
    }
  if (inst->next != NULL)
    {
      inst->next->prev = inst->prev;
    }
  else
    {
      b->last = inst->prev;
    }
  free (inst->operands);
  free ((char *) inst->comment);
  free (inst);
}

MALInst *
mal_emit_phi (MALBlock *b, MALType ty)
{
  MALInst *inst = mal_inst_create (MAL_OP_PHI, ty, MAL_INVALID_REG);
  if (inst != NULL)
    {
      mal_block_insert_end (b, inst);
    }
  return inst;
}

int
mal_inst_is_terminator (const MALInst *inst)
{
  return inst != NULL
         && (inst->opcode == MAL_OP_BR || inst->opcode == MAL_OP_CBR
             || inst->opcode == MAL_OP_SWITCH || inst->opcode == MAL_OP_RET
             || inst->opcode == MAL_OP_UNREACHABLE);
}
