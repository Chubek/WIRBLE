#include "wvm_encoder.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void *
default_alloc (void *ud, void *ptr, size_t oldSize, size_t newSize)
{
  (void) ud;
  (void) oldSize;
  if (newSize == 0u)
    {
      free (ptr);
      return NULL;
    }
  return realloc (ptr, newSize);
}

void *
wvmAlloc (WVMState *vm, size_t size)
{
  void *ptr;
  if (vm == NULL)
    {
      return malloc (size);
    }
  ptr = vm->allocFn (vm->allocUD, NULL, 0u, size);
  if (ptr != NULL)
    {
      vm->bytesAllocated += size;
    }
  return ptr;
}

void *
wvmRealloc (WVMState *vm, void *ptr, size_t oldSize, size_t newSize)
{
  void *result;
  if (vm == NULL)
    {
      return realloc (ptr, newSize);
    }
  result = vm->allocFn (vm->allocUD, ptr, oldSize, newSize);
  if (newSize > oldSize)
    {
      vm->bytesAllocated += newSize - oldSize;
    }
  return result;
}

void
wvmFree (WVMState *vm, void *ptr, size_t size)
{
  if (vm != NULL && vm->bytesAllocated >= size)
    {
      vm->bytesAllocated -= size;
    }
  if (vm == NULL)
    {
      free (ptr);
      return;
    }
  (void) vm->allocFn (vm->allocUD, ptr, size, 0u);
}

WVMState *
wvmCreateEx (void *(*allocFn) (void *ud, void *ptr, size_t oldSize,
                               size_t newSize),
             void *allocUD)
{
  WVMState *vm = (WVMState *) calloc (1u, sizeof (*vm));
  if (vm == NULL)
    {
      return NULL;
    }
  vm->allocFn = allocFn == NULL ? default_alloc : allocFn;
  vm->allocUD = allocUD;
  vm->stackCap = 32u;
  vm->stack = (WVMValue *) calloc (vm->stackCap, sizeof (*vm->stack));
  if (vm->stack == NULL)
    {
      free (vm);
      return NULL;
    }
  vm->maxFrameDepth = 16u;
  return vm;
}

WVMState *
wvmCreate (void)
{
  return wvmCreateEx (NULL, NULL);
}

void
wvmDestroy (WVMState *vm)
{
  if (vm == NULL)
    {
      return;
    }
  free (vm->stack);
  free (vm->modulePaths);
  free (vm);
}

void
wvmSetGCConfig (WVMState *vm, const WVMGCConfig *config)
{
  if (vm != NULL && config != NULL)
    {
      vm->gcConfig = *config;
    }
}

void
wvmGetGCConfig (const WVMState *vm, WVMGCConfig *out)
{
  if (vm != NULL && out != NULL)
    {
      *out = vm->gcConfig;
    }
}

void
wvmGetGCStats (const WVMState *vm, WVMGCStats *out)
{
  if (vm != NULL && out != NULL)
    {
      *out = vm->gcStats;
    }
}

void
wvmForceGC (WVMState *vm)
{
  (void) vm;
}

void
wvmSetJITConfig (WVMState *vm, const WVMJITConfig *config)
{
  if (vm != NULL && config != NULL)
    {
      vm->jitConfig = *config;
    }
}

void
wvmGetJITConfig (const WVMState *vm, WVMJITConfig *out)
{
  if (vm != NULL && out != NULL)
    {
      *out = vm->jitConfig;
    }
}

void
wvmSetErrorHandler (WVMState *vm, void (*handler) (WVMState *vm, const char *msg))
{
  if (vm != NULL)
    {
      vm->errorHandler = handler;
    }
}

const char *
wvmGetLastError (const WVMState *vm)
{
  return vm == NULL ? NULL : vm->lastError;
}

void
wvmClearError (WVMState *vm)
{
  if (vm != NULL)
    {
      vm->lastError = NULL;
      vm->hasError = 0;
    }
}

void
wvmSetUserData (WVMState *vm, void *ud)
{
  if (vm != NULL)
    {
      vm->userData = ud;
    }
}

void *
wvmGetUserData (const WVMState *vm)
{
  return vm == NULL ? NULL : vm->userData;
}

void
wvmFreePrototype (WVMState *vm, WVMPrototype *proto)
{
  wvmPrototypeDestroyInternal (vm, proto);
}

void
wvmEnsureStack (WVMState *vm, uint32_t extra)
{
  WVMValue *grown;
  uint32_t needed;
  uint32_t newCap;

  if (vm == NULL)
    {
      return;
    }
  needed = vm->stackSize + extra;
  if (needed <= vm->stackCap)
    {
      return;
    }
  newCap = vm->stackCap == 0u ? 32u : vm->stackCap;
  while (newCap < needed)
    {
      newCap *= 2u;
    }
  grown = (WVMValue *) realloc (vm->stack, (size_t) newCap * sizeof (*vm->stack));
  if (grown != NULL)
    {
      memset (grown + vm->stackCap, 0, (size_t) (newCap - vm->stackCap) * sizeof (*grown));
      vm->stack = grown;
      vm->stackCap = newCap;
    }
}

