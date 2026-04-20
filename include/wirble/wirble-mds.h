#ifndef WIRBLE_MDS_H
#define WIRBLE_MDS_H

#include <stddef.h>
#include <stdint.h>

#include "api-boilerplate.h"

WIRBLE_BEGIN_DECLS

/* Forward declarations */
typedef struct WRSSExpr WRSSExpr;
typedef struct MALModule MALModule;
typedef struct MALFunction MALFunction;
typedef struct MALInst MALInst;
typedef struct MALOperand MALOperand;
/* ════════════════════════════════════════════════════════════════════
 *  §1  BASIC TYPES
 * ════════════════════════════════════════════════════════════════════ */

typedef uint32_t MDSRegId;
typedef uint32_t MDSInstrId;
typedef uint32_t MDSIndex;
typedef uint32_t MDSPatternId;

typedef enum MDSTargetKind
{
  MDS_TARGET_GENERIC = 0,
  MDS_TARGET_X86_64,
  MDS_TARGET_AARCH64,
  MDS_TARGET_WASM
} MDSTargetKind;
/* ════════════════════════════════════════════════════════════════════
 *  §2  REGISTER MODEL
 * ════════════════════════════════════════════════════════════════════ */

typedef enum MDSRegisterClass
{
  MDS_REGCLASS_GPR = 0,
  MDS_REGCLASS_FP,
  MDS_REGCLASS_VECTOR,
  MDS_REGCLASS_SPECIAL,
  MDS_REGCLASS__COUNT
} MDSRegisterClass;
typedef struct MDSRegister
{
  MDSRegId id;
  const char *name; /* e.g., "rax", "xmm0", "v0" */
  MDSRegisterClass regClass;
  uint32_t bitWidth;
  int allocatable; /* Can be used by RA */

  /* Register aliasing (e.g., al/ax/eax/rax on x86) */
  MDSRegId *aliases;
  uint32_t aliasCount;
  /* Sub-register relationships */
  MDSRegId parentReg;    /* 0 if none */
  uint32_t subRegOffset; /* Bit offset within parent */

  /* Calling convention info */
  int isCallerSaved;
  int isCalleeSaved;
  int isArgumentReg;
  int isReturnReg;
  uint32_t argumentIndex; /* If isArgumentReg */
} MDSRegister;
/* ════════════════════════════════════════════════════════════════════
 *  §3  OPERAND MODEL
 * ════════════════════════════════════════════════════════════════════ */

typedef enum MDSOperandType
{
  MDS_OPERAND_REG,
  MDS_OPERAND_IMM,
  MDS_OPERAND_MEM,
  MDS_OPERAND_LABEL
} MDSOperandType;
typedef struct MDSOperandDesc
{
  const char *name; /* e.g., "$dst", "$src1" */
  MDSOperandType type;
  MDSRegisterClass regClass; /* If type == REG */
  uint32_t bitWidth;
  int isInput;
  int isOutput;
  int isEarlyClobber; /* Output written before inputs read */
  int isTiedTo;       /* Index of tied operand, -1 if none */

  /* Immediate constraints */
  int64_t immMin;
  int64_t immMax;
  /* Memory addressing mode constraints */
  int allowsBaseReg;
  int allowsIndexReg;
  int allowsDisplacement;
  int allowsScale;
} MDSOperandDesc;
/* ════════════════════════════════════════════════════════════════════
 *  §4  INSTRUCTION ENCODING
 * ════════════════════════════════════════════════════════════════════ */

typedef struct MDSEncoding
{ /* Fixed encoding bytes (opcode, prefixes, etc.) */
  uint8_t *fixedBytes;
  uint32_t fixedLength;
  /* Encoding template with placeholders for operands */
  const char *template; /* e.g., "48 8B /r" for x86 */

  /* Operand encoding positions */
  struct
  {
    uint32_t operandIndex; /* Which operand */
    uint32_t byteOffset;   /* Where in encoding */
    uint32_t bitOffset;
    uint32_t bitWidth;
  } *operandEncodings;
  uint32_t operandEncodingCount;
  /* Variable-length encoding info */
  uint32_t minLength;
  uint32_t maxLength;
} MDSEncoding;
/* ════════════════════════════════════════════════════════════════════
 *  §5  INSTRUCTION DESCRIPTOR
 * ════════════════════════════════════════════════════════════════════ */

typedef struct MDSInstruction
{
  MDSInstrId id;
  const char *mnemonic;    /* e.g., "add", "mov", "vaddps" */
  const char *asmTemplate; /* e.g., "add %0, %1, %2" */

  /* Operands */
  MDSOperandDesc *operands;
  uint32_t operndCount;
  /* Encoding */

  MDSEncoding encoding;
  /* Instruction properties */

  int isBranch;
  int isCall;
  int isReturn;
  int isLoad;
  int isStore;
  int isBarrier;
  int mayTrap;
  /* Cost model (from schema) */

  int selectionCost;
  int latency;
  int throughput;
  /* Scheduling constraints */

  uint32_t pipelineClass;
} MDSInstruction;
/* ════════════════════════════════════════════════════════════════════

    §6 FP / VECTOR EXTENSION SUITES
    ════════════════════════════════════════════════════════════════════ */

typedef struct MDSFPSuite
{
  const char *name;
  const char **formats;
  uint32_t formatCount;
} MDSFPSuite;
typedef struct MDSVectorSuite
{
  const char *name;
  uint32_t vectorWidth;
  const char **elementTypes;
  uint32_t elementTypeCount;
} MDSVectorSuite;
/* ════════════════════════════════════════════════════════════════════

    §7 MACHINE MODEL
    ════════════════════════════════════════════════════════════════════ */

