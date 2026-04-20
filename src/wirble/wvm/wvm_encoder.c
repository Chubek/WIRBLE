#include "wvm_encoder.h"

#include <wirble/wirble-mal.h>
#include <wirble/wirble-mds.h>

#include <limits.h>
#include <stdlib.h>
#include <string.h>

typedef struct WVMEncodeBuilder
{
  WVMInstr *instrs;
  uint32_t instrCount;
  uint32_t instrCap;
  WVMValue *consts;
  uint32_t constCount;
  uint32_t constCap;
  WVMReg scratchReg;
  uint16_t regCount;
} WVMEncodeBuilder;

const WVMEncodingType wvmOpcodeEncodings[WVM_OP_COUNT] = {
  [WVM_OP_NOP] = WVM_ENC_OP,
  [WVM_OP_HALT] = WVM_ENC_OP,
  [WVM_OP_RETURN] = WVM_ENC_OP_R,
  [WVM_OP_CALL] = WVM_ENC_OP_RRR,
  [WVM_OP_TAILCALL] = WVM_ENC_OP_RRR,
  [WVM_OP_JUMP] = WVM_ENC_OP_JUMP,
  [WVM_OP_JUMP_IF_TRUE] = WVM_ENC_OP_R_JUMP,
  [WVM_OP_JUMP_IF_FALSE] = WVM_ENC_OP_R_JUMP,
  [WVM_OP_MOVE] = WVM_ENC_OP_RR,
  [WVM_OP_LOAD_CONST] = WVM_ENC_OP_R_CONST,
  [WVM_OP_LOAD_IMM] = WVM_ENC_OP_R_IMM32,
  [WVM_OP_LOAD_NIL] = WVM_ENC_OP_R,
  [WVM_OP_LOAD_TRUE] = WVM_ENC_OP_R,
  [WVM_OP_LOAD_FALSE] = WVM_ENC_OP_R,
  [WVM_OP_ADD] = WVM_ENC_OP_RRR,
  [WVM_OP_SUB] = WVM_ENC_OP_RRR,
  [WVM_OP_MUL] = WVM_ENC_OP_RRR,
  [WVM_OP_DIV] = WVM_ENC_OP_RRR,
  [WVM_OP_MOD] = WVM_ENC_OP_RRR,
  [WVM_OP_NEG] = WVM_ENC_OP_RR,
  [WVM_OP_INC] = WVM_ENC_OP_R,
  [WVM_OP_DEC] = WVM_ENC_OP_R,
  [WVM_OP_AND] = WVM_ENC_OP_RRR,
  [WVM_OP_OR] = WVM_ENC_OP_RRR,
  [WVM_OP_XOR] = WVM_ENC_OP_RRR,
  [WVM_OP_NOT] = WVM_ENC_OP_RR,
  [WVM_OP_SHL] = WVM_ENC_OP_RRR,
  [WVM_OP_SHR] = WVM_ENC_OP_RRR,
  [WVM_OP_EQ] = WVM_ENC_OP_RRR,
  [WVM_OP_NE] = WVM_ENC_OP_RRR,
  [WVM_OP_LT] = WVM_ENC_OP_RRR,
  [WVM_OP_LE] = WVM_ENC_OP_RRR,
  [WVM_OP_GT] = WVM_ENC_OP_RRR,
  [WVM_OP_GE] = WVM_ENC_OP_RRR,
  [WVM_OP_LOAD_GLOBAL] = WVM_ENC_OP_R_CONST,
  [WVM_OP_STORE_GLOBAL] = WVM_ENC_OP_R_CONST,
  [WVM_OP_LOAD_UPVAL] = WVM_ENC_OP_R_IMM16,
  [WVM_OP_STORE_UPVAL] = WVM_ENC_OP_R_IMM16,
  [WVM_OP_LOAD_FIELD] = WVM_ENC_OP_RR_CONST,
  [WVM_OP_STORE_FIELD] = WVM_ENC_OP_RR_CONST,
  [WVM_OP_NEW_TABLE] = WVM_ENC_OP_R,
  [WVM_OP_GET_INDEX] = WVM_ENC_OP_RRR,
  [WVM_OP_SET_INDEX] = WVM_ENC_OP_RRR,
  [WVM_OP_GET_LEN] = WVM_ENC_OP_RR,
  [WVM_OP_CLOSURE] = WVM_ENC_OP_R_CONST,
  [WVM_OP_GUARD_TYPE] = WVM_ENC_OP_GUARD,
  [WVM_OP_GUARD_TRUE] = WVM_ENC_OP_GUARD,
  [WVM_OP_GUARD_FALSE] = WVM_ENC_OP_GUARD,
  [WVM_OP_GUARD_NOT_NIL] = WVM_ENC_OP_GUARD,
  [WVM_OP_GUARD_CLASS] = WVM_ENC_OP_GUARD,
  [WVM_OP_GUARD_RANGE] = WVM_ENC_OP_GUARD,
  [WVM_OP_GUARD_RESOURCE] = WVM_ENC_OP_GUARD,
  [WVM_OP_PUSH] = WVM_ENC_OP_R,
  [WVM_OP_POP] = WVM_ENC_OP_R,
  [WVM_OP_TO_INT] = WVM_ENC_OP_RR,
  [WVM_OP_TO_FLOAT] = WVM_ENC_OP_RR,
  [WVM_OP_TO_STRING] = WVM_ENC_OP_RR,
  [WVM_OP_TO_BOOL] = WVM_ENC_OP_RR,
};

