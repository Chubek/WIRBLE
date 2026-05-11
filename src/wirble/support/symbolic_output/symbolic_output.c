#include <wirble/wirble-symbolic_output.h>
#include <wirble/wirble-mal.h>

#include <stdio.h>
#include <string.h>

static int
is_x86_target (const MDSProgram *program)
{
  return program != NULL && program->machine != NULL
         && program->machine->targetKind == MDS_TARGET_X86_64;
}

static const char *
reg_prefix_for_target (const MDSProgram *program, WirbleAsmFlavor flavor,
                       WirbleX86Syntax x86Syntax)
{
  if (!is_x86_target (program))
    {
      return "";
    }
  if (flavor == WIRBLE_ASM_FLAVOR_GAS && x86Syntax == WIRBLE_X86_SYNTAX_ATT)
    {
      return "%";
    }
  return "";
}

static void
write_operand (FILE *out, const MDSProgram *program, const MALOperand *operand,
               WirbleAsmFlavor flavor, WirbleX86Syntax x86Syntax)
{
  const char *rp = reg_prefix_for_target (program, flavor, x86Syntax);

  switch (operand->kind)
    {
    case MAL_OPND_REG: fprintf (out, "%sr%u", rp, operand->reg); break;
    case MAL_OPND_IMM_INT:
      if (is_x86_target (program) && flavor == WIRBLE_ASM_FLAVOR_GAS
          && x86Syntax == WIRBLE_X86_SYNTAX_ATT)
        {
          fprintf (out, "$%lld", (long long) operand->imm_i);
        }
      else
        {
          fprintf (out, "%lld", (long long) operand->imm_i);
        }
      break;
    case MAL_OPND_IMM_FLOAT: fprintf (out, "%g", operand->imm_f); break;
    case MAL_OPND_BLOCK: fprintf (out, ".L%u", operand->block); break;
    case MAL_OPND_GLOBAL: fprintf (out, "%s", operand->globalName == NULL ? "?" : operand->globalName); break;
    case MAL_OPND_FUNC: fprintf (out, "%s", operand->funcName == NULL ? "?" : operand->funcName); break;
    case MAL_OPND_TYPE: fprintf (out, "type(%u)", (unsigned) operand->asType); break;
    case MAL_OPND_VECDESC:
      fprintf (out, "<%u x %u>", operand->vec.count,
               (unsigned) operand->vec.elementType);
      break;
    }
}

static void
write_inst (FILE *out, const MDSProgram *program, const MDSInst *inst,
            WirbleAsmFlavor flavor, WirbleX86Syntax x86Syntax)
{
  uint32_t i;

  if (inst->desc == NULL || inst->desc->mnemonic == NULL)
    {
      fprintf (out, "  .invalid\n");
      return;
    }
  fprintf (out, "  %s", inst->desc->mnemonic);
  if (inst->operandCount != 0u)
    {
      fputc (' ', out);
    }

  if (is_x86_target (program) && flavor == WIRBLE_ASM_FLAVOR_GAS
      && x86Syntax == WIRBLE_X86_SYNTAX_ATT)
    {
      for (i = inst->operandCount; i > 0u; --i)
        {
          if (i != inst->operandCount)
            {
              fprintf (out, ", ");
            }
          write_operand (out, program, &inst->operands[i - 1u], flavor,
                         x86Syntax);
        }
    }
  else
    {
      for (i = 0u; i < inst->operandCount; ++i)
        {
          if (i != 0u)
            {
              fprintf (out, ", ");
            }
          write_operand (out, program, &inst->operands[i], flavor, x86Syntax);
        }
    }
  fputc ('\n', out);
}

int
wirbleEmitSymbolicAssembly (const MDSProgram *program, const char *path,
                            WirbleAsmFlavor flavor, WirbleX86Syntax x86Syntax)
{
  FILE *out;
  uint32_t b;

  if (program == NULL || path == NULL)
    {
      return 0;
    }
  out = fopen (path, "w");
  if (out == NULL)
    {
      return 0;
    }

  if (flavor == WIRBLE_ASM_FLAVOR_GAS)
    {
      if (is_x86_target (program) && x86Syntax == WIRBLE_X86_SYNTAX_INTEL)
        {
          fprintf (out, ".intel_syntax noprefix\n");
        }
      fprintf (out, ".text\n");
    }
  else
    {
      fprintf (out, "section .text\n");
    }

  fprintf (out, ".global main\nmain:\n");
  for (b = 0u; b < program->blockCount; ++b)
    {
      uint32_t i;
      const MDSBasicBlock *block = &program->blocks[b];
      fprintf (out, ".L%u:\n", block->id);
      for (i = 0u; i < block->instrCount; ++i)
        {
          write_inst (out, program, &block->instructions[i], flavor, x86Syntax);
        }
    }

  fclose (out);
  return 1;
}
