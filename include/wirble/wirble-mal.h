/*
 * wirble-mal.h — WIRBLE Machine Abstraction Layer (MAL)
 *
 * Linear SSA-based intermediate representation sitting between
 * the Sea-of-Nodes WIL IR and the target-facing MDS layer.
 *
 * Pipeline:  WIL → [WRS rewrite] → MAL → [MRS rewrite] → [MDS lower] → Target
 * ISA
 *
 * Design goals:
 *   1. Linear SSA with infinite virtual registers
 *   2. Straightforward lowering from scheduled WIL graphs
 *   3. Target lowering via MDS machine descriptions
 *   4. Hooks for register allocation, scheduling, peephole, binarization
 *   5. DSL-friendly: structures map to S-expression (.mal / .mrs) files
 */

#ifndef WIRBLE_MAL_H
#define WIRBLE_MAL_H


#include <stddef.h>
#include <stdint.h>

#include "api-boilerplate.h"

WIRBLE_BEGIN_DECLS

/* ── Forward declarations from companion headers ────────────────────── */

/* wirble-wil.h */
typedef struct WILContext WILContext;
typedef struct WILSchedule WILSchedule;
typedef struct WILGraph WILGraph;
typedef struct WILNode WILNode;
typedef struct WILBlock WILBlock;

/* wirble-mds.h */
typedef struct MDSMachine MDSMachine;
typedef struct MDSInstruction MDSInstruction;
typedef struct MDSProgram MDSProgram;
typedef struct MDSInstrSelector MDSInstrSelector;
typedef struct MDSRegister MDSRegister;

/* wirble-wrs.h */
typedef struct WRSSExpr WRSSExpr;
typedef struct WRSFile WRSFile;
typedef struct WILRewriteSystem WILRewriteSystem;

/* ════════════════════════════════════════════════════════════════════
 *  §1  BASIC TYPEDEFS
 * ════════════════════════════════════════════════════════════════════ */

typedef uint32_t MALReg;     /* virtual register (SSA name)          */
typedef uint32_t MALBlockId; /* basic-block identifier               */
typedef uint32_t MALInstrId; /* instruction identifier               */
typedef uint32_t MALIndex;   /* generic index / count                */

#define MAL_REG_NONE ((MALReg)0xFFFFFFFFu)
#define MAL_BLOCK_NONE ((MALBlockId)0xFFFFFFFFu)

/* ════════════════════════════════════════════════════════════════════
 *  §2  TYPE SYSTEM
 * ════════════════════════════════════════════════════════════════════ */

typedef enum MALType
{
  MAL_TYPE_VOID = 0,

  /* integers */
  MAL_TYPE_I1, /* boolean / predicate                     */
  MAL_TYPE_I8,
  MAL_TYPE_I16,
  MAL_TYPE_I32,
  MAL_TYPE_I64,

  /* floating point */
  MAL_TYPE_F32,
  MAL_TYPE_F64,

  /* pointer (target-width, opaque)                                  */
  MAL_TYPE_PTR,

  /* fixed-width SIMD vector — lane type + count stored separately   */
  MAL_TYPE_VEC,

  MAL_TYPE__COUNT
} MALType;

/* Descriptor for vector types */
typedef struct MALVecDesc
{
  MALType laneType;   /* element type (I8..F64)                  */
  uint16_t laneCount; /* number of lanes                         */
} MALVecDesc;

/* ════════════════════════════════════════════════════════════════════
 *  §3  OPCODES
 * ════════════════════════════════════════════════════════════════════ */

