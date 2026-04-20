#include <wirble/wirble-mds.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef MAL_INVALID_REG
#define MAL_INVALID_REG ((uint32_t) -1)
#endif

static char *
mds_strdup (const char *text)
{
  size_t len;
  char *copy;

  if (text == NULL)
    {
      return NULL;
    }
  len = strlen (text) + 1u;
  copy = (char *) malloc (len);
  if (copy != NULL)
    {
      memcpy (copy, text, len);
    }
  return copy;
}

static const char *
mds_basename (const char *path)
{
  const char *slash;

  if (path == NULL)
    {
      return "generic";
    }
  slash = strrchr (path, '/');
  return slash == NULL ? path : slash + 1;
}

static int
mds_contains_token (const char *text, const char *token)
{
  size_t i;
  size_t j;
  size_t textLen;
  size_t tokenLen;

  if (text == NULL || token == NULL)
    {
      return 0;
    }
  textLen = strlen (text);
  tokenLen = strlen (token);
  if (tokenLen == 0u || tokenLen > textLen)
    {
      return 0;
    }
  for (i = 0u; i + tokenLen <= textLen; ++i)
    {
      for (j = 0u; j < tokenLen; ++j)
        {
          if ((char) tolower ((unsigned char) text[i + j])
              != (char) tolower ((unsigned char) token[j]))
            {
              break;
            }
        }
      if (j == tokenLen)
        {
          return 1;
        }
    }
  return 0;
}

static MDSTargetKind
mds_target_kind_from_name (const char *name)
{
  const char *base = mds_basename (name);

  if (mds_contains_token (base, "x86_64") || mds_contains_token (base, "amd64")
      || mds_contains_token (base, "x64"))
    {
      return MDS_TARGET_X86_64;
    }
  if (mds_contains_token (base, "aarch64") || mds_contains_token (base, "arm64"))
    {
      return MDS_TARGET_AARCH64;
    }
  if (mds_contains_token (base, "wasm"))
    {
      return MDS_TARGET_WASM;
    }
  return MDS_TARGET_GENERIC;
}

const char *
mdsTargetKindName (MDSTargetKind kind)
{
  switch (kind)
    {
    case MDS_TARGET_X86_64: return "x86_64";
    case MDS_TARGET_AARCH64: return "aarch64";
    case MDS_TARGET_WASM: return "wasm";
    case MDS_TARGET_GENERIC:
    default:
      return "generic";
    }
}

const MDSInstruction *
mds_find_instruction_by_mnemonic (const MDSMachine *machine, const char *mnemonic)
{
  uint32_t index;

  if (machine == NULL || mnemonic == NULL)
    {
      return NULL;
    }
  for (index = 0u; index < machine->instructionCount; ++index)
    {
      const MDSInstruction *instr = &machine->instructions[index];
      if (instr->mnemonic != NULL && strcmp (instr->mnemonic, mnemonic) == 0)
        {
          return instr;
        }
    }
  return NULL;
}

static MDSOperandDesc
mds_make_reg_operand (const char *name, int isInput, int isOutput)
{
  MDSOperandDesc operand;
  memset (&operand, 0, sizeof (operand));
  operand.name = name;
  operand.type = MDS_OPERAND_REG;
  operand.regClass = MDS_REGCLASS_GPR;
  operand.bitWidth = 64u;
  operand.isInput = isInput;
  operand.isOutput = isOutput;
  operand.isTiedTo = -1;
  return operand;
}

static MDSOperandDesc
mds_make_imm_operand (const char *name)
{
  MDSOperandDesc operand;
  memset (&operand, 0, sizeof (operand));
  operand.name = name;
  operand.type = MDS_OPERAND_IMM;
  operand.bitWidth = 64u;
  operand.isInput = 1;
  operand.isTiedTo = -1;
  operand.immMin = INT64_MIN;
  operand.immMax = INT64_MAX;
  return operand;
}

static int
mds_init_instruction (MDSInstruction *instr, uint32_t id, const char *mnemonic,
                      const char *asmTemplate, const MDSOperandDesc *operands,
                      uint32_t operandCount)
{
  uint32_t index;

  memset (instr, 0, sizeof (*instr));
  instr->id = id;
  instr->mnemonic = mds_strdup (mnemonic);
  instr->asmTemplate = mds_strdup (asmTemplate);
  instr->selectionCost = 1;
  instr->latency = 1;
  instr->throughput = 1;
  instr->operndCount = operandCount;
  instr->operands
      = operandCount == 0u ? NULL
                           : (MDSOperandDesc *) calloc (operandCount,
                                                        sizeof (*instr->operands));
  if ((operandCount != 0u && instr->operands == NULL) || instr->mnemonic == NULL
      || instr->asmTemplate == NULL)
    {
      return 0;
    }
  for (index = 0u; index < operandCount; ++index)
    {
      instr->operands[index] = operands[index];
      instr->operands[index].name = mds_strdup (operands[index].name);
      if (instr->operands[index].name == NULL)
        {
          return 0;
        }
    }
  instr->encoding.fixedLength = 1u;
  instr->encoding.minLength = 1u;
  instr->encoding.maxLength = 1u;
  instr->encoding.fixedBytes = (uint8_t *) malloc (1u);
  if (instr->encoding.fixedBytes == NULL)
    {
      return 0;
    }
  instr->encoding.fixedBytes[0] = (uint8_t) id;
  return 1;
}