void
wvmPush (WVMState *vm, WVMValue val)
{
  if (vm == NULL)
    {
      return;
    }
  wvmEnsureStack (vm, 1u);
  vm->stack[vm->stackSize++] = val;
}

WVMValue
wvmPop (WVMState *vm)
{
  if (vm == NULL || vm->stackSize == 0u)
    {
      return WVM_VALUE_NIL;
    }
  return vm->stack[--vm->stackSize];
}

WVMValue
wvmPeek (const WVMState *vm, int offset)
{
  uint32_t index;
  if (vm == NULL || offset < 0 || (uint32_t) offset >= vm->stackSize)
    {
      return WVM_VALUE_NIL;
    }
  index = vm->stackSize - 1u - (uint32_t) offset;
  return vm->stack[index];
}

void
wvmSetReg (WVMState *vm, WVMReg reg, WVMValue val)
{
  if (vm == NULL)
    {
      return;
    }
  if (reg >= vm->stackCap)
    {
      wvmEnsureStack (vm, reg - vm->stackSize + 1u);
    }
  if (reg >= vm->stackSize)
    {
      vm->stackSize = reg + 1u;
    }
  vm->stack[reg] = val;
}

WVMValue
wvmGetReg (const WVMState *vm, WVMReg reg)
{
  if (vm == NULL || reg >= vm->stackSize)
    {
      return WVM_VALUE_NIL;
    }
  return vm->stack[reg];
}

uint32_t
wvmStackSize (const WVMState *vm)
{
  return vm == NULL ? 0u : vm->stackSize;
}

static int
wvm_exec_binary_int (WVMOpcode op, WVMValue lhs, WVMValue rhs, WVMValue *out)
{
  int32_t a;
  int32_t b;

  if (!WVM_IS_INT (lhs) || !WVM_IS_INT (rhs) || out == NULL)
    {
      return 0;
    }
  a = WVM_AS_INT (lhs);
  b = WVM_AS_INT (rhs);
  switch (op)
    {
    case WVM_OP_ADD: *out = WVM_VALUE_FROM_INT (a + b); return 1;
    case WVM_OP_SUB: *out = WVM_VALUE_FROM_INT (a - b); return 1;
    case WVM_OP_MUL: *out = WVM_VALUE_FROM_INT (a * b); return 1;
    case WVM_OP_DIV: *out = WVM_VALUE_FROM_INT (b == 0 ? 0 : a / b); return 1;
    case WVM_OP_MOD: *out = WVM_VALUE_FROM_INT (b == 0 ? 0 : a % b); return 1;
    case WVM_OP_AND: *out = WVM_VALUE_FROM_INT (a & b); return 1;
    case WVM_OP_OR: *out = WVM_VALUE_FROM_INT (a | b); return 1;
    case WVM_OP_XOR: *out = WVM_VALUE_FROM_INT (a ^ b); return 1;
    case WVM_OP_SHL: *out = WVM_VALUE_FROM_INT (a << b); return 1;
    case WVM_OP_SHR: *out = WVM_VALUE_FROM_INT (a >> b); return 1;
    case WVM_OP_EQ: *out = a == b ? WVM_VALUE_TRUE : WVM_VALUE_FALSE; return 1;
    case WVM_OP_NE: *out = a != b ? WVM_VALUE_TRUE : WVM_VALUE_FALSE; return 1;
    case WVM_OP_LT: *out = a < b ? WVM_VALUE_TRUE : WVM_VALUE_FALSE; return 1;
    case WVM_OP_LE: *out = a <= b ? WVM_VALUE_TRUE : WVM_VALUE_FALSE; return 1;
    case WVM_OP_GT: *out = a > b ? WVM_VALUE_TRUE : WVM_VALUE_FALSE; return 1;
    case WVM_OP_GE: *out = a >= b ? WVM_VALUE_TRUE : WVM_VALUE_FALSE; return 1;
    default:
      return 0;
    }
}

