#include <wirble/wirble-mal.h>
#include <wirble/wirble-mds.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const MDSInstruction *mds_find_instruction_by_mnemonic (const MDSMachine *machine,
                                                        const char *mnemonic);

static uint32_t
mds_operand_bit_width (MALType type)
{
  switch (type)
    {
    case MAL_TYPE_I1: return 1u;
    case MAL_TYPE_I8: return 8u;
    case MAL_TYPE_I16: return 16u;
    case MAL_TYPE_I32: return 32u;
    case MAL_TYPE_I64:
    case MAL_TYPE_PTR:
      return 64u;
    case MAL_TYPE_F32: return 32u;
    case MAL_TYPE_F64: return 64u;
    case MAL_TYPE_VEC: return 128u;
    case MAL_TYPE_VOID:
    default:
      return 0u;
    }
}

static int
mds_is_immediate (const MALOperand *operand)
{
  return operand != NULL
         && (operand->kind == MAL_OPND_IMM_INT || operand->kind == MAL_OPND_IMM_FLOAT);
}

static int
mds_match_operand_constraint (const MDSPatternNode *root, uint32_t index,
                              const MALOperand *operand)
{
  uint32_t width;

  if (root == NULL || operand == NULL || index >= root->operandCount)
    {
      return 0;
    }
  if (root->operands[index].mustBeConst && !mds_is_immediate (operand))
    {
      return 0;
    }
  if (root->operands[index].mustBeReg && operand->kind != MAL_OPND_REG)
    {
      return 0;
    }
  if (root->operands[index].mustBeImm && !mds_is_immediate (operand))
    {
      return 0;
    }
  width = mds_operand_bit_width (operand->type);
  if (root->operands[index].bitWidth != 0 && width != 0u
      && width > (uint32_t) root->operands[index].bitWidth)
    {
      return 0;
    }
  return 1;
}

static int
mds_pattern_matches_inst (const MDSPattern *pattern, const MALInst *inst)
{
  uint32_t index;

  if (pattern == NULL || inst == NULL || pattern->root == NULL)
    {
      return 0;
    }
  if (pattern->root->malOpcode != (uint32_t) inst->opcode)
    {
      return 0;
    }
  if (pattern->root->operandCount != inst->operandCount)
    {
      return 0;
    }
  for (index = 0u; index < inst->operandCount; ++index)
    {
      if (!mds_match_operand_constraint (pattern->root, index, &inst->operands[index]))
        {
          return 0;
        }
    }
  return 1;
}

void
mdsAddPattern (MDSInstrSelector *selector, MDSPattern *pattern)
{
  MDSPattern *grown;
  uint32_t opcode;

  if (selector == NULL || pattern == NULL)
    {
      return;
    }
  grown = (MDSPattern *) realloc (selector->patterns,
                                  (size_t) (selector->patternCount + 1u)
                                      * sizeof (*selector->patterns));
  if (grown == NULL)
    {
      return;
    }
  selector->patterns = grown;
  selector->patterns[selector->patternCount] = *pattern;
  opcode = pattern->root == NULL ? 0u : pattern->root->malOpcode;
  if (selector->patternsByOpcode != NULL && opcode <= MAL_OP_TARGET
      && selector->patternsByOpcode[opcode] == NULL)
    {
      selector->patternsByOpcode[opcode] = &selector->patterns[selector->patternCount];
    }
  ++selector->patternCount;
}

static int
mds_build_mnemonic (char *buffer, size_t bufferSize, const MDSMachine *machine,
                    const char *suffix)
{
  const char *prefix;

  if (buffer == NULL || bufferSize == 0u || machine == NULL || suffix == NULL)
    {
      return 0;
    }
  switch (machine->targetKind)
    {
    case MDS_TARGET_X86_64: prefix = "x64"; break;
    case MDS_TARGET_AARCH64: prefix = "a64"; break;
    case MDS_TARGET_WASM: prefix = "wasm"; break;
    case MDS_TARGET_GENERIC:
    default: prefix = "generic"; break;
    }
  return snprintf (buffer, bufferSize, "%s.%s", prefix, suffix) > 0;
}

static int
mds_add_builtin_pattern (MDSInstrSelector *selector, MALOpcode opcode,
                         const char *suffix, int firstInputReg,
                         int lastInputImm)
{
  MDSPattern pattern;
  MDSPatternNode *root;
  const MDSInstruction *instr;
  MDSInstrId *emitIds;
  char mnemonic[64];

  if (selector == NULL || selector->machine == NULL
      || !mds_build_mnemonic (mnemonic, sizeof (mnemonic), selector->machine, suffix))
    {
      return 0;
    }
  instr = mds_find_instruction_by_mnemonic (selector->machine, mnemonic);
  if (instr == NULL)
    {
      return 1;
    }
  memset (&pattern, 0, sizeof (pattern));
  emitIds = (MDSInstrId *) calloc (1u, sizeof (*emitIds));
  root = (MDSPatternNode *) calloc (1u, sizeof (*root));
  if (emitIds == NULL || root == NULL)
    {
      free (emitIds);
      free (root);
      return 0;
    }
  emitIds[0] = instr->id;
  root->malOpcode = (uint32_t) opcode;
  root->operandCount = instr->operndCount == 0u
                           ? 0u
                           : instr->operndCount
                                 - (instr->operands[0].isOutput ? 1u : 0u);
  root->operands = root->operandCount == 0u
                       ? NULL
                       : calloc (root->operandCount, sizeof (*root->operands));
  if (root->operandCount != 0u && root->operands == NULL)
    {
      free (emitIds);
      free (root);
      return 0;
    }
  if (root->operandCount >= 1u)
    {
      root->operands[0].mustBeReg = firstInputReg;
      root->operands[0].mustBeImm = root->operandCount == 1u && lastInputImm;
      root->operands[0].mustBeConst = root->operandCount == 1u && lastInputImm;
      root->operands[0].bitWidth = 64;
    }
  if (root->operandCount >= 2u)
    {
      root->operands[0].mustBeReg = firstInputReg;
      root->operands[1].mustBeImm = lastInputImm;
      root->operands[1].mustBeReg = !lastInputImm;
      root->operands[1].mustBeConst = lastInputImm;
      root->operands[1].bitWidth = 64;
    }
  pattern.id = selector->patternCount;
  pattern.root = root;
  pattern.emitInstrs = emitIds;
  pattern.emitInstrCount = 1u;
  pattern.cost = instr->selectionCost;
  mdsAddPattern (selector, &pattern);
  return 1;
}