static const char *const wvmOpcodeNames[WVM_OP_COUNT] = {
  [WVM_OP_NOP] = "nop", [WVM_OP_HALT] = "halt", [WVM_OP_RETURN] = "return",
  [WVM_OP_CALL] = "call", [WVM_OP_TAILCALL] = "tailcall",
  [WVM_OP_JUMP] = "jump", [WVM_OP_JUMP_IF_TRUE] = "jump_if_true",
  [WVM_OP_JUMP_IF_FALSE] = "jump_if_false", [WVM_OP_MOVE] = "move",
  [WVM_OP_LOAD_CONST] = "load_const", [WVM_OP_LOAD_IMM] = "load_imm",
  [WVM_OP_LOAD_NIL] = "load_nil", [WVM_OP_LOAD_TRUE] = "load_true",
  [WVM_OP_LOAD_FALSE] = "load_false", [WVM_OP_ADD] = "add",
  [WVM_OP_SUB] = "sub", [WVM_OP_MUL] = "mul", [WVM_OP_DIV] = "div",
  [WVM_OP_MOD] = "mod", [WVM_OP_NEG] = "neg", [WVM_OP_INC] = "inc",
  [WVM_OP_DEC] = "dec", [WVM_OP_AND] = "and", [WVM_OP_OR] = "or",
  [WVM_OP_XOR] = "xor", [WVM_OP_NOT] = "not", [WVM_OP_SHL] = "shl",
  [WVM_OP_SHR] = "shr", [WVM_OP_EQ] = "eq", [WVM_OP_NE] = "ne",
  [WVM_OP_LT] = "lt", [WVM_OP_LE] = "le", [WVM_OP_GT] = "gt",
  [WVM_OP_GE] = "ge", [WVM_OP_LOAD_GLOBAL] = "load_global",
  [WVM_OP_STORE_GLOBAL] = "store_global", [WVM_OP_LOAD_UPVAL] = "load_upval",
  [WVM_OP_STORE_UPVAL] = "store_upval", [WVM_OP_LOAD_FIELD] = "load_field",
  [WVM_OP_STORE_FIELD] = "store_field", [WVM_OP_NEW_TABLE] = "new_table",
  [WVM_OP_GET_INDEX] = "get_index", [WVM_OP_SET_INDEX] = "set_index",
  [WVM_OP_GET_LEN] = "get_len", [WVM_OP_CLOSURE] = "closure",
  [WVM_OP_GUARD_TYPE] = "guard_type", [WVM_OP_GUARD_TRUE] = "guard_true",
  [WVM_OP_GUARD_FALSE] = "guard_false",
  [WVM_OP_GUARD_NOT_NIL] = "guard_not_nil",
  [WVM_OP_GUARD_CLASS] = "guard_class",
  [WVM_OP_GUARD_RANGE] = "guard_range",
  [WVM_OP_GUARD_RESOURCE] = "guard_resource", [WVM_OP_PUSH] = "push",
  [WVM_OP_POP] = "pop", [WVM_OP_TO_INT] = "to_int",
  [WVM_OP_TO_FLOAT] = "to_float", [WVM_OP_TO_STRING] = "to_string",
  [WVM_OP_TO_BOOL] = "to_bool"
};

