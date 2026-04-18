#ifndef WIRBLE_MDS_H
#define WIRBLE_MDS_H

#include "api-boilerplate.h"
#include "wirble-mal.h"

WIRBLE_BEGIN_DECLS

/* ------------------------------------------------------------
   Basic Types
   ------------------------------------------------------------ */

typedef unsigned long MDSRegId;
typedef unsigned long MDSInstrId;
typedef unsigned long MDSIndex;


/* ------------------------------------------------------------
   MDS Register Class
   ------------------------------------------------------------ */

typedef enum
{
  MDS_REG_CLASS_GPR,    /* General purpose registers */
  MDS_REG_CLASS_FP,     /* Floating point registers */
  MDS_REG_CLASS_VECTOR, /* Vector registers */
  MDS_REG_CLASS_SPECIAL /* Special purpose (flags, PC, etc.) */
} MDSRegisterClass;

typedef struct MDSRegister
{
  MDSRegId id;
  const char *name; /* e.g., "rax", "xmm0", "v0" */
  MDSRegisterClass regClass;
  unsigned int bitWidth;
  int isAllocatable; /* Can be used by register allocator */
} MDSRegister;

/* ------------------------------------------------------------
   MDS Instruction Descriptor
   ------------------------------------------------------------ */

typedef struct MDSOperand
{
  const char *name;
  MDSRegisterClass regClass;
  int isInput;
  int isOutput;
  int isImmediate;
} MDSOperand;

typedef struct MDSInstruction
{
  MDSInstrId id;
  const char *mnemonic;    /* e.g., "add", "mov", "vaddps" */
  const char *asmTemplate; /* e.g., "add %0, %1, %2" */

  MDSOperand *operands;
  MDSIndex operandCount;

  /* Binary encoding information */
  unsigned char *encodingBytes;
  MDSIndex encodingLength;

  /* Instruction selection cost (for pattern matching) */
  int selectionCost;

  /* Scheduling information */
  int latency;
  int throughput;

} MDSInstruction;

/* ------------------------------------------------------------
   MDS Machine Model
   ------------------------------------------------------------ */

typedef struct MDSMachine
{
  const char *name; /* e.g., "x86_64", "aarch64", "wasm" */
  const char *vendor;

  MDSRegister **registers;
  MDSIndex registerCount;

  MDSInstruction **instructions;
  MDSIndex instructionCount;

  /* Floating point suites */
  MALFloatingPointInterface **fpSuites;
  MDSIndex fpSuiteCount;

  /* Vector extension suites */
  MALVectorInterface **vectorSuites;
  MDSIndex vectorSuiteCount;

  /* Endianness */
  int isLittleEndian;

  /* Pointer size in bytes */
  unsigned int pointerSize;

} MDSMachine;

/* ------------------------------------------------------------
   Machine Specification Loading (JSON/YAML/XML)
   ------------------------------------------------------------ */

typedef enum
{
  MDS_FORMAT_JSON,
  MDS_FORMAT_YAML,
  MDS_FORMAT_XML
} MDSSpecFormat;

/* Load machine specification from file */
MDSMachine *mdsLoadMachine (const char *filepath, MDSSpecFormat format);

/* Destroy machine model */
void mdsDestroyMachine (MDSMachine *machine);

/* Validate machine specification */
int mdsValidateMachine (const MDSMachine *machine);

/* ------------------------------------------------------------
   Instruction Selection Pattern System
   ------------------------------------------------------------ */

typedef struct MDSPattern MDSPattern;
typedef struct MDSPatternRule MDSPatternRule;
typedef struct MDSInstrSelector MDSInstrSelector;

/* Create instruction selector for a machine */
MDSInstrSelector *mdsCreateSelector (const MDSMachine *machine);
void mdsDestroySelector (MDSInstrSelector *selector);

/* Load instruction selection patterns from file */
/* Pattern files are half-MAL (input), half-MDS (output) */
int mdsLoadPatterns (MDSInstrSelector *selector, const char *filepath);

/* Add a single pattern rule programmatically */
void mdsAddPattern (MDSInstrSelector *selector, MDSPatternRule *rule);

/* ------------------------------------------------------------
   MAL to MDS Lowering (Instruction Selection)
   ------------------------------------------------------------ */

typedef struct MDSProgram MDSProgram;

/* Lower MAL unit to MDS program using instruction selection */
MDSProgram *mdsLowerFromMAL (const MALUnit *malUnit,
                             MDSInstrSelector *selector,
                             const MDSMachine *machine);

/* Destroy MDS program */
void mdsDestroyProgram (MDSProgram *program);

/* ------------------------------------------------------------
   MDS Program Representation
   ------------------------------------------------------------ */

