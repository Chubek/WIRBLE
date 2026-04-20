#include <wirble/wirble-mal.h>
#include <wirble/wirble-mds.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void
mds_print_operand (FILE *out, const MALOperand *operand)
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
mds_print_program_to_file (FILE *out, const MDSProgram *program)
{
  uint32_t blockIndex;

  if (out == NULL || program == NULL)
    {
      return;
    }
  fprintf (out, "mds-program %s (%u blocks)\n",
           program->machine == NULL || program->machine->name == NULL ? "unknown"
                                                                       : program->machine->name,
           program->blockCount);
  for (blockIndex = 0u; blockIndex < program->blockCount; ++blockIndex)
    {
      const MDSBasicBlock *block = &program->blocks[blockIndex];
      uint32_t instIndex;
      fprintf (out, "b%u:\n", block->id);
      for (instIndex = 0u; instIndex < block->instrCount; ++instIndex)
        {
          const MDSInst *inst = &block->instructions[instIndex];
          uint32_t opIndex;
          fprintf (out, "  %s", inst->desc == NULL || inst->desc->mnemonic == NULL
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
              mds_print_operand (out, &inst->operands[opIndex]);
            }
          fputc ('\n', out);
        }
    }
}

void
mdsPrintMachine (const MDSMachine *machine)
{
  uint32_t index;

  if (machine == NULL)
    {
      return;
    }
  printf ("machine %s (%s)\n", machine->name == NULL ? "unknown" : machine->name,
          machine->vendor == NULL ? "unknown" : machine->vendor);
  for (index = 0u; index < machine->instructionCount; ++index)
    {
      mdsPrintInstruction (&machine->instructions[index]);
    }
}

void
mdsPrintInstruction (const MDSInstruction *instr)
{
  uint32_t index;

  if (instr == NULL)
    {
      return;
    }
  printf ("instr %u %s(", instr->id,
          instr->mnemonic == NULL ? "<unnamed>" : instr->mnemonic);
  for (index = 0u; index < instr->operndCount; ++index)
    {
      if (index != 0u)
        {
          printf (", ");
        }
      printf ("%s", instr->operands[index].name == NULL ? "?" : instr->operands[index].name);
    }
  printf (")\n");
}

void
mdsPrintProgram (const MDSProgram *program)
{
  mds_print_program_to_file (stdout, program);
}

int
mdsEmitAssembly (const MDSProgram *program, const char *path)
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
  mds_print_program_to_file (out, program);
  fclose (out);
  return 1;
}

unsigned char *
mdsEmitBuffer (const MDSProgram *program, size_t *outSize)
{
  unsigned char *buffer = NULL;
  size_t size = 0u;
  size_t capacity = 0u;
  uint32_t blockIndex;

  if (outSize != NULL)
    {
      *outSize = 0u;
    }
  if (program == NULL)
    {
      return NULL;
    }
  for (blockIndex = 0u; blockIndex < program->blockCount; ++blockIndex)
    {
      const MDSBasicBlock *block = &program->blocks[blockIndex];
      uint32_t instIndex;
      for (instIndex = 0u; instIndex < block->instrCount; ++instIndex)
        {
          const MDSInst *inst = &block->instructions[instIndex];
          size_t need = size + 1u + inst->operandCount * sizeof (uint64_t);
          uint32_t opIndex;
          unsigned char *grown;
          if (need > capacity)
            {
              capacity = need * 2u;
              grown = (unsigned char *) realloc (buffer, capacity);
              if (grown == NULL)
                {
                  free (buffer);
                  return NULL;
                }
              buffer = grown;
            }
          buffer[size++] = inst->desc == NULL ? 0u : inst->desc->encoding.fixedBytes[0];
          for (opIndex = 0u; opIndex < inst->operandCount; ++opIndex)
            {
              uint64_t payload = 0u;
              const MALOperand *operand = &inst->operands[opIndex];
              if (operand->kind == MAL_OPND_REG)
                {
                  payload = operand->reg;
                }
              else if (operand->kind == MAL_OPND_IMM_INT)
                {
                  payload = (uint64_t) operand->imm_i;
                }
              else if (operand->kind == MAL_OPND_BLOCK)
                {
                  payload = operand->block;
                }
              memcpy (buffer + size, &payload, sizeof (payload));
              size += sizeof (payload);
            }
        }
    }
  if (outSize != NULL)
    {
      *outSize = size;
    }
  return buffer;
}

int
mdsEmitBinary (const MDSProgram *program, const char *path)
{
  unsigned char *buffer;
  size_t size;
  FILE *out;

  if (program == NULL || path == NULL)
    {
      return 0;
    }
  buffer = mdsEmitBuffer (program, &size);
  if (buffer == NULL)
    {
      return 0;
    }
  out = fopen (path, "wb");
  if (out == NULL)
    {
      free (buffer);
      return 0;
    }
  fwrite (buffer, 1u, size, out);
  fclose (out);
  free (buffer);
  return 1;
}

unsigned char *
mdsEncodeInstruction (const MDSInstruction *instr, const MALOperand *operands,
                      uint32_t operandCount, size_t *outSize)
{
  unsigned char *buffer;
  size_t size;
  uint32_t index;

  if (instr == NULL)
    {
      return NULL;
    }
  size = 1u + (size_t) operandCount * sizeof (uint64_t);
  buffer = (unsigned char *) calloc (size == 0u ? 1u : size, 1u);
  if (buffer == NULL)
    {
      return NULL;
    }
  buffer[0] = instr->encoding.fixedBytes == NULL ? 0u : instr->encoding.fixedBytes[0];
  for (index = 0u; index < operandCount; ++index)
    {
      uint64_t payload = 0u;
      if (operands[index].kind == MAL_OPND_REG)
        {
          payload = operands[index].reg;
        }
      else if (operands[index].kind == MAL_OPND_IMM_INT)
        {
          payload = (uint64_t) operands[index].imm_i;
        }
      memcpy (buffer + 1u + (size_t) index * sizeof (payload), &payload,
              sizeof (payload));
    }
  if (outSize != NULL)
    {
      *outSize = size;
    }
  return buffer;
}

const MDSInstruction *
mdsDecodeInstruction (const MDSMachine *machine, const unsigned char *bytes,
                      size_t length)
{
  if (machine == NULL || bytes == NULL || length == 0u || machine->instrById == NULL)
    {
      return NULL;
    }
  if ((uint32_t) bytes[0] >= machine->instructionCount)
    {
      return NULL;
    }
  return machine->instrById[bytes[0]];
}

void
mdsDestroyProgram (MDSProgram *program)
{
  uint32_t blockIndex;

  if (program == NULL)
    {
      return;
    }
  for (blockIndex = 0u; blockIndex < program->blockCount; ++blockIndex)
    {
      MDSBasicBlock *block = &program->blocks[blockIndex];
      uint32_t instIndex;
      for (instIndex = 0u; instIndex < block->instrCount; ++instIndex)
        {
          free (block->instructions[instIndex].operands);
        }
      free (block->instructions);
      free (block->successors);
    }
  free (program->blocks);
  free (program);
}