static const char *const wvmGuardNames[WVM_GUARD_KIND_COUNT] = {
  [WVM_GUARD_TYPE_INT] = "type-int",
  [WVM_GUARD_TYPE_FLOAT] = "type-float",
  [WVM_GUARD_TYPE_STRING] = "type-string",
  [WVM_GUARD_TYPE_BOOL] = "type-bool",
  [WVM_GUARD_TYPE_TABLE] = "type-table",
  [WVM_GUARD_TYPE_FUNCTION] = "type-function",
  [WVM_GUARD_TYPE_NIL] = "type-nil",
  [WVM_GUARD_TYPE_USERDATA] = "type-userdata",
  [WVM_GUARD_COND_TRUE] = "cond-true",
  [WVM_GUARD_COND_FALSE] = "cond-false",
  [WVM_GUARD_COND_NOT_NIL] = "cond-not-nil",
  [WVM_GUARD_COND_EQ] = "cond-eq",
  [WVM_GUARD_COND_LT] = "cond-lt",
  [WVM_GUARD_COND_LE] = "cond-le",
  [WVM_GUARD_RANGE_INT_LO] = "range-int-lo",
  [WVM_GUARD_RANGE_INT_HI] = "range-int-hi",
  [WVM_GUARD_RANGE_INT_BOTH] = "range-int-both",
  [WVM_GUARD_CLASS] = "class",
  [WVM_GUARD_SHAPE] = "shape",
  [WVM_GUARD_PROTO] = "proto",
  [WVM_GUARD_RES_STACK_OVERFLOW] = "res-stack-overflow",
  [WVM_GUARD_RES_MEMORY] = "res-memory",
  [WVM_GUARD_RES_TIMEOUT] = "res-timeout",
  [WVM_GUARD_RES_RECURSION_DEPTH] = "res-recursion-depth"
};

char *
wvm_strdup (const char *text)
{
  size_t len;
  char *copy;

  if (text == NULL)
    {
      return NULL;
    }
  len = strlen (text);
  copy = (char *) malloc (len + 1u);
  if (copy != NULL)
    {
      memcpy (copy, text, len + 1u);
    }
  return copy;
}

void
wvmSetErrorf (WVMState *vm, const char *message)
{
  if (vm == NULL)
    {
      return;
    }
  vm->lastError = message;
  vm->hasError = message != NULL;
  if (vm->errorHandler != NULL && message != NULL)
    {
      vm->errorHandler (vm, message);
    }
}

static int
wvm_builder_append_instr (WVMEncodeBuilder *builder, WVMInstr instr)
{
  WVMInstr *grown;
  uint32_t newCap;

  if (builder->instrCount == builder->instrCap)
    {
      newCap = builder->instrCap == 0u ? 8u : builder->instrCap * 2u;
      grown = (WVMInstr *) realloc (builder->instrs,
                                    (size_t) newCap * sizeof (*builder->instrs));
      if (grown == NULL)
        {
          return 0;
        }
      builder->instrs = grown;
      builder->instrCap = newCap;
    }
  builder->instrs[builder->instrCount++] = instr;
  return 1;
}