typedef enum MALOpcode
{
  /* ── sentinel ─────────────────────────────────────────────────── */
  MAL_OP_NOP = 0,

  /* ── data movement ────────────────────────────────────────────── */
  MAL_OP_MOV,
  MAL_OP_LOAD,
  MAL_OP_STORE,
  MAL_OP_LEA, /* load effective address                  */

  /* ── integer arithmetic ───────────────────────────────────────── */
  MAL_OP_ADD,
  MAL_OP_SUB,
  MAL_OP_MUL,
  MAL_OP_SDIV, /* signed division                         */
  MAL_OP_UDIV, /* unsigned division                       */
  MAL_OP_SMOD, /* signed remainder                        */
  MAL_OP_UMOD, /* unsigned remainder                      */
  MAL_OP_NEG,

  /* ── bitwise ──────────────────────────────────────────────────── */
  MAL_OP_AND,
  MAL_OP_OR,
  MAL_OP_XOR,
  MAL_OP_NOT,
  MAL_OP_SHL,
  MAL_OP_LSHR, /* logical shift right                    */
  MAL_OP_ASHR, /* arithmetic shift right                 */
  MAL_OP_ROTL, /* rotate left                            */
  MAL_OP_ROTR, /* rotate right                           */

  /* ── floating-point arithmetic ────────────────────────────────── */
  MAL_OP_FADD,
  MAL_OP_FSUB,
  MAL_OP_FMUL,
  MAL_OP_FDIV,
  MAL_OP_FNEG,
  MAL_OP_FABS,
  MAL_OP_FSQRT,
  MAL_OP_FMA, /* fused multiply-add                     */

  /* ── integer comparison (produces I1) ─────────────────────────── */
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

  /* ── float comparison (produces I1) ───────────────────────────── */
  MAL_OP_FCMP_OEQ,
  MAL_OP_FCMP_ONE,
  MAL_OP_FCMP_OLT,
  MAL_OP_FCMP_OLE,
  MAL_OP_FCMP_OGT,
  MAL_OP_FCMP_OGE,
  MAL_OP_FCMP_UNO, /* unordered                              */
  MAL_OP_FCMP_ORD, /* ordered                                */

  /* ── control flow ─────────────────────────────────────────────── */
  MAL_OP_JMP,    /* unconditional jump                     */
  MAL_OP_BR,     /* conditional branch (cond, T, F)        */
  MAL_OP_SWITCH, /* multi-way branch                       */
  MAL_OP_CALL,
  MAL_OP_CALL_INDIRECT, /* call through pointer                   */
  MAL_OP_RET,
  MAL_OP_RET_VOID,

  /* ── SSA constructs ───────────────────────────────────────────── */
  MAL_OP_PHI,
  MAL_OP_SELECT, /* ternary: select(cond, a, b)            */
  MAL_OP_COPY,   /* parallel-copy (used during regalloc)   */

  /* ── type conversions ─────────────────────────────────────────── */
  MAL_OP_SEXT,    /* sign-extend                            */
  MAL_OP_ZEXT,    /* zero-extend                            */
  MAL_OP_TRUNC,   /* truncate                               */
  MAL_OP_ITOF,    /* int → float                            */
  MAL_OP_UTOF,    /* unsigned int → float                   */
  MAL_OP_FTOI,    /* float → signed int                     */
  MAL_OP_FTOU,    /* float → unsigned int                   */
  MAL_OP_BITCAST, /* reinterpret bits                       */
  MAL_OP_PTRTOINT,
  MAL_OP_INTTOPTR,

  /* ── vector operations ────────────────────────────────────────── */
  MAL_OP_VADD,
  MAL_OP_VSUB,
  MAL_OP_VMUL,
  MAL_OP_VDIV,
  MAL_OP_VLOAD,
  MAL_OP_VSTORE,
  MAL_OP_VEXTRACT, /* extract scalar from lane               */
  MAL_OP_VINSERT,  /* insert scalar into lane                */
  MAL_OP_VSPLAT,   /* broadcast scalar to all lanes          */
  MAL_OP_VSHUFFLE, /* permute lanes                          */

  /* ── memory intrinsics ────────────────────────────────────────── */
  MAL_OP_MEMCPY,
  MAL_OP_MEMSET,
  MAL_OP_MEMMOVE,

  /* ── atomic operations ────────────────────────────────────────── */
  MAL_OP_ATOMIC_LOAD,
  MAL_OP_ATOMIC_STORE,
  MAL_OP_ATOMIC_XCHG,
  MAL_OP_ATOMIC_CMPXCHG,
  MAL_OP_ATOMIC_ADD,
  MAL_OP_ATOMIC_SUB,
  MAL_OP_ATOMIC_AND,
  MAL_OP_ATOMIC_OR,
  MAL_OP_ATOMIC_XOR,
  MAL_OP_FENCE,

  /* ── stack / frame ────────────────────────────────────────────── */
  MAL_OP_ALLOCA,     /* stack allocation                       */
  MAL_OP_FRAME_ADDR, /* materialise frame pointer              */

  /* ── target-specific escape hatch ─────────────────────────────── */
  MAL_OP_TARGET, /* opaque target op; mnemonic in metadata */

  MAL_OP__COUNT
} MALOpcode;

