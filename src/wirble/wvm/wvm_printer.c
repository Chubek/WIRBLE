#include "wvm_encoder.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *
wvm_format (const char *text)
{
  return wvm_strdup (text == NULL ? "" : text);
}

char *
wvmDisassembleInstr (WVMInstr instr)
{
  char buffer[128];
  WVMOpcode op = WVM_DECODE_OP (instr);

  switch (wvmGetEncoding (op))
    {
    case WVM_ENC_OP:
      snprintf (buffer, sizeof (buffer), "%s", wvmOpcodeName (op));
      break;
    case WVM_ENC_OP_R:
      snprintf (buffer, sizeof (buffer), "%s r%u", wvmOpcodeName (op),
                WVM_DECODE_R_A (instr));
      break;
    case WVM_ENC_OP_RR:
      snprintf (buffer, sizeof (buffer), "%s r%u, r%u", wvmOpcodeName (op),
                WVM_DECODE_R_A (instr), WVM_DECODE_R_B (instr));
      break;
    case WVM_ENC_OP_RRR:
      snprintf (buffer, sizeof (buffer), "%s r%u, r%u, r%u", wvmOpcodeName (op),
                WVM_DECODE_R_A (instr), WVM_DECODE_R_B (instr),
                WVM_DECODE_R_C (instr));
      break;
    case WVM_ENC_OP_R_IMM32:
      snprintf (buffer, sizeof (buffer), "%s r%u, %d", wvmOpcodeName (op),
                WVM_DECODE_R_A (instr), (int32_t) WVM_DECODE_IMM32 (instr));
      break;
    case WVM_ENC_OP_R_CONST:
      snprintf (buffer, sizeof (buffer), "%s r%u, k%u", wvmOpcodeName (op),
                WVM_DECODE_R_A (instr), WVM_DECODE_CONST (instr));
      break;
    case WVM_ENC_OP_JUMP:
      snprintf (buffer, sizeof (buffer), "%s %d", wvmOpcodeName (op),
                (int) WVM_DECODE_JUMP (instr));
      break;
    case WVM_ENC_OP_R_JUMP:
      snprintf (buffer, sizeof (buffer), "%s r%u, %d", wvmOpcodeName (op),
                WVM_DECODE_R_A (instr), (int) WVM_DECODE_R_JUMP (instr));
      break;
    default:
      snprintf (buffer, sizeof (buffer), "%s ?", wvmOpcodeName (op));
      break;
    }
  return wvm_strdup (buffer);
}

char *
wvmDisassemble (WVMState *vm, const WVMPrototype *proto)
{
  char *text;
  size_t cap = 128u;
  size_t len = 0u;
  uint32_t index;

  (void) vm;
  if (proto == NULL)
    {
      return wvm_format ("<null prototype>");
    }
  text = (char *) malloc (cap);
  if (text == NULL)
    {
      return NULL;
    }
  text[0] = '\0';
  for (index = 0u; index < proto->instrCount; ++index)
    {
      char *line = wvmDisassembleInstr (proto->instrs[index]);
      size_t needed = len + strlen (line) + 32u;
      if (needed > cap)
        {
          char *grown;
          while (cap < needed)
            {
              cap *= 2u;
            }
          grown = (char *) realloc (text, cap);
          if (grown == NULL)
            {
              free (line);
              free (text);
              return NULL;
            }
          text = grown;
        }
      len += (size_t) snprintf (text + len, cap - len, "%04u  %s\n", index, line);
      free (line);
    }
  return text;
}

const char *
wvmValueTypeName (WVMValue val)
{
  if (WVM_IS_NIL (val)) return "nil";
  if (WVM_IS_BOOL (val)) return "bool";
  if (WVM_IS_INT (val)) return "int";
  if (WVM_IS_FLOAT (val)) return "float";
  if (WVM_IS_OBJ (val)) return "object";
  return "unknown";
}

char *
wvmValueToString (WVMState *vm, WVMValue val)
{
  char buffer[64];
  (void) vm;
  if (WVM_IS_NIL (val)) return wvm_format ("nil");
  if (WVM_IS_TRUE (val)) return wvm_format ("true");
  if (WVM_IS_FALSE (val)) return wvm_format ("false");
  if (WVM_IS_INT (val))
    {
      snprintf (buffer, sizeof (buffer), "%d", WVM_AS_INT (val));
      return wvm_strdup (buffer);
    }
  if (WVM_IS_FLOAT (val))
    {
      snprintf (buffer, sizeof (buffer), "%g", WVM_AS_FLOAT (val));
      return wvm_strdup (buffer);
    }
  snprintf (buffer, sizeof (buffer), "<%s>", wvmValueTypeName (val));
  return wvm_strdup (buffer);
}

char *
wvmStackTrace (const WVMState *vm)
{
  char buffer[128];
  snprintf (buffer, sizeof (buffer), "frames=%u", wvmFrameDepth (vm));
  return wvm_strdup (buffer);
}

uint32_t
wvmFrameDepth (const WVMState *vm)
{
  return vm == NULL ? 0u : vm->frameDepth;
}

const WVMFrame *
wvmGetFrame (const WVMState *vm, uint32_t depth)
{
  const WVMFrame *frame;
  uint32_t index;
  if (vm == NULL)
    {
      return NULL;
    }
  frame = vm->topFrame;
  for (index = 0u; frame != NULL && index < depth; ++index)
    {
      frame = frame->prev;
    }
  return frame;
}

const char *
wvmFrameFunctionName (const WVMFrame *frame)
{
  return frame != NULL && frame->closure != NULL && frame->closure->proto != NULL
             ? frame->closure->proto->name
             : "<frame>";
}

const char *
wvmFrameSourceFile (const WVMFrame *frame)
{
  return frame != NULL && frame->closure != NULL && frame->closure->proto != NULL
             ? frame->closure->proto->source
             : "<unknown>";
}

uint32_t
wvmFrameLine (const WVMFrame *frame)
{
  return frame == NULL ? 0u : frame->pc;
}

void
wvmDumpState (const WVMState *vm, void *file)
{
  FILE *out = file == NULL ? stdout : (FILE *) file;
  fprintf (out, "wvm state: stack=%u frames=%u error=%s\n",
           vm == NULL ? 0u : vm->stackSize, vm == NULL ? 0u : vm->frameDepth,
           vm == NULL || vm->lastError == NULL ? "<none>" : vm->lastError);
}

void
wvmDumpJITStats (const WVMState *vm, void *file)
{
  FILE *out = file == NULL ? stdout : (FILE *) file;
  fprintf (out, "wvm jit: tier=%u\n", vm == NULL ? 0u : vm->jitConfig.maxTier);
}

void
wvmDumpGCStats (const WVMState *vm, void *file)
{
  FILE *out = file == NULL ? stdout : (FILE *) file;
  fprintf (out, "wvm gc: allocated=%zu collections=%llu\n",
           vm == NULL ? 0u : vm->bytesAllocated,
           (unsigned long long) (vm == NULL ? 0u : vm->gcStats.collectionCount));
}
