#include "mal_ir.h"

#include <stdio.h>

const char *
mal_type_name (MALType type)
{
  switch (type)
    {
    case MAL_TYPE_VOID: return "void";
    case MAL_TYPE_I1: return "i1";
    case MAL_TYPE_I8: return "i8";
    case MAL_TYPE_I16: return "i16";
    case MAL_TYPE_I32: return "i32";
    case MAL_TYPE_I64: return "i64";
    case MAL_TYPE_F32: return "f32";
    case MAL_TYPE_F64: return "f64";
    case MAL_TYPE_PTR: return "ptr";
    case MAL_TYPE_VEC: return "vec";
    default: return "?";
    }
}

const char *
mal_opcode_name (MALOpcode opcode)
{
  switch (opcode)
    {
    case MAL_OP_NOP: return "nop";
    case MAL_OP_COPY: return "copy";
    case MAL_OP_LOAD: return "load";
    case MAL_OP_STORE: return "store";
    case MAL_OP_LOAD_IMM: return "load_imm";
    case MAL_OP_ADD: return "add";
    case MAL_OP_SUB: return "sub";
    case MAL_OP_MUL: return "mul";
    case MAL_OP_SDIV: return "sdiv";
    case MAL_OP_SREM: return "srem";
    case MAL_OP_NEG: return "neg";
    case MAL_OP_AND: return "and";
    case MAL_OP_OR: return "or";
    case MAL_OP_XOR: return "xor";
    case MAL_OP_NOT: return "not";
    case MAL_OP_SHL: return "shl";
    case MAL_OP_ASHR: return "ashr";
    case MAL_OP_FADD: return "fadd";
    case MAL_OP_FSUB: return "fsub";
    case MAL_OP_FMUL: return "fmul";
    case MAL_OP_FDIV: return "fdiv";
    case MAL_OP_FNEG: return "fneg";
    case MAL_OP_FABS: return "fabs";
    case MAL_OP_ICMP_EQ: return "icmp_eq";
    case MAL_OP_ICMP_NE: return "icmp_ne";
    case MAL_OP_ICMP_SLT: return "icmp_slt";
    case MAL_OP_ICMP_SLE: return "icmp_sle";
    case MAL_OP_ICMP_SGT: return "icmp_sgt";
    case MAL_OP_ICMP_SGE: return "icmp_sge";
    case MAL_OP_BR: return "br";
    case MAL_OP_CBR: return "cbr";
    case MAL_OP_RET: return "ret";
    case MAL_OP_PHI: return "phi";
    case MAL_OP_SELECT: return "select";
    case MAL_OP_SEXT: return "sext";
    case MAL_OP_BITCAST: return "bitcast";
    case MAL_OP_TARGET: return "target";
    default: return "op";
    }
}

void
mal_print_operand (FILE *out, const MALOperand *operand)
{
  if (out == NULL || operand == NULL)
    {
      return;
    }
  switch (operand->kind)
    {
    case MAL_OPND_REG:
      fprintf (out, "r%u", operand->reg);
      break;
    case MAL_OPND_IMM_INT:
      fprintf (out, "%lld", (long long) operand->imm_i);
      break;
    case MAL_OPND_IMM_FLOAT:
      fprintf (out, "%g", operand->imm_f);
      break;
    case MAL_OPND_BLOCK:
      fprintf (out, "b%u", operand->block);
      break;
    case MAL_OPND_GLOBAL:
      fprintf (out, "@%s", operand->globalName);
      break;
    case MAL_OPND_FUNC:
      fprintf (out, "$%s", operand->funcName);
      break;
    case MAL_OPND_TYPE:
      fprintf (out, "%s", mal_type_name (operand->asType));
      break;
    case MAL_OPND_VECDESC:
      fprintf (out, "<%u x %s>", operand->vec.count,
               mal_type_name (operand->vec.elementType));
      break;
    }
}

void
mal_print_inst (FILE *out, const MALInst *inst)
{
  uint32_t i;
  if (out == NULL || inst == NULL)
    {
      return;
    }
  fprintf (out, "  i%u ", inst->id == MAL_INVALID_INST ? 0u : inst->id);
  if (inst->dst != MAL_INVALID_REG)
    {
      fprintf (out, "r%u = ", inst->dst);
    }
  fprintf (out, "%s.%s", mal_opcode_name (inst->opcode), mal_type_name (inst->type));
  if (inst->operandCount != 0u)
    {
      fprintf (out, " ");
    }
  for (i = 0u; i < inst->operandCount; ++i)
    {
      if (i != 0u)
        {
          fprintf (out, ", ");
        }
      mal_print_operand (out, &inst->operands[i]);
    }
  fputc ('\n', out);
}

void
mal_print_function_to_file (FILE *out, const MALFunction *fn)
{
  MALBlock *block;
  if (out == NULL || fn == NULL)
    {
      return;
    }
  fprintf (out, "function %s -> %s\n", fn->name, mal_type_name (fn->retType));
  for (block = fn->blocks; block != NULL; block = block->next)
    {
      MALInst *inst;
      fprintf (out, "b%u (%s):\n", block->id,
               block->name == NULL ? "block" : block->name);
      for (inst = block->first; inst != NULL; inst = inst->next)
        {
          mal_print_inst (out, inst);
        }
    }
}

void
mal_print_module_to_file (FILE *out, const MALModule *m)
{
  MALFunction *fn;
  if (out == NULL || m == NULL)
    {
      return;
    }
  fprintf (out, "module %s\n", m->targetTriple == NULL ? "unknown" : m->targetTriple);
  for (fn = m->functions; fn != NULL; fn = fn->next)
    {
      mal_print_function_to_file (out, fn);
    }
}

void
mal_dump_function (const MALFunction *fn)
{
  mal_print_function_to_file (stdout, fn);
}

void
mal_dump_module (const MALModule *m)
{
  mal_print_module_to_file (stdout, m);
}