/* ════════════════════════════════════════════════════════════════════
 *  §4  OPERANDS
 * ════════════════════════════════════════════════════════════════════ */

typedef enum MALOperandKind
{
  MAL_OPND_REG = 0,   /* virtual (or physical after RA) reg     */
  MAL_OPND_IMM_INT,   /* 64-bit signed immediate                */
  MAL_OPND_IMM_FLOAT, /* 64-bit double immediate                */
  MAL_OPND_BLOCK,     /* basic-block reference (for BR / PHI)   */
  MAL_OPND_FUNC,      /* function reference (for CALL)          */
  MAL_OPND_GLOBAL,    /* global symbol reference                */
  MAL_OPND_TYPE,      /* type tag (for conversions)             */
  MAL_OPND_VECDESC,   /* vector descriptor                      */
} MALOperandKind;

typedef struct MALOperand
{
  MALOperandKind kind;
  union
  {
    MALReg reg;
    int64_t immInt;
    double immFloat;
    MALBlockId blockRef;
    const char *funcRef;   /* function name                     */
    const char *globalRef; /* global symbol name                */
    MALType typeTag;
    MALVecDesc vecDesc;
  } u;
} MALOperand;

/* ════════════════════════════════════════════════════════════════════
 *  §5  INSTRUCTIONS
 * ════════════════════════════════════════════════════════════════════ */

#define MAL_MAX_OPERANDS 8 /* inline capacity; overflow → heap      */

typedef struct MALInst
{
  MALInstrId id;
  MALOpcode opcode;
  MALType type; /* result type (VOID for stores/jumps)   */

  /* destination (SSA def) — MAL_REG_NONE if no result              */
  MALReg dst;

  /* source operands                                                */
  MALOperand ops[MAL_MAX_OPERANDS];
  MALIndex opCount;
  MALOperand *opsOverflow; /* non-NULL when opCount > MAX          */

  /* metadata / annotations                                         */
  const char *comment; /* optional debug annotation            */
  uint32_t flags;      /* MAL_INST_FLAG_* bits                 */

  /* linked-list threading inside block                             */
  struct MALInst *prev;
  struct MALInst *next;
} MALInst;

/* Instruction flag bits */
#define MAL_INST_FLAG_COMMUTATIVE (1u << 0)
#define MAL_INST_FLAG_SIDEEFFECT (1u << 1)
#define MAL_INST_FLAG_VOLATILE (1u << 2)
#define MAL_INST_FLAG_DEAD (1u << 3) /* marked by DCE        */

/* ════════════════════════════════════════════════════════════════════
 *  §6  BASIC BLOCKS
 * ════════════════════════════════════════════════════════════════════ */