static int
wvm_builder_append_const (WVMEncodeBuilder *builder, WVMValue value,
                          uint32_t *outIndex)
{
  WVMValue *grown;
  uint32_t newCap;

  if (builder->constCount == builder->constCap)
    {
      newCap = builder->constCap == 0u ? 4u : builder->constCap * 2u;
      grown = (WVMValue *) realloc (builder->consts,
                                    (size_t) newCap * sizeof (*builder->consts));
      if (grown == NULL)
        {
          return 0;
        }
      builder->consts = grown;
      builder->constCap = newCap;
    }
  if (outIndex != NULL)
    {
      *outIndex = builder->constCount;
    }
  builder->consts[builder->constCount++] = value;
  return 1;
}

static int
wvm_operand_to_reg (WVMEncodeBuilder *builder, const MALOperand *operand,
                    WVMReg *outReg)
{
  uint32_t constIndex;

  if (builder == NULL || operand == NULL || outReg == NULL)
    {
      return 0;
    }
  switch (operand->kind)
    {
    case MAL_OPND_REG:
      *outReg = (WVMReg) operand->reg;
      return 1;
    case MAL_OPND_IMM_INT:
      if (operand->imm_i >= INT32_MIN && operand->imm_i <= INT32_MAX)
        {
          *outReg = builder->scratchReg;
          return wvm_builder_append_instr (
              builder,
              WVM_ENCODE_OP_R_IMM32 (WVM_OP_LOAD_IMM, *outReg, operand->imm_i));
        }
      if (!wvm_builder_append_const (builder, WVM_VALUE_FROM_INT (operand->imm_i),
                                     &constIndex))
        {
          return 0;
        }
      *outReg = builder->scratchReg;
      return wvm_builder_append_instr (
          builder, WVM_ENCODE_OP_R_CONST (WVM_OP_LOAD_CONST, *outReg, constIndex));
    case MAL_OPND_IMM_FLOAT:
      if (!wvm_builder_append_const (builder, WVM_VALUE_FROM_FLOAT (operand->imm_f),
                                     &constIndex))
        {
          return 0;
        }
      *outReg = builder->scratchReg;
      return wvm_builder_append_instr (
          builder, WVM_ENCODE_OP_R_CONST (WVM_OP_LOAD_CONST, *outReg, constIndex));
    default:
      return 0;
    }
}

