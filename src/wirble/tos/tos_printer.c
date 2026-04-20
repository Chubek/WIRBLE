#include "tos_stream.h"

#include <stdio.h>

static void
print_operand (FILE *out, const MALOperand *operand)
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
      fprintf (out, "@%s", operand->globalName == NULL ? "?" : operand->globalName);
      break;
    case MAL_OPND_FUNC:
      fprintf (out, "$%s", operand->funcName == NULL ? "?" : operand->funcName);
      break;
    case MAL_OPND_TYPE:
      fprintf (out, "type(%u)", (unsigned) operand->asType);
      break;
    case MAL_OPND_VECDESC:
      fprintf (out, "<%u x %u>", operand->vec.count,
               (unsigned) operand->vec.elementType);
      break;
    }
}

static void
print_program_to_file (FILE *out, const TOSProgram *program)
{
  uint32_t blockIndex;

  if (out == NULL || program == NULL)
    {
      return;
    }
  fprintf (out, "tos-program %s (%u blocks, %u instructions, %s)\n",
           program->machine == NULL || program->machine->name == NULL ? "unknown"
                                                                       : program->machine->name,
           program->blockCount, program->instrCount,
           program->orderKind == TOS_ORDER_REVERSE_POSTORDER ? "rpo" : "input");
  for (blockIndex = 0u; blockIndex < program->blockCount; ++blockIndex)
    {
      const TOSBlock *block = &program->blocks[blockIndex];
      uint32_t instIndex;
      fprintf (out, "layout b%u", block->id);
      if (block->successorCount != 0u)
        {
          uint32_t succIndex;
          fprintf (out, " -> ");
          for (succIndex = 0u; succIndex < block->successorCount; ++succIndex)
            {
              if (succIndex != 0u)
                {
                  fprintf (out, ", ");
                }
              fprintf (out, "b%u", block->successors[succIndex]);
            }
        }
      fprintf (out, "\n");
      for (instIndex = 0u; instIndex < block->instrCount; ++instIndex)
        {
          const TOSInst *inst = &program->instructions[block->firstInstr + instIndex];
          uint32_t opIndex;
          fprintf (out, "  t%u: %s", inst->id,
                   inst->desc == NULL || inst->desc->mnemonic == NULL
                       ? "<invalid>"
                       : inst->desc->mnemonic);
          if (inst->operandCount != 0u)
            {
              fputc (' ', out);
            }
          for (opIndex = 0u; opIndex < inst->operandCount; ++opIndex)
            {
              if (opIndex != 0u)
                {
                  fprintf (out, ", ");
                }
              print_operand (out, &inst->operands[opIndex]);
            }
          fputc ('\n', out);
        }
    }
}

void
tosPrintProgram (const TOSProgram *program)
{
  print_program_to_file (stdout, program);
}

int
tosEmitText (const TOSProgram *program, const char *path)
{
  FILE *out;

  if (program == NULL || path == NULL)
    {
      return 0;
    }
  out = fopen (path, "w");
  if (out == NULL)
    {
      return 0;
    }
  print_program_to_file (out, program);
  fclose (out);
  return 1;
}