typedef struct MALBlock
{
  MALBlockId id;
  const char *name; /* optional label (e.g. "entry")         */

  /* instruction doubly-linked list                                 */
  MALInst *head;
  MALInst *tail;
  MALIndex instrCount;

  /* CFG edges                                                      */
  MALBlockId *successors;
  MALIndex succCount;
  MALBlockId *predecessors;
  MALIndex predCount;

  /* dominator tree (populated by analysis passes)                  */
  MALBlockId idom; /* immediate dominator                   */
  MALBlockId *domChildren;
  MALIndex domChildCount;

  /* loop nesting depth (0 = not in loop; set by loop analysis)     */
  uint32_t loopDepth;

  /* liveness (populated by regalloc prep)                          */
  uint32_t *liveIn;  /* bitset of live-in  virtual regs       */
  uint32_t *liveOut; /* bitset of live-out virtual regs       */
} MALBlock;

/* ════════════════════════════════════════════════════════════════════
 *  §7  FUNCTIONS
 * ════════════════════════════════════════════════════════════════════ */

typedef struct MALParam
{
  MALType type;
  MALReg reg; /* virtual register holding the param    */
} MALParam;

typedef struct MALFunction
{
  const char *name;
  MALType retType;

  MALParam *params;
  MALIndex paramCount;

  MALBlock **blocks;
  MALIndex blockCount;
  MALIndex blockCap; /* allocated capacity                    */

  MALBlockId entryBlock; /* id of the entry block                 */

  /* SSA register counter — next available vreg                     */
  MALReg nextReg;

  /* instruction id counter                                         */
  MALInstrId nextInstrId;

  /* stack frame estimate (bytes, updated by alloca lowering)       */
  uint32_t frameSize;

  /* calling convention tag (target-specific, from MDS)             */
  uint32_t callingConv;
} MALFunction;

/* ════════════════════════════════════════════════════════════════════
 *  §8  MODULE (top-level container)
 * ════════════════════════════════════════════════════════════════════ */

typedef struct MALGlobal
{
  const char *name;
  MALType type;
  int64_t initVal; /* initial value (0 if BSS)              */
  uint32_t size;   /* size in bytes                         */
  uint32_t align;  /* alignment in bytes                    */
  int isConst;     /* 1 = read-only                         */
} MALGlobal;

typedef struct MALModule
{
  const char *name;

  MALFunction **functions;
  MALIndex funcCount;
  MALIndex funcCap;

  MALGlobal **globals;
  MALIndex globalCount;
  MALIndex globalCap;

  /* target triple string (e.g. "riscv64-unknown-elf")              */
  const char *targetTriple;

  /* associated machine model (borrowed, not owned)                 */
  const MDSMachine *machine;
} MALModule;

/* ════════════════════════════════════════════════════════════════════
 *  §9  BUILDER API — programmatic IR construction
 * ════════════════════════════════════════════════════════════════════ */

/* ── module / function / block lifecycle ──────────────────────────── */
MALModule *mal_module_create (const char *name);
void mal_module_destroy (MALModule *mod);
void mal_module_set_target (MALModule *mod, const char *triple,
                            const MDSMachine *machine);

MALFunction *mal_function_create (MALModule *mod, const char *name,
                                  MALType retType, const MALType *paramTypes,
                                  MALIndex paramCount);
void mal_function_destroy (MALFunction *fn);

MALBlock *mal_block_create (MALFunction *fn, const char *name);
void mal_block_remove (MALFunction *fn, MALBlockId id);

/* ── register allocation ─────────────────────────────────────────── */
MALReg mal_new_reg (MALFunction *fn);
MALReg mal_new_reg_typed (MALFunction *fn, MALType type);

/* ── operand constructors ────────────────────────────────────────── */
MALOperand mal_op_reg (MALReg r);
MALOperand mal_op_imm_int (int64_t val);
MALOperand mal_op_imm_float (double val);
MALOperand mal_op_block (MALBlockId bid);
MALOperand mal_op_func (const char *name);
MALOperand mal_op_global (const char *name);
MALOperand mal_op_type (MALType t);
MALOperand mal_op_vecdesc (MALType lane, uint16_t count);

/* ── instruction emission ────────────────────────────────────────── */