static void
mds_free_instruction_members (MDSInstruction *instr)
{
  uint32_t index;

  if (instr == NULL)
    {
      return;
    }
  for (index = 0u; index < instr->operndCount; ++index)
    {
      free ((char *) instr->operands[index].name);
    }
  free (instr->operands);
  free ((char *) instr->mnemonic);
  free ((char *) instr->asmTemplate);
  free (instr->encoding.fixedBytes);
  free ((char *) instr->encoding.template);
  free (instr->encoding.operandEncodings);
}

static int
mds_build_builtin_registers (MDSMachine *machine)
{
  static const struct
  {
    MDSRegId id;
    const char *name;
    MDSRegisterClass regClass;
    uint32_t bitWidth;
    int allocatable;
  } genericRegs[] = {
    { 0u, "r0", MDS_REGCLASS_GPR, 64u, 1 },
    { 1u, "r1", MDS_REGCLASS_GPR, 64u, 1 },
    { 2u, "r2", MDS_REGCLASS_GPR, 64u, 1 },
    { 3u, "r3", MDS_REGCLASS_GPR, 64u, 1 },
    { 4u, "sp", MDS_REGCLASS_SPECIAL, 64u, 0 },
  };
  uint32_t index;

  machine->registerCount = (uint32_t) (sizeof (genericRegs) / sizeof (genericRegs[0]));
  machine->registers = (MDSRegister *) calloc (machine->registerCount,
                                               sizeof (*machine->registers));
  if (machine->registers == NULL)
    {
      return 0;
    }
  for (index = 0u; index < machine->registerCount; ++index)
    {
      MDSRegister *reg = &machine->registers[index];
      reg->id = genericRegs[index].id;
      reg->name = mds_strdup (genericRegs[index].name);
      reg->regClass = genericRegs[index].regClass;
      reg->bitWidth = genericRegs[index].bitWidth;
      reg->allocatable = genericRegs[index].allocatable;
      reg->parentReg = 0u;
      if (reg->name == NULL)
        {
          return 0;
        }
    }
  return 1;
}

