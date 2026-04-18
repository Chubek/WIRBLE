#ifndef WIRBLE_MAL_H
#define WIRBLE_MAL_H

#include "api-boilerplate.h"
#include "wirble-wil.h" /* WIL Sea-of-Nodes IR */
#include "wirble-wrs.h" /* WIL rewrite system for pre-lowering optimizations */

WIRBLE_BEGIN_DECLS

/* ------------------------------------------------------------
   Basic Types
   ------------------------------------------------------------ */

typedef unsigned long MALInstrId;
typedef unsigned long MALReg; /* Infinite virtual registers */
typedef unsigned long MALBlockId;
typedef unsigned long MALIndex;

/* ------------------------------------------------------------
   MAL Instruction Kinds
   ------------------------------------------------------------ */

typedef enum
{
  MAL_OP_ADD,
  MAL_OP_SUB,
  MAL_OP_MUL,
  MAL_OP_DIV,
  MAL_OP_LOAD,
  MAL_OP_STORE,
  MAL_OP_CALL,
  MAL_OP_BRANCH,
  MAL_OP_BRANCH_COND,
  MAL_OP_RET,

  /* Floating point abstract opcodes */
  MAL_OP_FP_ADD,
  MAL_OP_FP_SUB,
  MAL_OP_FP_MUL,
  MAL_OP_FP_DIV,

  /* Vector abstract opcodes (may exist in several vector suites) */
  MAL_OP_VEC_ADD,
  MAL_OP_VEC_SUB,
  MAL_OP_VEC_MUL,
  MAL_OP_VEC_DIV,
  MAL_OP_VEC_LOAD,
  MAL_OP_VEC_STORE

} MALOpcode;

/* ------------------------------------------------------------
   Floating Point Abstract Interface (MDS will fill later)
   ------------------------------------------------------------ */

typedef struct MALFloatingPointInterface
{
  const char *suiteName;
  int (*isSupported) (const char *mnemonic);
  int fpRegisterClass; /* Virtual register class id for FP */

} MALFloatingPointInterface;

/* vector interface: machines may have several different vector suites */
typedef struct MALVectorInterface
{
  const char *suiteName;
  int (*isSupported) (const char *mnemonic);
  int vectorRegisterClass; /* Virtual register class id for vectors */
  unsigned long laneWidthBytes;
  unsigned long vectorWidthBytes;
} MALVectorInterface;

/* ------------------------------------------------------------
   MAL Machine Model Stubs (to be filled by MDS later)
   ------------------------------------------------------------ */

typedef struct MALMachineModel
{
  MALFloatingPointInterface **fpSuites;
  MALIndex fpSuiteCount;

  MALVectorInterface **vectorSuites;
  MALIndex vectorSuiteCount;

} MALMachineModel;

/* ------------------------------------------------------------
   MAL Instruction Representation
   ------------------------------------------------------------ */

typedef struct MALInstr
{
  MALInstrId id;
  MALOpcode opcode;

  MALReg *outputs;
  MALIndex outputCount;

  MALReg *inputs;
  MALIndex inputCount;

} MALInstr;

/* ------------------------------------------------------------
   MAL Basic Block (linear chain)
   ------------------------------------------------------------ */

typedef struct MALBlock
{
  MALBlockId id;
  MALInstr **instructions;
  MALIndex instrCount;

  MALBlockId *successors;
  MALIndex successorCount;
} MALBlock;

/* ------------------------------------------------------------
   MAL Program Unit
   ------------------------------------------------------------ */

typedef struct MALUnit
{
  MALBlock **blocks;
  MALIndex blockCount;
  MALMachineModel *machineModel;
} MALUnit;

/* ------------------------------------------------------------
   Construction API
   ------------------------------------------------------------ */

MALUnit *malCreateUnit (void);
void malDestroyUnit (MALUnit *unit);

MALBlock *malCreateBlock (void);
void malAddBlock (MALUnit *unit, MALBlock *block);
void malAddSuccessor (MALBlock *blk, MALBlockId succ);

MALInstr *malCreateInstr (MALOpcode opcode, MALReg *outputs, MALIndex outCount,
                          MALReg *inputs, MALIndex inCount);

void malAppendInstr (MALBlock *block, MALInstr *instr);

/* ------------------------------------------------------------
   Virtual Register Management (infinite)
   ------------------------------------------------------------ */

MALReg malNewVirtualReg (void);

/* Allocate registers from FP or Vector register classes */
MALReg malNewFPReg (int fpClassId);
MALReg malNewVectorReg (int vectorClassId);

/* ------------------------------------------------------------
   Lowering from WIL (Instruction Selection Stub)
   ------------------------------------------------------------ */

/*
  Lower WIL (scheduled graph) to MAL using instruction selection.
  Requires:
    - WILSchedule (from wil.h)
    - Machine model for selecting appropriate opcodes
*/
MALUnit *malLowerFromWIL (const WILContext *wilCtx,
                          const WILSchedule *schedule,
                          const MALMachineModel *model);

/* ------------------------------------------------------------
   Peephole Optimization (MAL’s own TRS)
   ------------------------------------------------------------ */

typedef struct MALPattern MALPattern;
typedef struct MALRewriteRule MALRewriteRule;
typedef struct MALRewriteSystem MALRewriteSystem;

MALRewriteSystem *malCreateRewriteSystem (void);
void malDestroyRewriteSystem (MALRewriteSystem *sys);

MALPattern *malPatternOpcode (MALOpcode opcode);
MALPattern *malPatternWildcard (void);
void malDestroyPattern (MALPattern *p);

/* rewrite action: return replacement instruction list */
typedef MALInstr **(*MALRewriteAction) (MALUnit *unit, MALBlock *block,
                                        MALInstr *instr, void *userData,
                                        MALIndex *outCount);

MALRewriteRule *malCreateRule (const char *name, MALPattern *pattern,
                               MALRewriteAction action, void *userData);

void malAddRule (MALRewriteSystem *sys, MALRewriteRule *rule);

int malApplyOnePeephole (MALUnit *unit, MALRewriteSystem *sys);
int malApplyPeepholeToFixpoint (MALUnit *unit, MALRewriteSystem *sys);

/* ------------------------------------------------------------
   Instruction Scheduling (local and global)
   ------------------------------------------------------------ */

/*
  MAL instruction scheduling:
    - local scheduling inside each block
    - global scheduling using block dependencies
*/
void malScheduleLocal (MALBlock *block);
void malScheduleGlobal (MALUnit *unit);

/* ------------------------------------------------------------
   Register Allocation Stub (needs MDS)
   ------------------------------------------------------------ */

void malRegisterAllocate (MALUnit *unit, const MALMachineModel *model);

/* ------------------------------------------------------------
   Debugging
   ------------------------------------------------------------ */

void malPrintInstr (const MALInstr *instr);
void malPrintBlock (const MALBlock *block);
void malPrintUnit (const MALUnit *unit);

WIRBLE_END_DECLS

#endif /* WIRBLE_MAL_H */