/*
 * mal_emit — append a new instruction to `block`.
 * Returns the MALInst* for further operand additions if needed.
 *
 *   MALInst *i = mal_emit(block, fn, MAL_OP_ADD, MAL_TYPE_I32, dst);
 *   mal_add_operand(i, mal_op_reg(src1));
 *   mal_add_operand(i, mal_op_reg(src2));
 */
MALInst *mal_emit (MALBlock *block, MALFunction *fn, MALOpcode op,
                   MALType type, MALReg dst);

/* Emit with a pre-built operand array (convenience) */
MALInst *mal_emit_full (MALBlock *block, MALFunction *fn, MALOpcode op,
                        MALType type, MALReg dst, const MALOperand *ops,
                        MALIndex opCount);

void mal_add_operand (MALInst *inst, MALOperand op);

/* Insert `inst` before `before` in the same block */
void mal_insert_before (MALBlock *block, MALInst *before, MALInst *inst);

/* Remove and free an instruction */
void mal_remove_inst (MALBlock *block, MALInst *inst);

/* ── PHI helpers ─────────────────────────────────────────────────── */

/*
 * mal_emit_phi — create a PHI node in `block` with `count` incoming
 * edges.  Operands are (block, value) pairs:
 *   mal_add_operand(phi, mal_op_block(pred1));
 *   mal_add_operand(phi, mal_op_reg(val1));
 *   mal_add_operand(phi, mal_op_block(pred2));
 *   mal_add_operand(phi, mal_op_reg(val2));
 */
MALInst *mal_emit_phi (MALBlock *block, MALFunction *fn, MALType type,
                       MALReg dst, MALIndex incomingCount);

/* ════════════════════════════════════════════════════════════════════
 *  §10  WIL → MAL LOWERING
 * ════════════════════════════════════════════════════════════════════ */

/*
 * Lower a scheduled WIL graph into a MAL module.
 *
 * Requires:
 *   - `wilCtx`   : WIL context owning the graph
 *   - `schedule` : a valid WIL schedule (basic-block ordering of nodes)
 *   - `machine`  : target machine description (may be NULL for
 *                  target-independent lowering)
 *
 * Returns a new MALModule (caller owns), or NULL on failure.
 */
MALModule *malLowerFromWIL (const WILContext *wilCtx,
                            const WILSchedule *schedule,
                            const MDSMachine *machine);

/* Per-function lowering (used internally, exposed for testing) */
MALFunction *malLowerFunctionFromWIL (const WILContext *wilCtx,
                                      const WILGraph *graph,
                                      const WILSchedule *schedule,
                                      const MDSMachine *machine);

/* ════════════════════════════════════════════════════════════════════
 *  §11  MAL-LEVEL REWRITE SYSTEM (MRS)
 *
 *  Peephole optimizations expressed as TRS rules, loadable from
 *  .mrs S-expression files via WRS infrastructure.
 * ════════════════════════════════════════════════════════════════════ */

/* Opaque handles */
typedef struct MALPattern MALPattern;
typedef struct MALRewriteRule MALRewriteRule;
typedef struct MALRewriteSystem MALRewriteSystem;

/* Match context passed to rewrite actions */
typedef struct MALMatchCtx
{
  MALInst **matched; /* array of matched instructions         */
  MALIndex matchCount;
  MALBlock *block;
  MALFunction *fn;
} MALMatchCtx;

/* Rewrite action callback:
 *   Return a NULL-terminated array of replacement MALInst*,
 *   or NULL to indicate "no rewrite". */
typedef MALInst **(*MALRewriteAction) (const MALMatchCtx *ctx, void *userData);

/* ── lifecycle ────────────────────────────────────────────────────── */
MALRewriteSystem *malCreateRewriteSystem (void);
void malDestroyRewriteSystem (MALRewriteSystem *sys);

