#ifndef WIRBLE_MAL_H
#define WIRBLE_MAL_H

#include <stddef.h>
#include <stdint.h>

#include "api-boilerplate.h"

WIRBLE_BEGIN_DECLS

/* Forward declarations for integrations */
struct WILGraph;
struct WRSSExpr;
struct MDSMachine;
struct MDSInstrSelector;
struct MDSProgram;

/* ════════════════════════════════════════════════════════════════
 *  §1 BASIC TYPES
 * ════════════════════════════════════════════════════════════════ */

typedef uint32_t MALReg;
typedef uint32_t MALBlockId;
typedef uint32_t MALInstrId;
typedef uint32_t MALIndex;

#define MAL_INVALID_REG ((MALReg) - 1)
#define MAL_INVALID_BLOCK ((MALBlockId) - 1)
#define MAL_INVALID_INST ((MALInstrId) - 1)

/* ════════════════════════════════════════════════════════════════
 *  §2 TYPE SYSTEM
 * ════════════════════════════════════════════════════════════════ */

typedef enum MALType
{
  MAL_TYPE_VOID = 0,

  MAL_TYPE_I1,
  MAL_TYPE_I8,
  MAL_TYPE_I16,
  MAL_TYPE_I32,
  MAL_TYPE_I64,

  MAL_TYPE_F32,
  MAL_TYPE_F64,

  MAL_TYPE_PTR,
  MAL_TYPE_VEC,

  MAL_TYPE__COUNT
} MALType;

typedef struct MALVecDesc
{
  MALType elementType;
  uint32_t count;
} MALVecDesc;

/* ════════════════════════════════════════════════════════════════
 *  §3 EXTENSIVE OPCODE SET
 *  Covers all major IR categories and target lowering escape
 * ════════════════════════════════════════════════════════════════ */

typedef enum MALOpcode
{

  /* ─ DATA MOVEMENT ─ */
  MAL_OP_NOP = 0,
  MAL_OP_COPY,
  MAL_OP_LOAD,
  MAL_OP_STORE,
  MAL_OP_LOAD_IMM,
  MAL_OP_MEMCPY,
  MAL_OP_MEMSET,

  /* ─ INTEGER ARITH ─ */
  MAL_OP_ADD,
  MAL_OP_SUB,
  MAL_OP_MUL,
  MAL_OP_UDIV,
  MAL_OP_SDIV,
  MAL_OP_UREM,
  MAL_OP_SREM,
  MAL_OP_NEG,
  MAL_OP_INC,
  MAL_OP_DEC,

  /* ─ BITWISE ─ */
  MAL_OP_AND,
  MAL_OP_OR,
  MAL_OP_XOR,
  MAL_OP_NOT,
  MAL_OP_SHL,
  MAL_OP_LSHR,
  MAL_OP_ASHR,
  MAL_OP_ROTL,
  MAL_OP_ROTR,

  /* ─ FLOATING-POINT ─ */
  MAL_OP_FADD,
  MAL_OP_FSUB,
  MAL_OP_FMUL,
  MAL_OP_FDIV,
  MAL_OP_FREM,
  MAL_OP_FNEG,
  MAL_OP_FABS,
  MAL_OP_FSQRT,
  MAL_OP_FMA,

  /* ─ COMPARE ─ */
  MAL_OP_ICMP_EQ,
  MAL_OP_ICMP_NE,
  MAL_OP_ICMP_SLT,
  MAL_OP_ICMP_SLE,
  MAL_OP_ICMP_SGT,
  MAL_OP_ICMP_SGE,
  MAL_OP_ICMP_ULT,
  MAL_OP_ICMP_ULE,
  MAL_OP_ICMP_UGT,
  MAL_OP_ICMP_UGE,

  MAL_OP_FCMP_OEQ,
  MAL_OP_FCMP_ONE,
  MAL_OP_FCMP_OLT,
  MAL_OP_FCMP_OLE,
  MAL_OP_FCMP_OGT,
  MAL_OP_FCMP_OGE,
  MAL_OP_FCMP_UEQ,
  MAL_OP_FCMP_UNE,

  /* ─ CONTROL FLOW ─ */
  MAL_OP_BR,
  MAL_OP_CBR,
  MAL_OP_SWITCH,
  MAL_OP_RET,
  MAL_OP_UNREACHABLE,

  /* ─ SSA CONSTRUCTS ─ */
  MAL_OP_PHI,
  MAL_OP_SELECT,
  MAL_OP_INLINE_CALL,
  MAL_OP_CALL,
  MAL_OP_TAILCALL,

  /* ─ CASTING & CONVERSION ─ */
  MAL_OP_ZEXT,
  MAL_OP_SEXT,
  MAL_OP_TRUNC,
  MAL_OP_FEXT,
  MAL_OP_FTRUNC,
  MAL_OP_BITCAST,
  MAL_OP_PTRCAST,
  MAL_OP_INTTOPTR,
  MAL_OP_PTRTOINT,
  MAL_OP_SITOFP,
  MAL_OP_UITOFP,
  MAL_OP_FPTOSI,
  MAL_OP_FPTOUI,

  /* ─ STACK / FRAME OPS ─ */
  MAL_OP_ALLOCA,
  MAL_OP_STACKSAVE,
  MAL_OP_STACKRESTORE,

  /* ─ ATOMICS ─ */
  MAL_OP_ATOMIC_LOAD,
  MAL_OP_ATOMIC_STORE,
  MAL_OP_ATOMIC_RMW,
  MAL_OP_ATOMIC_CAS,

  /* ─ VECTOR OPS ─ */
  MAL_OP_VADD,
  MAL_OP_VSUB,
  MAL_OP_VMUL,
  MAL_OP_VDIV,
  MAL_OP_VSHUFFLE,
  MAL_OP_VEXTRACT,
  MAL_OP_VINSERT,
  MAL_OP_VBROADCAST,

  /* ─ TARGET CUSTOM INSTR ─ */
  MAL_OP_TARGET

} MALOpcode;