static int
wvm_emit_from_mnemonic (WVMEncodeBuilder *builder, const TOSInst *inst)
{
  const char *name;
  WVMReg dst;
  WVMReg lhs;
  WVMReg rhs;

  if (builder == NULL || inst == NULL || inst->desc == NULL
      || inst->desc->mnemonic == NULL)
    {
      return 0;
    }
  name = inst->desc->mnemonic;
  if (strcmp (name, "x64.mov.rr") == 0 || strcmp (name, "generic.mov.rr") == 0
      || strcmp (name, "a64.mov.rr") == 0 || strcmp (name, "wasm.mov.rr") == 0)
    {
      if (inst->operandCount != 2u || inst->operands[0].kind != MAL_OPND_REG)
        {
          return 0;
        }
      dst = (WVMReg) inst->operands[0].reg;
      if (!wvm_operand_to_reg (builder, &inst->operands[1], &rhs))
        {
          return 0;
        }
      return wvm_builder_append_instr (builder,
                                       WVM_ENCODE_OP_RR (WVM_OP_MOVE, dst, rhs));
    }
  if (strcmp (name, "x64.mov.ri") == 0 || strcmp (name, "generic.mov.ri") == 0
      || strcmp (name, "a64.mov.ri") == 0 || strcmp (name, "wasm.mov.ri") == 0)
    {
      if (inst->operandCount != 2u || inst->operands[0].kind != MAL_OPND_REG)
        {
          return 0;
        }
      dst = (WVMReg) inst->operands[0].reg;
      if (!wvm_operand_to_reg (builder, &inst->operands[1], &rhs))
        {
          return 0;
        }
      if (rhs == dst)
        {
          return 1;
        }
      return wvm_builder_append_instr (builder,
                                       WVM_ENCODE_OP_RR (WVM_OP_MOVE, dst, rhs));
    }
  if (strcmp (name, "x64.ret") == 0 || strcmp (name, "generic.ret") == 0
      || strcmp (name, "a64.ret") == 0 || strcmp (name, "wasm.ret") == 0)
    {
      if (inst->operandCount != 1u || !wvm_operand_to_reg (builder, &inst->operands[0], &dst))
        {
          return 0;
        }
      return wvm_builder_append_instr (builder,
                                       WVM_ENCODE_OP_R (WVM_OP_RETURN, dst));
    }
  if (inst->operandCount < 2u || inst->operands[0].kind != MAL_OPND_REG)
    {
      return 0;
    }
  dst = (WVMReg) inst->operands[0].reg;
  if (!wvm_operand_to_reg (builder, &inst->operands[1], &lhs))
    {
      return 0;
    }
  if (strcmp (name, "x64.neg") == 0 || strcmp (name, "generic.neg") == 0
      || strcmp (name, "a64.neg") == 0 || strcmp (name, "wasm.neg") == 0)
    {
      return wvm_builder_append_instr (builder,
                                       WVM_ENCODE_OP_RR (WVM_OP_NEG, dst, lhs));
    }
  if (strcmp (name, "x64.not") == 0 || strcmp (name, "generic.not") == 0
      || strcmp (name, "a64.not") == 0 || strcmp (name, "wasm.not") == 0)
    {
      return wvm_builder_append_instr (builder,
                                       WVM_ENCODE_OP_RR (WVM_OP_NOT, dst, lhs));
    }
  if (inst->operandCount < 3u || !wvm_operand_to_reg (builder, &inst->operands[2], &rhs))
    {
      return 0;
    }
#define WVM_EMIT_BIN(NAME, OP)                                                 \
  if (strstr (name, (NAME)) != NULL)                                           \
    {                                                                          \
      return wvm_builder_append_instr (builder,                                \
                                       WVM_ENCODE_OP_RRR ((OP), dst, lhs, rhs)); \
    }
  WVM_EMIT_BIN ("add.", WVM_OP_ADD);
  WVM_EMIT_BIN ("sub.", WVM_OP_SUB);
  WVM_EMIT_BIN ("mul.", WVM_OP_MUL);
  WVM_EMIT_BIN ("and.", WVM_OP_AND);
  WVM_EMIT_BIN ("or.", WVM_OP_OR);
  WVM_EMIT_BIN ("xor.", WVM_OP_XOR);
  WVM_EMIT_BIN ("shl.", WVM_OP_SHL);
  WVM_EMIT_BIN ("ashr.", WVM_OP_SHR);
  WVM_EMIT_BIN ("icmp.eq", WVM_OP_EQ);
  WVM_EMIT_BIN ("icmp.ne", WVM_OP_NE);
  WVM_EMIT_BIN ("icmp.slt", WVM_OP_LT);
  WVM_EMIT_BIN ("icmp.sle", WVM_OP_LE);
  WVM_EMIT_BIN ("icmp.sgt", WVM_OP_GT);
  WVM_EMIT_BIN ("icmp.sge", WVM_OP_GE);
#undef WVM_EMIT_BIN
  return 0;
}