/* ── pattern construction ─────────────────────────────────────────── */
MALPattern *malPatternOpcode (MALOpcode opcode);
MALPattern *malPatternWildcard (void);
MALPattern *malPatternSeq (MALPattern **pats, MALIndex count);
MALPattern *malPatternOpcodeTyped (MALOpcode opcode, MALType type);
void malDestroyPattern (MALPattern *p);

/* ── rule construction ────────────────────────────────────────────── */
MALRewriteRule *malCreateRule (const char *name, MALPattern *pattern,
                               MALRewriteAction action, void *userData);
void malDestroyRule (MALRewriteRule *rule);
void malAddRule (MALRewriteSystem *sys, MALRewriteRule *rule);

/* ── loading rules from .mrs S-expression files ───────────────────── */
int malLoadMRS (MALRewriteSystem *sys, const char *path);
int malLoadMRSFromSExpr (MALRewriteSystem *sys, const WRSSExpr *expr);

/* ── application ──────────────────────────────────────────────────── */
int malApplyOnePeephole (MALModule *mod, MALRewriteSystem *sys);
int malApplyPeepholeToFixpoint (MALModule *mod, MALRewriteSystem *sys);

/* Stats */
typedef struct MALRewriteStats
{
  uint64_t rulesApplied;
  uint64_t iterations;
  uint64_t instsBefore;
  uint64_t instsAfter;
} MALRewriteStats;

MALRewriteStats malGetRewriteStats (const MALRewriteSystem *sys);

/* ════════════════════════════════════════════════════════════════════
 *  §12  ANALYSIS PASSES
 * ════════════════════════════════════════════════════════════════════ */

/* Build / rebuild CFG predecessor lists */
void malBuildCFG (MALFunction *fn);

/* Compute dominator tree (fills idom / domChildren in each block) */
void malComputeDominators (MALFunction *fn);

/* Compute loop nesting depths */
void malComputeLoopInfo (MALFunction *fn);

/* Compute liveness (fills liveIn / liveOut bitsets) */
void malComputeLiveness (MALFunction *fn);

/* SSA validation — returns 0 on success, nonzero on error */
int malVerifySSA (const MALFunction *fn);

/* Full module verification */
int malVerifyModule (const MALModule *mod);

/* ════════════════════════════════════════════════════════════════════
 *  §13  OPTIMIZATION PASSES
 * ════════════════════════════════════════════════════════════════════ */

/* Dead code elimination */
void malDCE (MALFunction *fn);

/* Constant folding */
void malConstantFold (MALFunction *fn);

/* Copy propagation */
void malCopyProp (MALFunction *fn);

/* Common subexpression elimination (local, within blocks) */
void malLocalCSE (MALFunction *fn);

/* ════════════════════════════════════════════════════════════════════
 *  §14  INSTRUCTION SCHEDULING
 * ════════════════════════════════════════════════════════════════════ */

/* Local list scheduling within a single block */
void malScheduleLocal (MALBlock *block, const MDSMachine *machine);

/* Global scheduling across the function */
void malScheduleGlobal (MALFunction *fn, const MDSMachine *machine);

/* ════════════════════════════════════════════════════════════════════
 *  §15  REGISTER ALLOCATION
 * ════════════════════════════════════════════════════════════════════ */

typedef enum MALRegAllocKind
{
  MAL_RA_LINEAR_SCAN = 0,
  MAL_RA_GRAPH_COLORING,
} MALRegAllocKind;

typedef struct MALRegAllocConfig
{
  MALRegAllocKind kind;
  int spillWeightByLoopDepth; /* use loop depth for spill cost */
  int coalesceAggressively;
} MALRegAllocConfig;

typedef struct MALRegAllocResult
{
  int success; /* 0 = success                           */
  uint32_t spillCount;
  uint32_t reloadsInserted;
  uint32_t copiesCoalesced;
} MALRegAllocResult;

MALRegAllocResult malRegisterAllocate (MALFunction *fn,
                                       const MDSMachine *machine,
                                       const MALRegAllocConfig *config);