/* ════════════════════════════════════════════════════════════════
 *  §4 OPERANDS
 * ════════════════════════════════════════════════════════════════ */

typedef enum MALOperandKind
{
  MAL_OPND_REG = 0,
  MAL_OPND_IMM_INT,
  MAL_OPND_IMM_FLOAT,
  MAL_OPND_BLOCK,
  MAL_OPND_GLOBAL,
  MAL_OPND_FUNC,
  MAL_OPND_TYPE,
  MAL_OPND_VECDESC
} MALOperandKind;

typedef struct MALOperand
{
  MALOperandKind kind;
  MALType type;
  union
  {
    MALReg reg;
    int64_t imm_i;
    double imm_f;
    MALBlockId block;
    const char *globalName;
    const char *funcName;
    MALType asType;
    MALVecDesc vec;
  };
} MALOperand;

/* ════════════════════════════════════════════════════════════════
 *  §5 INSTRUCTIONS
 * ════════════════════════════════════════════════════════════════ */

typedef struct MALInst
{
  MALInstrId id;
  MALOpcode opcode;
  MALType type;

  MALReg dst; /* SSA output reg, MAL_INVALID_REG if none */

  MALOperand *operands;
  uint32_t operandCount;
  uint32_t operandCap;

  const char *comment;

  struct MALInst *prev;
  struct MALInst *next;

} MALInst;

/* ════════════════════════════════════════════════════════════════
 *  §6 BASIC BLOCKS
 * ════════════════════════════════════════════════════════════════ */

typedef struct MALBlock
{
  MALBlockId id;
  const char *name;

  MALInst *first;
  MALInst *last;

  MALBlockId *preds;
  MALBlockId *succs;
  uint32_t predCount;
  uint32_t succCount;

  uint32_t domParent;
  MALBlockId *domChildren;
  uint32_t domChildCount;

  uint32_t loopDepth;

  /* Bitsets for liveness (dense) */
  uint8_t *liveIn;
  uint8_t *liveOut;

  struct MALBlock *next;
} MALBlock;

/* ════════════════════════════════════════════════════════════════
 *  §7 FUNCTIONS
 * ════════════════════════════════════════════════════════════════ */

typedef struct MALFunction
{
  const char *name;

  MALType retType;
  MALType *paramTypes;
  uint32_t paramCount;

  MALBlock *blocks;
  uint32_t blockCount;

  MALReg nextSSAReg;

  struct MALFunction *next;
} MALFunction;

/* ════════════════════════════════════════════════════════════════
 *  §8 MODULE
 * ════════════════════════════════════════════════════════════════ */

typedef struct MALModule
{
  MALFunction *functions;
  uint32_t functionCount;

  const char *targetTriple;
  struct MDSMachine *mdsMachine;
} MALModule;

/* ════════════════════════════════════════════════════════════════
 *  §9 BUILDER API
 * ════════════════════════════════════════════════════════════════ */

MALModule *mal_module_create (const char *triple);
void mal_module_destroy (MALModule *m);

MALFunction *mal_function_create (MALModule *m, const char *name,
                                  MALType retType);
MALBlock *mal_block_create (MALFunction *fn, const char *name);

MALReg mal_new_reg (MALFunction *fn);

MALInst *mal_inst_create (MALOpcode opc, MALType ty, MALReg dst);
void mal_inst_add_operand (MALInst *inst, MALOperand op);

void mal_block_insert_end (MALBlock *b, MALInst *inst);
void mal_inst_remove (MALBlock *b, MALInst *inst);

/* PHI constructor */
MALInst *mal_emit_phi (MALBlock *b, MALType ty);