static uint16_t
wvm_compute_reg_count (const TOSProgram *program)
{
  uint32_t instIndex;
  uint32_t maxReg = 0u;

  if (program == NULL)
    {
      return 1u;
    }
  for (instIndex = 0u; instIndex < program->instrCount; ++instIndex)
    {
      const TOSInst *inst = &program->instructions[instIndex];
      uint32_t opIndex;
      for (opIndex = 0u; opIndex < inst->operandCount; ++opIndex)
        {
          if (inst->operands[opIndex].kind == MAL_OPND_REG
              && inst->operands[opIndex].reg > maxReg)
            {
              maxReg = inst->operands[opIndex].reg;
            }
        }
    }
  if (maxReg >= UINT16_MAX - 1u)
    {
      return UINT16_MAX;
    }
  return (uint16_t) (maxReg + 2u);
}

WVMPrototype *
wvmPrototypeFromTOS (const TOSProgram *program, const char *name,
                     const char *source)
{
  WVMEncodeBuilder builder;
  WVMPrototype *proto;
  uint32_t instIndex;

  if (program == NULL)
    {
      return NULL;
    }
  memset (&builder, 0, sizeof (builder));
  builder.regCount = wvm_compute_reg_count (program);
  builder.scratchReg = (WVMReg) (builder.regCount - 1u);

  for (instIndex = 0u; instIndex < program->instrCount; ++instIndex)
    {
      if (!wvm_emit_from_mnemonic (&builder, &program->instructions[instIndex]))
        {
          free (builder.instrs);
          free (builder.consts);
          return NULL;
        }
    }
  if (builder.instrCount == 0u
      && !wvm_builder_append_instr (&builder, WVM_ENCODE_OP (WVM_OP_HALT)))
    {
      free (builder.instrs);
      free (builder.consts);
      return NULL;
    }

  proto = (WVMPrototype *) calloc (1u, sizeof (*proto));
  if (proto == NULL)
    {
      free (builder.instrs);
      free (builder.consts);
      return NULL;
    }
  proto->header.type = WVM_OBJ_FUNCTION;
  proto->name = wvm_strdup (name == NULL ? "main" : name);
  proto->source = wvm_strdup (source == NULL ? "<tos>" : source);
  proto->instrCount = builder.instrCount;
  proto->instrs = builder.instrs;
  proto->constCount = builder.constCount;
  proto->consts = builder.consts;
  proto->regCount = builder.regCount;
  proto->paramCount = 0u;
  proto->lineMap = (uint32_t *) calloc (proto->instrCount == 0u ? 1u : proto->instrCount,
                                        sizeof (*proto->lineMap));
  if (proto->lineMap == NULL)
    {
      wvmPrototypeDestroyInternal (NULL, proto);
      return NULL;
    }
  return proto;
}

void
wvmPrototypeDestroyInternal (WVMState *vm, WVMPrototype *proto)
{
  uint32_t index;

  (void) vm;
  if (proto == NULL)
    {
      return;
    }
  free ((void *) proto->name);
  free ((void *) proto->source);
  free (proto->instrs);
  free (proto->consts);
  if (proto->strs != NULL)
    {
      for (index = 0u; index < proto->strCount; ++index)
        {
          free (proto->strs[index]);
        }
    }
  free (proto->strs);
  free (proto->upvalIsLocal);
  free (proto->upvalIndex);
  free (proto->protos);
  free (proto->lineMap);
  free (proto);
}

WVMEncodingType
wvmGetEncoding (WVMOpcode op)
{
  return op < WVM_OP_COUNT ? wvmOpcodeEncodings[op] : WVM_ENC_OP;
}

const char *
wvmOpcodeName (WVMOpcode op)
{
  return op < WVM_OP_COUNT && wvmOpcodeNames[op] != NULL ? wvmOpcodeNames[op]
                                                         : "unknown";
}