/* ════════════════════════════════════════════════════════════════════
 *  §16  MAL → MDS LOWERING (instruction selection bridge)
 * ════════════════════════════════════════════════════════════════════ */

/*
 * Lower a MAL module to an MDS program using the instruction selector.
 * The selector should already have patterns loaded (via mdsLoadPatterns).
 *
 * Returns a new MDSProgram (caller owns), or NULL on failure.
 */
MDSProgram *malLowerToMDS (const MALModule *mod, const MDSMachine *machine,
                           const MDSInstrSelector *selector);

/* ════════════════════════════════════════════════════════════════════
 *  §17  BINARIZATION & LINKING
 * ════════════════════════════════════════════════════════════════════ */

typedef struct MALBinaryOpts
{
  const char *outputPath;
  const char *format; /* "elf", "coff", "macho", "raw"         */
  int relocatable;    /* 1 = .o, 0 = executable                */
  int debugInfo;      /* include DWARF / CodeView              */
} MALBinaryOpts;

typedef struct MALLinkOpts
{
  const char *outputPath;
  const char **objectFiles;
  MALIndex objectCount;
  const char **libraries;
  MALIndex libCount;
  const char *entrySymbol;
  const char *linkerScript; /* NULL = default                      */
} MALLinkOpts;

/* Emit a binary object from a lowered MDS program */
int mal_binarize (const MDSProgram *prog, const MDSMachine *machine,
                  const MALBinaryOpts *opts);

/* Link object files into a final executable / shared library */
int mal_link (const MALLinkOpts *opts);

/* ════════════════════════════════════════════════════════════════════
 *  §18  SERIALIZATION (S-expression round-trip)
 * ════════════════════════════════════════

════════════════════════════════════════════════════════════════════ */

/* Write MAL module to .mal S-expression file */
int mal_write_sexpr (const MALModule *mod, const char *path);

/* Parse MAL module from .mal S-expression file */
MALModule *mal_read_sexpr (const char *path);

/* Parse from in-memory WRSSExpr */
MALModule *mal_from_sexpr (const WRSSExpr *expr);

/* Convert MAL module to WRSSExpr (caller must free via wrsSExprDestroy) */
WRSSExpr *mal_to_sexpr (const MALModule *mod);

/* ════════════════════════════════════════════════════════════════════
 *  §19  DEBUG & VISUALIZATION
 * ════════════════════════════════════════════════════════════════════ */

/* Print to stdout */
void mal_dump_module (const MALModule *mod);
void mal_dump_function (const MALFunction *fn);
void mal_dump_block (const MALBlock *block);
void mal_dump_inst (const MALInst *inst);

/* Print to file */
void mal_fprint_module (FILE *fp, const MALModule *mod);
void mal_fprint_function (FILE *fp, const MALFunction *fn);
void mal_fprint_block (FILE *fp, const MALBlock *block);
void mal_fprint_inst (FILE *fp, const MALInst *inst);

/* Generate GraphViz DOT for CFG visualization */
char *mal_cfg_to_dot (const MALFunction *fn);
int mal_cfg_to_dot_file (const MALFunction *fn, const char *path);

/* Generate GraphViz DOT for dominator tree */
char *mal_domtree_to_dot (const MALFunction *fn);
int mal_domtree_to_dot_file (const MALFunction *fn, const char *path);

/* ════════════════════════════════════════════════════════════════════
 *  §20  UTILITY FUNCTIONS
 * ════════════════════════════════════════════════════════════════════ */

/* Opcode properties */
const char *mal_opcode_name (MALOpcode op);
int mal_opcode_is_terminator (MALOpcode op);
int mal_opcode_is_commutative (MALOpcode op);
int mal_opcode_has_side_effect (MALOpcode op);
int mal_opcode_is_comparison (MALOpcode op);