int
wvmExecute (WVMState *vm, WVMPrototype *proto)
{
  uint32_t pc = 0u;
  WVMFrame frame;
  uint32_t previousStackSize;

  if (vm == NULL || proto == NULL || !wvmVerifyPrototype (vm, proto))
    {
      return 0;
    }
  wvmClearError (vm);
  previousStackSize = vm->stackSize;
  wvmEnsureStack (vm, proto->regCount);
  if (previousStackSize < proto->regCount)
    {
      memset (vm->stack + previousStackSize, 0,
              (size_t) (proto->regCount - previousStackSize)
                  * sizeof (*vm->stack));
    }
  vm->stackSize = proto->regCount;
  memset (&frame, 0, sizeof (frame));
  frame.ip = proto->instrs;
  frame.regs = vm->stack;
  frame.regCount = proto->regCount;
  frame.pc = 0u;
  vm->topFrame = &frame;
  vm->frameDepth = 1u;

  while (pc < proto->instrCount)
    {
      WVMInstr instr = proto->instrs[pc];
      WVMOpcode op = WVM_DECODE_OP (instr);
      WVMReg a = WVM_DECODE_R_A (instr);
      WVMReg b = WVM_DECODE_R_B (instr);
      WVMReg c = WVM_DECODE_R_C (instr);
      frame.pc = pc;
      switch (op)
        {
        case WVM_OP_NOP:
          ++pc;
          break;
        case WVM_OP_HALT:
          pc = proto->instrCount;
          break;
        case WVM_OP_RETURN:
          vm->stack[0] = vm->stack[a];
          vm->stackSize = proto->regCount == 0u ? 0u : proto->regCount;
          vm->topFrame = NULL;
          vm->frameDepth = 0u;
          return 1;
        case WVM_OP_MOVE:
          vm->stack[a] = vm->stack[b];
          ++pc;
          break;
        case WVM_OP_LOAD_IMM:
          vm->stack[a] = WVM_VALUE_FROM_INT ((int32_t) WVM_DECODE_IMM32 (instr));
          ++pc;
          break;
        case WVM_OP_LOAD_CONST:
          vm->stack[a] = proto->consts[WVM_DECODE_CONST (instr)];
          ++pc;
          break;
        case WVM_OP_LOAD_NIL:
          vm->stack[a] = WVM_VALUE_NIL;
          ++pc;
          break;
        case WVM_OP_LOAD_TRUE:
          vm->stack[a] = WVM_VALUE_TRUE;
          ++pc;
          break;
        case WVM_OP_LOAD_FALSE:
          vm->stack[a] = WVM_VALUE_FALSE;
          ++pc;
          break;
        case WVM_OP_NEG:
          vm->stack[a] = WVM_VALUE_FROM_INT (-WVM_AS_INT (vm->stack[b]));
          ++pc;
          break;
        case WVM_OP_NOT:
          vm->stack[a] = WVM_VALUE_FROM_INT (~WVM_AS_INT (vm->stack[b]));
          ++pc;
          break;
        case WVM_OP_ADD:
        case WVM_OP_SUB:
        case WVM_OP_MUL:
        case WVM_OP_DIV:
        case WVM_OP_MOD:
        case WVM_OP_AND:
        case WVM_OP_OR:
        case WVM_OP_XOR:
        case WVM_OP_SHL:
        case WVM_OP_SHR:
        case WVM_OP_EQ:
        case WVM_OP_NE:
        case WVM_OP_LT:
        case WVM_OP_LE:
        case WVM_OP_GT:
        case WVM_OP_GE:
          if (!wvm_exec_binary_int (op, vm->stack[b], vm->stack[c], &vm->stack[a]))
            {
              wvmSetErrorf (vm, "unsupported operand type in WVM interpreter");
              vm->topFrame = NULL;
              vm->frameDepth = 0u;
              return 0;
            }
          ++pc;
          break;
        case WVM_OP_JUMP:
          pc = (uint32_t) ((int32_t) pc + WVM_DECODE_JUMP (instr));
          break;
        case WVM_OP_JUMP_IF_TRUE:
          pc = WVM_IS_TRUTHY (vm->stack[a])
                   ? (uint32_t) ((int32_t) pc + WVM_DECODE_R_JUMP (instr))
                   : pc + 1u;
          break;
        case WVM_OP_JUMP_IF_FALSE:
          pc = !WVM_IS_TRUTHY (vm->stack[a])
                   ? (uint32_t) ((int32_t) pc + WVM_DECODE_R_JUMP (instr))
                   : pc + 1u;
          break;
        default:
          wvmSetErrorf (vm, "opcode is not implemented in the Stage 10 interpreter");
          vm->topFrame = NULL;
          vm->frameDepth = 0u;
          return 0;
        }
    }

  vm->topFrame = NULL;
  vm->frameDepth = 0u;
  return 1;
}

int
wvmCall (WVMState *vm, WVMClosure *closure, int argCount, WVMValue *args,
         WVMValue *results, int maxResults)
{
  (void) argCount;
  (void) args;
  if (vm == NULL || closure == NULL || closure->proto == NULL)
    {
      return 0;
    }
  if (!wvmExecute (vm, closure->proto))
    {
      return 0;
    }
  if (results != NULL && maxResults > 0)
    {
      results[0] = wvmGetReg (vm, 0u);
    }
  return 1;
}

int wvmStep (WVMState *vm, uint32_t count) { (void) vm; (void) count; return 0; }
int wvmResume (WVMState *vm, WVMCoroutine *coro, int argCount, WVMValue *args,
               WVMValue *results, int maxResults)
{ (void) vm; (void) coro; (void) argCount; (void) args; (void) results; (void) maxResults; return 0; }
int wvmYield (WVMState *vm, int resultCount, WVMValue *results)
{ (void) vm; (void) resultCount; (void) results; return 0; }