/* ════════════════════════════════════════════════════════════════
 *  §10 WIL → MAL LOWERING
 * ════════════════════════════════════════════════════════════════ */

int malLowerFromWIL (MALModule *dst, const struct WILGraph *src);

/* ════════════════════════════════════════════════════════════════
 *  §11 MAL REWRITE SYSTEM (MRS)
 *  Pattern matching, peepholes, ISel pre-rewrites
 * ════════════════════════════════════════════════════════════════ */

typedef struct MALMatchCtx
{
  MALInst *inst;
  MALBlock *block;
  MALFunction *function;
} MALMatchCtx;

typedef struct MALRewriteAction
{
  int removeInst;
  MALInst *replaceWith;
  MALInst **insertBefore;
  uint32_t insertBeforeCount;
  MALInst **insertAfter;
  uint32_t insertAfterCount;
} MALRewriteAction;

typedef struct MALRewriteRule MALRewriteRule;

typedef struct MALRewriteSystem
{
  MALRewriteRule *rules;
  uint32_t ruleCount;
} MALRewriteSystem;

MALRewriteSystem *mal_mrs_load (const char *path);
void mal_mrs_destroy (MALRewriteSystem *mrs);

int malApplyOnePeephole (MALRewriteSystem *mrs, MALFunction *fn);
int malApplyPeepholeToFixpoint (MALRewriteSystem *mrs, MALFunction *fn);

/* ════════════════════════════════════════════════════════════════
 *  §12 ANALYSIS PASSES
 * ════════════════════════════════════════════════════════════════ */

void mal_compute_cfg (MALFunction *fn);
void mal_compute_dominators (MALFunction *fn);
void mal_compute_liveness (MALFunction *fn);
int mal_verify_ssa (MALFunction *fn);

/* ════════════════════════════════════════════════════════════════
 *  §13 OPTIMIZATION PASSES
 * ════════════════════════════════════════════════════════════════ */

void mal_dce (MALFunction *fn);
void mal_const_fold (MALFunction *fn);
void mal_copy_prop (MALFunction *fn);
void mal_local_cse (MALFunction *fn);

/* ════════════════════════════════════════════════════════════════
 *  §14 SCHEDULING
 * ════════════════════════════════════════════════════════════════ */

void malScheduleLocal (MALFunction *fn);
void malScheduleGlobal (MALFunction *fn);

/* ════════════════════════════════════════════════════════════════
 *  §15 REGISTER ALLOCATION
 * ════════════════════════════════════════════════════════════════ */

typedef struct MALRAOptions
{
  int useLinearScan;
  int spillCostHeuristic;
} MALRAOptions;

int malRegisterAllocate (MALFunction *fn, const MALRAOptions *opts,
                         const struct MDSMachine *machine);

/* ════════════════════════════════════════════════════════════════
 *  §16 MAL → MDS LOWERING
 * ════════════════════════════════════════════════════════════════ */

struct MDSInstrSelector * 

struct MDSProgram *malLowerToMDS (const MALModule *m,
                                  const struct MDSInstrSelector *isel);

/* ════════════════════════════════════════════════════════════════
 *  §17 BINARIZATION + LINKING STUBS
 * ════════════════════════════════════════════════════════════════ */

typedef struct MALBinaryOpts
{
  int emitRelocations;
  int emitDebugInfo;
} MALBinaryOpts;

int mal_binarize (const struct MDSProgram *prog, const char *path,
                  const MALBinaryOpts *opts);

int mal_link (const char *outputPath, const char **objectFiles,
              uint32_t objectCount);

/* ════════════════════════════════════════════════════════════════
 *  §18 S-EXPRESSION SERIALIZATION
 * ════════════════════════════════════════════════════════════════ */

int mal_write_sexpr (const MALModule *m, const char *path);
MALModule *mal_read_sexpr (const char *path);

/* ════════════════════════════════════════════════════════════════
 *  §19 DEBUGGING / VISUALIZATION
 * ════════════════════════════════════════════════════════════════ */

void mal_dump_function (const MALFunction *fn);
void mal_dump_module (const MALModule *m);

void mal_generate_cfg_dot (const MALFunction *fn, const char *path);
void mal_generate_dom_dot (const MALFunction *fn, const char *path);

/* ════════════════════════════════════════════════════════════════
 *  §20 PIPELINE HELPERS
 * ════════════════════════════════════════════════════════════════ */

void mal_run_default_pipeline (MALFunction *fn, struct MDSMachine *machine,
                               struct MDSInstrSelector *isel);

void mal_optimize_function (MALFunction *fn);

/* ════════════════════════════════════════════════════════════════
 *  §21 VERSIONING
 * ════════════════════════════════════════════════════════════════ */

const char *mal_version_string (void);

WIRBLE_END_DECLS

#endif /* WIRBLE_MAL_H */