int
wvmOpcodeRegCount (WVMOpcode op)
{
  switch (wvmGetEncoding (op))
    {
    case WVM_ENC_OP_R: return 1;
    case WVM_ENC_OP_RR: return 2;
    case WVM_ENC_OP_RRR: return 3;
    case WVM_ENC_OP_R_IMM16:
    case WVM_ENC_OP_R_IMM32:
    case WVM_ENC_OP_R_CONST:
    case WVM_ENC_OP_R_STR:
    case WVM_ENC_OP_R_JUMP:
      return 1;
    case WVM_ENC_OP_RR_CONST:
    case WVM_ENC_OP_RR_JUMP:
      return 2;
    case WVM_ENC_OP_GUARD:
      return 1;
    default:
      return 0;
    }
}

int
wvmOpcodeIsBranch (WVMOpcode op)
{
  return op == WVM_OP_JUMP || op == WVM_OP_JUMP_IF_TRUE
         || op == WVM_OP_JUMP_IF_FALSE || op == WVM_OP_RETURN;
}

int
wvmOpcodeIsGuard (WVMOpcode op)
{
  return op >= WVM_OP_GUARD_TYPE && op <= WVM_OP_GUARD_RESOURCE;
}

int
wvmOpcodeHasSideEffect (WVMOpcode op)
{
  return op == WVM_OP_RETURN || op == WVM_OP_CALL || op == WVM_OP_TAILCALL
         || op == WVM_OP_STORE_GLOBAL || op == WVM_OP_STORE_UPVAL
         || op == WVM_OP_STORE_FIELD || op == WVM_OP_SET_INDEX;
}

const char *
wvmGuardKindName (WVMGuardKind kind)
{
  return kind < WVM_GUARD_KIND_COUNT && wvmGuardNames[kind] != NULL
             ? wvmGuardNames[kind]
             : "guard-unknown";
}

int
wvmVerifyPrototype (WVMState *vm, const WVMPrototype *proto)
{
  uint32_t index;

  if (proto == NULL || proto->instrs == NULL)
    {
      wvmSetErrorf (vm, "prototype is missing bytecode");
      return 0;
    }
  for (index = 0u; index < proto->instrCount; ++index)
    {
      WVMInstr instr = proto->instrs[index];
      WVMOpcode op = WVM_DECODE_OP (instr);
      if (op >= WVM_OP_COUNT)
        {
          wvmSetErrorf (vm, "prototype contains an invalid opcode");
          return 0;
        }
      switch (wvmGetEncoding (op))
        {
        case WVM_ENC_OP_R:
          if (WVM_DECODE_R_A (instr) >= proto->regCount)
            {
              wvmSetErrorf (vm, "prototype references an invalid register");
              return 0;
            }
          break;
        case WVM_ENC_OP_RR:
          if (WVM_DECODE_R_A (instr) >= proto->regCount
              || WVM_DECODE_R_B (instr) >= proto->regCount)
            {
              wvmSetErrorf (vm, "prototype references an invalid register");
              return 0;
            }
          break;
        case WVM_ENC_OP_RRR:
          if (WVM_DECODE_R_A (instr) >= proto->regCount
              || WVM_DECODE_R_B (instr) >= proto->regCount
              || WVM_DECODE_R_C (instr) >= proto->regCount)
            {
              wvmSetErrorf (vm, "prototype references an invalid register");
              return 0;
            }
          break;
        case WVM_ENC_OP_R_CONST:
          if (WVM_DECODE_R_A (instr) >= proto->regCount
              || WVM_DECODE_CONST (instr) >= proto->constCount)
            {
              wvmSetErrorf (vm, "prototype references an invalid constant");
              return 0;
            }
          break;
        case WVM_ENC_OP_R_IMM32:
          if (WVM_DECODE_R_A (instr) >= proto->regCount)
            {
              wvmSetErrorf (vm, "prototype references an invalid register");
              return 0;
            }
          break;
        default:
          break;
        }
    }
  wvmClearError (vm);
  return 1;
}

const char *
wvmVersion (void)
{
  return WVM_VERSION_STRING;
}

const char *
wvmBuildInfo (void)
{
  return "WIRBLE Stage10 WVM backend";
}

uint32_t
wvmFeatures (void)
{
  return 0u;
}