static int
mds_build_builtin_instructions (MDSMachine *machine)
{
  const char *prefix;
  const char *asmPrefix;
  MDSOperandDesc binaryReg[] = { { 0 }, { 0 } };
  MDSOperandDesc binaryImm[] = { { 0 }, { 0 } };
  MDSOperandDesc ternaryReg[] = { { 0 }, { 0 }, { 0 } };
  MDSOperandDesc ternaryImm[] = { { 0 }, { 0 }, { 0 } };
  MDSOperandDesc retOps[] = { { 0 } };
  char mnemonic[64];
  char asmTemplate[80];
  uint32_t index = 0u;

  switch (machine->targetKind)
    {
    case MDS_TARGET_X86_64:
      prefix = "x64";
      asmPrefix = "x86_64";
      break;
    case MDS_TARGET_AARCH64:
      prefix = "a64";
      asmPrefix = "aarch64";
      break;
    case MDS_TARGET_WASM:
      prefix = "wasm";
      asmPrefix = "wasm";
      break;
    case MDS_TARGET_GENERIC:
    default:
      prefix = "generic";
      asmPrefix = "generic";
      break;
    }

  machine->instructionCount = 21u;
  machine->instructions = (MDSInstruction *) calloc (machine->instructionCount,
                                                     sizeof (*machine->instructions));
  machine->instrById = (MDSInstruction **) calloc (machine->instructionCount,
                                                   sizeof (*machine->instrById));
  if (machine->instructions == NULL || machine->instrById == NULL)
    {
      return 0;
    }

  binaryReg[0] = mds_make_reg_operand ("dst", 0, 1);
  binaryReg[1] = mds_make_reg_operand ("src", 1, 0);
  binaryImm[0] = mds_make_reg_operand ("dst", 0, 1);
  binaryImm[1] = mds_make_imm_operand ("imm");
  ternaryReg[0] = mds_make_reg_operand ("dst", 0, 1);
  ternaryReg[1] = mds_make_reg_operand ("lhs", 1, 0);
  ternaryReg[2] = mds_make_reg_operand ("rhs", 1, 0);
  ternaryImm[0] = mds_make_reg_operand ("dst", 0, 1);
  ternaryImm[1] = mds_make_reg_operand ("lhs", 1, 0);
  ternaryImm[2] = mds_make_imm_operand ("imm");
  retOps[0] = mds_make_reg_operand ("value", 1, 0);

#define MDS_INIT_INSTR(NAME, OPS, COUNT, TEMPLATE)                               \
  do                                                                             \
    {                                                                            \
      snprintf (mnemonic, sizeof (mnemonic), "%s.%s", prefix, (NAME));         \
      snprintf (asmTemplate, sizeof (asmTemplate), "%s %s", asmPrefix,         \
                (TEMPLATE));                                                     \
      if (!mds_init_instruction (&machine->instructions[index], index, mnemonic, \
                                 asmTemplate, (OPS), (COUNT)))                   \
        {                                                                        \
          return 0;                                                              \
        }                                                                        \
      machine->instrById[index] = &machine->instructions[index];                 \
      ++index;                                                                   \
    }                                                                            \
  while (0)

  MDS_INIT_INSTR ("mov.rr", binaryReg, 2u, "mov %0, %1");
  MDS_INIT_INSTR ("mov.ri", binaryImm, 2u, "mov %0, %1");
  MDS_INIT_INSTR ("add.rr", ternaryReg, 3u, "add %0, %1, %2");
  MDS_INIT_INSTR ("add.ri", ternaryImm, 3u, "add %0, %1, %2");
  MDS_INIT_INSTR ("sub.rr", ternaryReg, 3u, "sub %0, %1, %2");
  MDS_INIT_INSTR ("sub.ri", ternaryImm, 3u, "sub %0, %1, %2");
  MDS_INIT_INSTR ("mul.rr", ternaryReg, 3u, "mul %0, %1, %2");
  MDS_INIT_INSTR ("neg", binaryReg, 2u, "neg %0, %1");
  MDS_INIT_INSTR ("and.rr", ternaryReg, 3u, "and %0, %1, %2");
  MDS_INIT_INSTR ("or.rr", ternaryReg, 3u, "or %0, %1, %2");
  MDS_INIT_INSTR ("xor.rr", ternaryReg, 3u, "xor %0, %1, %2");
  MDS_INIT_INSTR ("not", binaryReg, 2u, "not %0, %1");
  MDS_INIT_INSTR ("shl.ri", ternaryImm, 3u, "shl %0, %1, %2");
  MDS_INIT_INSTR ("ashr.ri", ternaryImm, 3u, "ashr %0, %1, %2");
  MDS_INIT_INSTR ("icmp.eq", ternaryReg, 3u, "icmp.eq %0, %1, %2");
  MDS_INIT_INSTR ("icmp.ne", ternaryReg, 3u, "icmp.ne %0, %1, %2");
  MDS_INIT_INSTR ("icmp.slt", ternaryReg, 3u, "icmp.slt %0, %1, %2");
  MDS_INIT_INSTR ("icmp.sle", ternaryReg, 3u, "icmp.sle %0, %1, %2");
  MDS_INIT_INSTR ("icmp.sgt", ternaryReg, 3u, "icmp.sgt %0, %1, %2");
  MDS_INIT_INSTR ("icmp.sge", ternaryReg, 3u, "icmp.sge %0, %1, %2");
  MDS_INIT_INSTR ("ret", retOps, 1u, "ret %0");
#undef MDS_INIT_INSTR

  return index == machine->instructionCount;
}

MDSMachine *
mdsLoadMachine (const char *path, MDSSpecFormat format)
{
  MDSMachine *machine;

  (void) format;
  machine = (MDSMachine *) calloc (1u, sizeof (*machine));
  if (machine == NULL)
    {
      return NULL;
    }
  machine->targetKind = mds_target_kind_from_name (path);
  machine->name = mds_strdup (mdsTargetKindName (machine->targetKind));
  machine->vendor = mds_strdup (machine->targetKind == MDS_TARGET_AARCH64
                                    ? "ARM"
                                    : machine->targetKind == MDS_TARGET_WASM ? "W3C"
                                                                              : "Generic");
  machine->endianness = MDS_ENDIAN_LITTLE;
  machine->pointerSize = machine->targetKind == MDS_TARGET_WASM ? 4u : 8u;
  if (machine->name == NULL || machine->vendor == NULL
      || !mds_build_builtin_registers (machine)
      || !mds_build_builtin_instructions (machine))
    {
      mdsDestroyMachine (machine);
      return NULL;
    }
  return machine;
}

void
mdsDestroyMachine (MDSMachine *machine)
{
  uint32_t index;

  if (machine == NULL)
    {
      return;
    }
  for (index = 0u; index < machine->registerCount; ++index)
    {
      free ((char *) machine->registers[index].name);
      free (machine->registers[index].aliases);
    }
  for (index = 0u; index < machine->instructionCount; ++index)
    {
      mds_free_instruction_members (&machine->instructions[index]);
    }
  free (machine->registers);
  free (machine->instructions);
  free (machine->instrById);
  free (machine->fpSuites);
  free (machine->vectorSuites);
  free ((char *) machine->name);
  free ((char *) machine->vendor);
  free (machine);
}

int
mdsValidateMachine (const MDSMachine *machine)
{
  uint32_t index;

  if (machine == NULL || machine->name == NULL || machine->instructions == NULL
      || machine->instructionCount == 0u)
    {
      return 0;
    }
  for (index = 0u; index < machine->instructionCount; ++index)
    {
      const MDSInstruction *instr = &machine->instructions[index];
      if (instr->mnemonic == NULL || instr->asmTemplate == NULL)
        {
          return 0;
        }
    }
  return 1;
}