/* Type properties */
const char *mal_type_name (MALType t);
uint32_t mal_type_size (MALType t, const MDSMachine *machine);
uint32_t mal_type_align (MALType t, const MDSMachine *machine);
int mal_type_is_int (MALType t);
int mal_type_is_float (MALType t);
int mal_type_is_vector (MALType t);

/* Operand utilities */
int mal_operand_is_constant (const MALOperand *op);
int mal_operand_equals (const MALOperand *a, const MALOperand *b);

/* Block utilities */
MALBlock *mal_get_block (const MALFunction *fn, MALBlockId id);
int mal_block_is_empty (const MALBlock *block);
int mal_block_ends_with_terminator (const MALBlock *block);

/* Instruction utilities */
int mal_inst_is_pure (const MALInst *inst);
int mal_inst_defines_reg (const MALInst *inst, MALReg reg);
int mal_inst_uses_reg (const MALInst *inst, MALReg reg);

/* Register utilities */
int mal_reg_is_virtual (MALReg reg);
int mal_reg_is_physical (MALReg reg);

/* ════════════════════════════════════════════════════════════════════
 *  §21  MACHINE MODEL INTEGRATION (from MDS)
 * ════════════════════════════════════════════════════════════════════ */

/*
 * MAL doesn't own the machine model — it's borrowed from MDS.
 * These are convenience wrappers that query the MDSMachine.
 */

/* Query register classes available on target */
typedef enum MALRegClass
{
  MAL_REGCLASS_GPR = 0,
  MAL_REGCLASS_FP,
  MAL_REGCLASS_VECTOR,
  MAL_REGCLASS_SPECIAL,
  MAL_REGCLASS__COUNT
} MALRegClass;

/* Get physical register count for a class */
uint32_t mal_machine_reg_count (const MDSMachine *machine, MALRegClass cls);

/* Get register name (for debug printing) */
const char *mal_machine_reg_name (const MDSMachine *machine, MALRegClass cls,
                                  uint32_t physReg);

/* Check if target supports an opcode natively */
int mal_machine_supports_opcode (const MDSMachine *machine, MALOpcode op);

/* Get target pointer size in bytes */
uint32_t mal_machine_pointer_size (const MDSMachine *machine);

/* Get target natural alignment */
uint32_t mal_machine_natural_align (const MDSMachine *machine);

/* ════════════════════════════════════════════════════════════════════
 *  §22  STATS & METRICS
 * ════════════════════════════════════════════════════════════════════ */

typedef struct MALStats
{
  uint64_t functionCount;
  uint64_t blockCount;
  uint64_t instructionCount;
  uint64_t phiCount;
  uint64_t virtualRegCount;
} MALStats;

/* Collect module statistics */
void mal_collect_stats (const MALModule *mod, MALStats *outStats);

/* Collect per-function statistics */
void mal_collect_function_stats (const MALFunction *fn, MALStats *outStats);

/* ════════════════════════════════════════════════════════════════════
 *  §23  PASSES PIPELINE HELPERS
 *
 *  Convenience helpers for typical backend pipelines.
 * ════════════════════════════════════════════════════════════════════ */

/* Run standard MAL optimization pipeline */
void mal_run_default_pipeline (MALModule *mod, const MDSMachine *machine,
                               MALRewriteSystem *mrs);

/* Typical per-function pipeline:
 *
 *   CFG
 *   Dominators
 *   Loop info
 *   Constant fold
 *   Copy propagation
 *   CSE
 *   DCE
 */
void mal_optimize_function (MALFunction *fn);

/* Prepare function for register allocation */
void mal_prepare_for_regalloc (MALFunction *fn);

/* ════════════════════════════════════════════════════════════════════
 *  §24  VERSIONING
 * ════════════════════════════════════════════════════════════════════ */

#define WIRBLE_MAL_VERSION_MAJOR 1
#define WIRBLE_MAL_VERSION_MINOR 0
#define WIRBLE_MAL_VERSION_PATCH 0

const char *mal_version_string (void);

WIRBLE_END_DECLS

#endif /* WIRBLE_MAL_H */