typedef struct MDSBasicBlock
{
  MDSIndex id;
  MDSInstruction **instructions;
  MDSIndex instrCount;

  MDSIndex *successors;
  MDSIndex successorCount;
} MDSBasicBlock;

struct MDSProgram
{
  MDSBasicBlock **blocks;
  MDSIndex blockCount;

  const MDSMachine *machine;

  /* Symbol table for labels, functions, etc. */
  void *symbolTable;
};

/* ------------------------------------------------------------
   Post-Lowering Optimization
   ------------------------------------------------------------ */

/* Peephole optimization on MDS program */
void mdsOptimizePeephole (MDSProgram *program);

/* Dead code elimination */
void mdsOptimizeDCE (MDSProgram *program);

/* Instruction scheduling */
void mdsScheduleInstructions (MDSProgram *program);

/* Register allocation (final physical register assignment) */
void mdsAllocateRegisters (MDSProgram *program);

/* ------------------------------------------------------------
   Code Emission (Assembly and Binary)
   ------------------------------------------------------------ */

/* Emit assembly code to file */
int mdsEmitAssembly (const MDSProgram *program, const char *filepath);

/* Emit binary machine code to file */
int mdsEmitBinary (const MDSProgram *program, const char *filepath);

/* Emit to memory buffer (for JIT compilation) */
unsigned char *mdsEmitToBuffer (const MDSProgram *program, size_t *outSize);

/* ------------------------------------------------------------
   Serialization and Deserialization
   ------------------------------------------------------------ */

/* Serialize MDS program to JSON/YAML/XML */
int mdsSerializeProgram (const MDSProgram *program, const char *filepath,
                         MDSSpecFormat format);

/* Deserialize MDS program from JSON/YAML/XML */
MDSProgram *mdsDeserializeProgram (const char *filepath, MDSSpecFormat format,
                                   const MDSMachine *machine);

/* ------------------------------------------------------------
   Instruction Encoding Utilities
   ------------------------------------------------------------ */

/* Encode a single instruction to bytes */
unsigned char *mdsEncodeInstruction (const MDSInstruction *instr,
                                     const MDSOperand *operands,
                                     MDSIndex operandCount, size_t *outSize);

/* Decode bytes to instruction (for disassembly) */
MDSInstruction *mdsDecodeInstruction (const MDSMachine *machine,
                                      const unsigned char *bytes,
                                      size_t length);

/* ------------------------------------------------------------
   Register Allocation Interface
   ------------------------------------------------------------ */

typedef struct MDSRegisterAllocator MDSRegisterAllocator;

/* Create register allocator for a machine */
MDSRegisterAllocator *mdsCreateAllocator (const MDSMachine *machine);
void mdsDestroyAllocator (MDSRegisterAllocator *allocator);

/* Perform register allocation on program */
void mdsRunAllocation (MDSRegisterAllocator *allocator, MDSProgram *program);

/* Query allocation results */
MDSRegId mdsGetPhysicalReg (const MDSRegisterAllocator *allocator,
                            MALReg virtualReg);

/* ------------------------------------------------------------
   Instruction Selection Cost Model
   ------------------------------------------------------------ */

/* Compute cost of selecting a pattern */
int mdsPatternCost (const MDSPattern *pattern, const MALInstr *instr);

/* Find best pattern match for MAL instruction */
const MDSPatternRule *mdsFindBestPattern (const MDSInstrSelector *selector,
                                          const MALInstr *instr);

/* ------------------------------------------------------------
   Debugging and Introspection
   ------------------------------------------------------------ */

/* Print machine specification */
void mdsPrintMachine (const MDSMachine *machine);

/* Print instruction descriptor */
void mdsPrintInstruction (const MDSInstruction *instr);

/* Print register descriptor */
void mdsPrintRegister (const MDSRegister *reg);

/* Print MDS program (assembly-like format) */
void mdsPrintProgram (const MDSProgram *program);

/* Print instruction selection patterns */
void mdsPrintPatterns (const MDSInstrSelector *selector);

/* Disassemble binary to MDS program */
MDSProgram *mdsDisassemble (const unsigned char *bytes, size_t length,
                            const MDSMachine *machine);

/* ------------------------------------------------------------
   Virtual Machine Support (e.g., WASM)
   ------------------------------------------------------------ */

/* Check if machine is virtual */
int mdsIsVirtualMachine (const MDSMachine *machine);

/* Emit to virtual machine bytecode format */
int mdsEmitVMBytecode (const MDSProgram *program, const char *filepath);

WIRBLE_BEGIN_DECLS

#endif /* WIRBLE_MDS_H */