static int
mds_add_ret_pattern (MDSInstrSelector *selector, const char *suffix)
{
  return mds_add_builtin_pattern (selector, MAL_OP_RET, suffix, 1, 0);
}

MDSInstrSelector *
mdsCreateSelector (const MDSMachine *machine)
{
  MDSInstrSelector *selector;

  if (machine == NULL)
    {
      return NULL;
    }
  selector = (MDSInstrSelector *) calloc (1u, sizeof (*selector));
  if (selector == NULL)
    {
      return NULL;
    }
  selector->machine = machine;
  selector->patternsByOpcode
      = (MDSPattern **) calloc ((size_t) MAL_OP_TARGET + 1u,
                                sizeof (*selector->patternsByOpcode));
  if (selector->patternsByOpcode == NULL)
    {
      free (selector);
      return NULL;
    }
  if (!mdsLoadPatterns (selector, NULL))
    {
      mdsDestroySelector (selector);
      return NULL;
    }
  return selector;
}

void
mdsDestroySelector (MDSInstrSelector *selector)
{
  uint32_t index;

  if (selector == NULL)
    {
      return;
    }
  for (index = 0u; index < selector->patternCount; ++index)
    {
      if (selector->patterns[index].root != NULL)
        {
          free (selector->patterns[index].root->operands);
        }
      free (selector->patterns[index].root);
      free (selector->patterns[index].emitInstrs);
    }
  free (selector->patterns);
  free (selector->patternsByOpcode);
  free (selector);
}

int
mdsLoadPatterns (MDSInstrSelector *selector, const char *path)
{
  (void) path;
  if (selector == NULL)
    {
      return 0;
    }
  return mds_add_builtin_pattern (selector, MAL_OP_COPY, "mov.rr", 1, 0)
         && mds_add_builtin_pattern (selector, MAL_OP_COPY, "mov.ri", 0, 1)
         && mds_add_builtin_pattern (selector, MAL_OP_ADD, "add.rr", 1, 0)
         && mds_add_builtin_pattern (selector, MAL_OP_ADD, "add.ri", 1, 1)
         && mds_add_builtin_pattern (selector, MAL_OP_SUB, "sub.rr", 1, 0)
         && mds_add_builtin_pattern (selector, MAL_OP_SUB, "sub.ri", 1, 1)
         && mds_add_builtin_pattern (selector, MAL_OP_MUL, "mul.rr", 1, 0)
         && mds_add_builtin_pattern (selector, MAL_OP_NEG, "neg", 1, 0)
         && mds_add_builtin_pattern (selector, MAL_OP_AND, "and.rr", 1, 0)
         && mds_add_builtin_pattern (selector, MAL_OP_OR, "or.rr", 1, 0)
         && mds_add_builtin_pattern (selector, MAL_OP_XOR, "xor.rr", 1, 0)
         && mds_add_builtin_pattern (selector, MAL_OP_NOT, "not", 1, 0)
         && mds_add_builtin_pattern (selector, MAL_OP_SHL, "shl.ri", 1, 1)
         && mds_add_builtin_pattern (selector, MAL_OP_ASHR, "ashr.ri", 1, 1)
         && mds_add_builtin_pattern (selector, MAL_OP_ICMP_EQ, "icmp.eq", 1, 0)
         && mds_add_builtin_pattern (selector, MAL_OP_ICMP_NE, "icmp.ne", 1, 0)
         && mds_add_builtin_pattern (selector, MAL_OP_ICMP_SLT, "icmp.slt", 1, 0)
         && mds_add_builtin_pattern (selector, MAL_OP_ICMP_SLE, "icmp.sle", 1, 0)
         && mds_add_builtin_pattern (selector, MAL_OP_ICMP_SGT, "icmp.sgt", 1, 0)
         && mds_add_builtin_pattern (selector, MAL_OP_ICMP_SGE, "icmp.sge", 1, 0)
         && mds_add_ret_pattern (selector, "ret");
}

const MDSPattern *
mdsMatchPattern (const MDSInstrSelector *selector, const MALInst *inst)
{
  uint32_t index;

  if (selector == NULL || inst == NULL)
    {
      return NULL;
    }
  for (index = 0u; index < selector->patternCount; ++index)
    {
      if (mds_pattern_matches_inst (&selector->patterns[index], inst))
        {
          return &selector->patterns[index];
        }
    }
  return NULL;
}

const MDSPattern *
mdsSelectBestPattern (const MDSInstrSelector *selector, const MALInst *inst)
{
  const MDSPattern *best = NULL;
  uint32_t index;

  if (selector == NULL || inst == NULL)
    {
      return NULL;
    }
  for (index = 0u; index < selector->patternCount; ++index)
    {
      const MDSPattern *pattern = &selector->patterns[index];
      if (!mds_pattern_matches_inst (pattern, inst))
        {
          continue;
        }
      if (best == NULL || pattern->cost < best->cost)
        {
          best = pattern;
        }
    }
  return best;
}