typedef enum MDSEndianness
{
  MDS_ENDIAN_LITTLE,
  MDS_ENDIAN_BIG

} MDSEndianness;
typedef struct MDSMachine
{
  MDSTargetKind targetKind;
  const char *name;
  const char *vendor;
  MDSEndianness endianness;
  uint32_t pointerSize;
  /* Register file */

  MDSRegister *registers;
  uint32_t registerCount;
  /* Instruction set */

  MDSInstruction *instructions;
  uint32_t instructionCount;
  /* Lookup tables for fast access */

  MDSInstruction **instrById;
  /* Extension suites */

  MDSFPSuite *fpSuites;
  uint32_t fpSuiteCount;
  MDSVectorSuite *vectorSuites;
  uint32_t vectorSuiteCount;
} MDSMachine;
/* ════════════════════════════════════════════════════════════════════

    §8 MACHINE SPEC LOADING
    ════════════════════════════════════════════════════════════════════ */

typedef enum MDSSpecFormat
{
  MDS_FORMAT_JSON,
  MDS_FORMAT_YAML,
  MDS_FORMAT_XML

} MDSSpecFormat;
MDSMachine *mdsLoadMachine (const char *path, MDSSpecFormat format);
void mdsDestroyMachine (MDSMachine *machine);
int mdsValidateMachine (const MDSMachine *machine);
/* ════════════════════════════════════════════════════════════════════

    §9 INSTRUCTION SELECTION PATTERN GRAPH
    ════════════════════════════════════════════════════════════════════ */

typedef struct MDSPatternNode
{
  /* MAL opcode constraint */

  uint32_t malOpcode;
  /* Operand constraints */

  struct
  {
    int mustBeConst;
    int mustBeReg;
    int mustBeImm;
    int bitWidth;
  } *operands;
  uint32_t operandCount;
  /* DAG edges */

  struct MDSPatternNode **inputs;
  uint32_t inputCount;
} MDSPatternNode;
typedef struct MDSPattern
{
  MDSPatternId id;
  /* Pattern DAG root */

  MDSPatternNode *root;
  /* Emitted instructions */

  MDSInstrId *emitInstrs;
  uint32_t emitInstrCount;
  /* Cost override */

  int cost;
} MDSPattern;
typedef struct MDSInstrSelector
{
  const MDSMachine *machine;
  MDSPattern *patterns;
  uint32_t patternCount;
  /* Fast opcode dispatch */

  MDSPattern **patternsByOpcode;
} MDSInstrSelector;
/* Pattern management */

MDSInstrSelector *mdsCreateSelector (const MDSMachine *machine);
void mdsDestroySelector (MDSInstrSelector *selector);
int mdsLoadPatterns (MDSInstrSelector *selector, const char *path);
void mdsAddPattern (MDSInstrSelector *selector, MDSPattern *pattern);
/* Pattern matching */

const MDSPattern *mdsMatchPattern (const MDSInstrSelector *selector,
                                   const MALInst *inst);
const MDSPattern *mdsSelectBestPattern (const MDSInstrSelector *selector,
                                        const MALInst *inst);
/* ════════════════════════════════════════════════════════════════════

    §10 LOWERING MAL → MDS
    ════════════════════════════════════════════════════════════════════ */

typedef struct MDSProgram MDSProgram;
typedef struct MDSBasicBlock MDSBasicBlock;
MDSProgram *mdsLowerFromMAL (const MALModule *module,
                             const MDSInstrSelector *selector,
                             const MDSMachine *machine);
void mdsDestroyProgram (MDSProgram *program);
/* ════════════════════════════════════════════════════════════════════

    §11 MDS PROGRAM
    ════════════════════════════════════════════════════════════════════ */

typedef struct MDSInst
{
  const MDSInstruction *desc;
  /* Physical operands */

  MALOperand *operands;
  uint32_t operandCount;
} MDSInst;

struct MDSBasicBlock
{
  uint32_t id;
  MDSInst *instructions;
  uint32_t instrCount;
  uint32_t *successors;
  uint32_t successorCount;
};

struct MDSProgram
{
  const MDSMachine *machine;
  MDSBasicBlock *blocks;
  uint32_t blockCount;
};
/* ════════════════════════════════════════════════════════════════════

    §12 CODE GENERATION
    ════════════════════════════════════════════════════════════════════ */

int mdsEmitAssembly (const MDSProgram *program, const char *path);
int mdsEmitBinary (const MDSProgram *program, const char *path);
unsigned char *mdsEmitBuffer (const MDSProgram *program, size_t *outSize);
/* ════════════════════════════════════════════════════════════════════

    §13 ENCODING / DECODING
    ════════════════════════════════════════════════════════════════════ */

unsigned char *mdsEncodeInstruction (const MDSInstruction *instr,
                                     const MALOperand *operands,
                                     uint32_t operandCount, size_t *outSize);
const MDSInstruction *mdsDecodeInstruction (const MDSMachine *machine,
                                            const unsigned char *bytes,
                                            size_t length);
/* ════════════════════════════════════════════════════════════════════

    §14 DEBUGGING
    ════════════════════════════════════════════════════════════════════ */

void mdsPrintMachine (const MDSMachine *machine);
void mdsPrintInstruction (const MDSInstruction *instr);
void mdsPrintProgram (const MDSProgram *program);
const char *mdsTargetKindName (MDSTargetKind kind);
WIRBLE_END_DECLS

#endif
