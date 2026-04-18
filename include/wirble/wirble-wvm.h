#ifndef WIRBLE_WVM_H
#define WIRBLE_WVM_H

#include <stddef.h>
#include <stdint.h>

#include "api-boilerplate.h"

WIRBLE_BEGIN_DECLS

/* ------------------------------------------------------------
   WVM Instruction Encoding
   All instructions fit in 64 bits (uint64_t)
   ------------------------------------------------------------ */

typedef uint64_t WVMInstr;
typedef uint8_t WVMOpcode;
typedef uint16_t WVMReg;       /* Register index (0-65535) */
typedef uint32_t WVMOffset;    /* Offset into constant/string pool */
typedef int32_t WVMJumpOffset; /* Signed jump offset */

/* Instruction encoding formats */
typedef enum
{
  WVM_ENC_OP,          /* Opcode only (8 bits) */
  WVM_ENC_OP_R,        /* Opcode + 1 register (8 + 16 bits) */
  WVM_ENC_OP_RR,       /* Opcode + 2 registers (8 + 16 + 16 bits) */
  WVM_ENC_OP_RRR,      /* Opcode + 3 registers (8 + 16 + 16 + 16 bits) */
  WVM_ENC_OP_R_IMM16,  /* Opcode + register + 16-bit immediate */
  WVM_ENC_OP_R_IMM32,  /* Opcode + register + 32-bit immediate */
  WVM_ENC_OP_R_CONST,  /* Opcode + register + constant pool offset */
  WVM_ENC_OP_RR_CONST, /* Opcode + 2 registers + constant pool offset */
  WVM_ENC_OP_R_STR,    /* Opcode + register + string pool offset */
  WVM_ENC_OP_JUMP,     /* Opcode + signed jump offset (32 bits) */
  WVM_ENC_OP_R_JUMP,   /* Opcode + register + jump offset */
  WVM_ENC_OP_RR_JUMP,  /* Opcode + 2 registers + jump offset */
  WVM_ENC_OP_GUARD     /* Opcode + guard type + register + jump offset */
} WVMEncodingType;

/* Opcode definitions */
typedef enum
{
  /* Control flow */
  WVM_OP_NOP = 0,
  WVM_OP_HALT,
  WVM_OP_RETURN,
  WVM_OP_CALL,
  WVM_OP_TAILCALL,
  WVM_OP_JUMP,
  WVM_OP_JUMP_IF_TRUE,
  WVM_OP_JUMP_IF_FALSE,

  /* Register operations */
  WVM_OP_MOVE,       /* R[A] = R[B] */
  WVM_OP_LOAD_CONST, /* R[A] = K[B] */
  WVM_OP_LOAD_IMM,   /* R[A] = immediate */
  WVM_OP_LOAD_NIL,   /* R[A] = nil */
  WVM_OP_LOAD_TRUE,  /* R[A] = true */
  WVM_OP_LOAD_FALSE, /* R[A] = false */

  /* Arithmetic */
  WVM_OP_ADD, /* R[A] = R[B] + R[C] */
  WVM_OP_SUB, /* R[A] = R[B] - R[C] */
  WVM_OP_MUL, /* R[A] = R[B] * R[C] */
  WVM_OP_DIV, /* R[A] = R[B] / R[C] */
  WVM_OP_MOD, /* R[A] = R[B] % R[C] */
  WVM_OP_NEG, /* R[A] = -R[B] */
  WVM_OP_INC, /* R[A]++ */
  WVM_OP_DEC, /* R[A]-- */

  /* Bitwise */
  WVM_OP_AND,
  WVM_OP_OR,
  WVM_OP_XOR,
  WVM_OP_NOT,
  WVM_OP_SHL,
  WVM_OP_SHR,

  /* Comparison */
  WVM_OP_EQ, /* R[A] = R[B] == R[C] */
  WVM_OP_NE,
  WVM_OP_LT,
  WVM_OP_LE,
  WVM_OP_GT,
  WVM_OP_GE,

  /* Memory operations */
  WVM_OP_LOAD_GLOBAL,  /* R[A] = globals[K[B]] */
  WVM_OP_STORE_GLOBAL, /* globals[K[A]] = R[B] */
  WVM_OP_LOAD_UPVAL,   /* R[A] = upvalues[B] */
  WVM_OP_STORE_UPVAL,  /* upvalues[A] = R[B] */
  WVM_OP_LOAD_FIELD,   /* R[A] = R[B][K[C]] */
  WVM_OP_STORE_FIELD,  /* R[A][K[B]] = R[C] */

  /* Table operations */
  WVM_OP_NEW_TABLE, /* R[A] = {} */
  WVM_OP_GET_INDEX, /* R[A] = R[B][R[C]] */
  WVM_OP_SET_INDEX, /* R[A][R[B]] = R[C] */
  WVM_OP_GET_LEN,   /* R[A] = #R[B] */

  /* Function operations */
  WVM_OP_CLOSURE, /* R[A] = closure(proto[K[B]]) */

  /* Guards (for JIT deoptimization) */
  WVM_OP_GUARD_TYPE,     /* Guard R[A] has expected type, else deopt */
  WVM_OP_GUARD_TRUE,     /* Guard R[A] is true, else deopt */
  WVM_OP_GUARD_FALSE,    /* Guard R[A] is false, else deopt */
  WVM_OP_GUARD_NOT_NIL,  /* Guard R[A] is not nil, else deopt */
  WVM_OP_GUARD_CLASS,    /* Guard R[A] has expected class, else deopt */
  WVM_OP_GUARD_RANGE,    /* Guard R[A] in range, else deopt */
  WVM_OP_GUARD_RESOURCE, /* Guard resource available, else deopt */

  /* Stack operations */
  WVM_OP_PUSH,
  WVM_OP_POP,

  /* Type conversion */
  WVM_OP_TO_INT,
  WVM_OP_TO_FLOAT,
  WVM_OP_TO_STRING,
  WVM_OP_TO_BOOL,

  WVM_OP_COUNT
} WVMOpcodeEnum;

/* Instruction encoding lookup table */
extern const WVMEncodingType wvmOpcodeEncodings[WVM_OP_COUNT];

/* ------------------------------------------------------------
   Instruction Encoding/Decoding Macros
   ------------------------------------------------------------ */

/* Encode instructions */
#define WVM_ENCODE_OP(op) ((uint64_t)(op))

#define WVM_ENCODE_OP_R(op, a) (((uint64_t)(op)) | (((uint64_t)(a)) << 8))

#define WVM_ENCODE_OP_RR(op, a, b)                                            \
  (((uint64_t)(op)) | (((uint64_t)(a)) << 8) | (((uint64_t)(b)) << 24))

#define WVM_ENCODE_OP_RRR(op, a, b, c)                                        \
  (((uint64_t)(op)) | (((uint64_t)(a)) << 8) | (((uint64_t)(b)) << 24)        \
   | (((uint64_t)(c)) << 40))

#define WVM_ENCODE_OP_R_IMM16(op, a, imm)                                     \
  (((uint64_t)(op)) | (((uint64_t)(a)) << 8)                                  \
   | (((uint64_t)(uint16_t)(imm)) << 24))

#define WVM_ENCODE_OP_R_IMM32(op, a, imm)                                     \
  (((uint64_t)(op)) | (((uint64_t)(a)) << 8)                                  \
   | (((uint64_t)(uint32_t)(imm)) << 24))

#define WVM_ENCODE_OP_R_CONST(op, a, k)                                       \
  (((uint64_t)(op)) | (((uint64_t)(a)) << 8)                                  \
   | (((uint64_t)(uint32_t)(k)) << 24))

#define WVM_ENCODE_OP_RR_CONST(op, a, b, k)                                   \
  (((uint64_t)(op)) | (((uint64_t)(a)) << 8) | (((uint64_t)(b)) << 24)        \
   | (((uint64_t)(uint16_t)(k)) << 40))

#define WVM_ENCODE_OP_R_STR(op, a, s)                                         \
  (((uint64_t)(op)) | (((uint64_t)(a)) << 8)                                  \
   | (((uint64_t)(uint32_t)(s)) << 24))

#define WVM_ENCODE_OP_JUMP(op, off)                                           \
  (((uint64_t)(op)) | (((uint64_t)(uint32_t)(off)) << 8))

#define WVM_ENCODE_OP_R_JUMP(op, a, off)                                      \
  (((uint64_t)(op)) | (((uint64_t)(a)) << 8)                                  \
   | (((uint64_t)(uint32_t)(off)) << 24))

#define WVM_ENCODE_OP_RR_JUMP(op, a, b, off)                                  \
  (((uint64_t)(op)) | (((uint64_t)(a)) << 8) | (((uint64_t)(b)) << 24)        \
   | (((uint64_t)(uint16_t)(off)) << 40))

/* Guard encoding: opcode(8) | guard_kind(8) | reg(16) | deopt_offset(32) */
#define WVM_ENCODE_GUARD(op, kind, reg, deopt)                                \
  (((uint64_t)(op)) | (((uint64_t)(kind)) << 8) | (((uint64_t)(reg)) << 16)   \
   | (((uint64_t)(uint32_t)(deopt)) << 32))

/* Decode instructions */
#define WVM_DECODE_OP(instr) ((WVMOpcode)((instr) & 0xFF))
#define WVM_DECODE_R_A(instr) ((WVMReg)(((instr) >> 8) & 0xFFFF))
#define WVM_DECODE_R_B(instr) ((WVMReg)(((instr) >> 24) & 0xFFFF))
#define WVM_DECODE_R_C(instr) ((WVMReg)(((instr) >> 40) & 0xFFFF))
#define WVM_DECODE_IMM16(instr) ((uint16_t)(((instr) >> 24) & 0xFFFF))
#define WVM_DECODE_IMM32(instr) ((uint32_t)(((instr) >> 24) & 0xFFFFFFFF))
#define WVM_DECODE_CONST(instr) ((WVMOffset)(((instr) >> 24) & 0xFFFFFFFF))
#define WVM_DECODE_STR(instr) ((WVMOffset)(((instr) >> 24) & 0xFFFFFFFF))
#define WVM_DECODE_JUMP(instr)                                                \
  ((WVMJumpOffset)(int32_t)(((instr) >> 8) & 0xFFFFFFFF))
#define WVM_DECODE_R_JUMP(instr)                                              \
  ((WVMJumpOffset)(int32_t)(((instr) >> 24) & 0xFFFFFFFF))
#define WVM_DECODE_RR_JUMP(instr) ((int16_t)(((instr) >> 40) & 0xFFFF))
#define WVM_DECODE_GUARD_KIND(instr) ((uint8_t)(((instr) >> 8) & 0xFF))
#define WVM_DECODE_GUARD_REG(instr) ((WVMReg)(((instr) >> 16) & 0xFFFF))
#define WVM_DECODE_GUARD_DEOPT(instr)                                         \
  ((uint32_t)(((instr) >> 32) & 0xFFFFFFFF))

/* ------------------------------------------------------------
   Guard Kinds
   ------------------------------------------------------------ */

typedef enum
{
  /* Type guards */
  WVM_GUARD_TYPE_INT = 0,
  WVM_GUARD_TYPE_FLOAT,
  WVM_GUARD_TYPE_STRING,
  WVM_GUARD_TYPE_BOOL,
  WVM_GUARD_TYPE_TABLE,
  WVM_GUARD_TYPE_FUNCTION,
  WVM_GUARD_TYPE_NIL,
  WVM_GUARD_TYPE_USERDATA,

  /* Conditional guards */
  WVM_GUARD_COND_TRUE,
  WVM_GUARD_COND_FALSE,
  WVM_GUARD_COND_NOT_NIL,
  WVM_GUARD_COND_EQ,
  WVM_GUARD_COND_LT,
  WVM_GUARD_COND_LE,

  /* Range guards */
  WVM_GUARD_RANGE_INT_LO,
  WVM_GUARD_RANGE_INT_HI,
  WVM_GUARD_RANGE_INT_BOTH,

  /* Class/shape guards */
  WVM_GUARD_CLASS,
  WVM_GUARD_SHAPE,
  WVM_GUARD_PROTO,

  /* Resource guards */
  WVM_GUARD_RES_STACK_OVERFLOW,
  WVM_GUARD_RES_MEMORY,
  WVM_GUARD_RES_TIMEOUT,
  WVM_GUARD_RES_RECURSION_DEPTH,

  WVM_GUARD_KIND_COUNT
} WVMGuardKind;

/* ------------------------------------------------------------
   Value Representation
   NaN-boxed: 64-bit, using quiet NaN space for non-float types
   ------------------------------------------------------------ */

typedef uint64_t WVMValue;

/* NaN-boxing constants */
#define WVM_NANBOX_TAG_MASK 0xFFFF000000000000ULL
#define WVM_NANBOX_QNAN 0x7FFC000000000000ULL
#define WVM_NANBOX_TAG_NIL (WVM_NANBOX_QNAN | 0x0001000000000000ULL)
#define WVM_NANBOX_TAG_FALSE (WVM_NANBOX_QNAN | 0x0002000000000000ULL)
#define WVM_NANBOX_TAG_TRUE (WVM_NANBOX_QNAN | 0x0003000000000000ULL)
#define WVM_NANBOX_TAG_INT (WVM_NANBOX_QNAN | 0x0004000000000000ULL)
#define WVM_NANBOX_TAG_OBJ (WVM_NANBOX_QNAN | 0x8000000000000000ULL)

#define WVM_VALUE_NIL WVM_NANBOX_TAG_NIL
#define WVM_VALUE_TRUE WVM_NANBOX_TAG_TRUE
#define WVM_VALUE_FALSE WVM_NANBOX_TAG_FALSE

/* Value creation macros */
#define WVM_VALUE_FROM_FLOAT(f) (*(WVMValue *)&(double){ (f) })
#define WVM_VALUE_FROM_INT(i) (WVM_NANBOX_TAG_INT | ((uint64_t)(uint32_t)(i)))
#define WVM_VALUE_FROM_OBJ(ptr)                                               \
  (WVM_NANBOX_TAG_OBJ | ((uint64_t)(uintptr_t)(ptr) & 0x0000FFFFFFFFFFFFULL))

/* Value query macros */
#define WVM_IS_FLOAT(v) (((v) & WVM_NANBOX_QNAN) != WVM_NANBOX_QNAN)
#define WVM_IS_NIL(v) ((v) == WVM_NANBOX_TAG_NIL)
#define WVM_IS_BOOL(v)                                                        \
  ((v) == WVM_NANBOX_TAG_TRUE || (v) == WVM_NANBOX_TAG_FALSE)
#define WVM_IS_TRUE(v) ((v) == WVM_NANBOX_TAG_TRUE)
#define WVM_IS_FALSE(v) ((v) == WVM_NANBOX_TAG_FALSE)
#define WVM_IS_INT(v) (((v) & WVM_NANBOX_TAG_MASK) == WVM_NANBOX_TAG_INT)
#define WVM_IS_OBJ(v) (((v) & WVM_NANBOX_TAG_OBJ) == WVM_NANBOX_TAG_OBJ)

/* Value extraction macros */
#define WVM_AS_FLOAT(v) (*(double *)&(WVMValue){ (v) })
#define WVM_AS_INT(v) ((int32_t)((v) & 0xFFFFFFFF))
#define WVM_AS_OBJ(v) ((void *)(uintptr_t)((v) & 0x0000FFFFFFFFFFFFULL))
#define WVM_AS_BOOL(v) ((v) == WVM_NANBOX_TAG_TRUE)

/* Truthiness: nil and false are falsy, everything else truthy */
#define WVM_IS_TRUTHY(v) (!WVM_IS_NIL (v) && !WVM_IS_FALSE (v))

/* ------------------------------------------------------------
   Object System (heap-allocated types)
   ------------------------------------------------------------ */

typedef enum
{
  WVM_OBJ_STRING = 0,
  WVM_OBJ_TABLE,
  WVM_OBJ_FUNCTION,
  WVM_OBJ_CLOSURE,
  WVM_OBJ_UPVALUE,
  WVM_OBJ_USERDATA,
  WVM_OBJ_COROUTINE,
  WVM_OBJ_MODULE,
  WVM_OBJ_COUNT
} WVMObjType;

/* Common object header — every heap object starts with this */
typedef struct WVMObj
{
  WVMObjType type;
  uint32_t hash;         /* Cached hash, 0 if not yet computed */
  uint32_t flags;        /* GC mark, pinned, etc. */
  struct WVMObj *gcNext; /* GC linked list */
} WVMObj;

/* String object */
typedef struct
{
  WVMObj header;
  uint32_t length;
  uint32_t hash;
  char data[]; /* Flexible array member */
} WVMString;

/* Upvalue object */
typedef struct WVMUpvalue
{
  WVMObj header;
  WVMValue *location;      /* Points into stack or to 'closed' */
  WVMValue closed;         /* Holds value when closed over */
  struct WVMUpvalue *next; /* Open upvalue linked list */
} WVMUpvalue;

/* Function prototype (bytecode) */
typedef struct WVMPrototype
{
  WVMObj header;
  const char *name;
  const char *source; /* Source filename */
  uint32_t instrCount;
  WVMInstr *instrs; /* Bytecode array */
  uint32_t constCount;
  WVMValue *consts; /* Constant pool */
  uint32_t strCount;
  WVMString **strs; /* String pool */
  uint32_t upvalCount;
  uint8_t *upvalIsLocal; /* 1 if upval captures local */
  uint16_t *upvalIndex;  /* Index in parent or stack */
  uint16_t regCount;     /* Number of registers used */
  uint16_t paramCount;
  uint8_t isVararg;
  uint32_t protoCount;
  struct WVMPrototype **protos; /* Nested function prototypes */
  /* Debug info */
  uint32_t *lineMap; /* Line number per instruction */
} WVMPrototype;

/* Closure (prototype + captured upvalues) */
typedef struct
{
  WVMObj header;
  WVMPrototype *proto;
  uint32_t upvalCount;
  WVMUpvalue *upvals[]; /* Flexible array member */
} WVMClosure;

/* Table (hash map + optional array part) */
typedef struct
{
  WVMObj header;
  uint32_t arrayLen;
  uint32_t arrayCap;
  WVMValue *array;  /* Dense integer-keyed portion */
  uint32_t hashCap; /* Power of 2 */
  uint32_t hashCount;
  WVMValue *hashKeys;
  WVMValue *hashVals;
} WVMTable;

/* Native function callback */
typedef int (*WVMNativeFn) (struct WVMState *vm, int argCount, WVMValue *args);

/* Native function object */
typedef struct
{
  WVMObj header;
  WVMNativeFn fn;
  const char *name;
  uint16_t paramCount; /* 0xFFFF = vararg */
} WVMNativeFunc;

/* Userdata */
typedef struct
{
  WVMObj header;
  void *data;
  size_t size;
  WVMTable *metatable;
  void (*finalizer) (void *data);
} WVMUserdata;

/* Module object */
typedef struct
{
  WVMObj header;
  WVMString *name;
  WVMTable *exports;
  uint8_t loaded;
} WVMModule;

/* ------------------------------------------------------------
   Stack Frame
   ------------------------------------------------------------ */

typedef struct WVMFrame
{
  WVMClosure *closure;   /* Current closure (NULL for native) */
  WVMNativeFunc *native; /* Current native (NULL for bytecode) */
  WVMInstr *ip;          /* Instruction pointer */
  WVMValue *regs;        /* Base of register window */
  uint16_t regCount;
  uint32_t pc;           /* Program counter offset */
  struct WVMFrame *prev; /* Caller frame */
  /* Deoptimization info for JIT frames */
  uint8_t isJIT;     /* Was this frame JIT-compiled? */
  void *jitSideData; /* Opaque JIT metadata for deopt */
} WVMFrame;

/* ------------------------------------------------------------
   Coroutine State
   ------------------------------------------------------------ */

typedef enum
{
  WVM_CORO_DEAD = 0,
  WVM_CORO_READY,
  WVM_CORO_RUNNING,
  WVM_CORO_SUSPENDED,
  WVM_CORO_ERROR
} WVMCoroStatus;

typedef struct WVMCoroutine
{
  WVMObj header;
  WVMCoroStatus status;
  WVMValue *stack;
  uint32_t stackSize;
  uint32_t stackCap;
  WVMFrame *topFrame;
  WVMUpvalue *openUpvals;
  struct WVMState *vm; /* Owning VM */
} WVMCoroutine;

/* ------------------------------------------------------------
   GC Configuration
   ------------------------------------------------------------ */

typedef struct
{
  size_t heapThreshold;         /* Bytes before next collection */
  size_t heapGrowFactor;        /* Numerator/256: grow by factor */
  uint8_t generational;         /* 0=mark-sweep, 1=generational */
  uint8_t incremental;          /* 0=stop-the-world, 1=incremental */
  uint32_t incrementalStepSize; /* Objects per incremental step */
} WVMGCConfig;

/* GC statistics */
typedef struct
{
  size_t totalAllocated;
  size_t totalFreed;
  size_t liveObjects;
  uint64_t collectionCount;
  double lastPauseMs;
  double totalPauseMs;
} WVMGCStats;

/* ------------------------------------------------------------
   JIT Compilation
   ------------------------------------------------------------ */

/* JIT compilation tier */
typedef enum
{
  WVM_JIT_NONE = 0,  /* Interpret only */
  WVM_JIT_BASELINE,  /* Method-based, minimal optimization */
  WVM_JIT_OPTIMIZED, /* Method-based, full optimization */
  WVM_JIT_TRACE      /* Trace-based compilation */
} WVMJITTier;

/* JIT configuration */
typedef struct
{
  uint8_t enabled;
  WVMJITTier maxTier;
  uint32_t baselineThreshold;  /* Call count to trigger baseline */
  uint32_t optimizedThreshold; /* Call count to trigger optimized */
  uint32_t traceHotThreshold;  /* Loop iteration count for trace */
  uint32_t traceMaxLength;     /* Max instructions per trace */
  uint32_t traceMaxSideExits;  /* Max side exits before blacklist */
  uint32_t deoptThreshold;     /* Deopts before blacklisting */
  size_t codeCacheSize;        /* Max machine code cache in bytes */
} WVMJITConfig;

/* Trace recording state */
typedef enum
{
  WVM_TRACE_IDLE = 0,
  WVM_TRACE_RECORDING,
  WVM_TRACE_COMPILING,
  WVM_TRACE_ABORTED
} WVMTraceState;

/* A compiled trace */
typedef struct WVMTrace
{
  uint32_t id;
  WVMPrototype *proto; /* Originating prototype */
  uint32_t startPC;    /* Entry PC in prototype */
  uint32_t instrCount; /* Number of recorded instructions */
  void *machineCode;   /* JIT-compiled native code */
  size_t codeSize;
  uint32_t sideExitCount;
  struct WVMTrace **sideExits; /* Linked side traces */
  uint32_t execCount;          /* Execution counter */
  uint32_t deoptCount;         /* Deoptimization counter */
  uint8_t blacklisted;         /* Too many deopts */
} WVMTrace;

/* Method JIT compiled code */
typedef struct WVMCompiledMethod
{
  WVMPrototype *proto;
  WVMJITTier tier;
  void *machineCode;
  size_t codeSize;
  uint32_t deoptCount;
  uint8_t blacklisted;
  /* On-stack-replacement (OSR) entry points */
  uint32_t osrEntryCount;
  uint32_t *osrPCs;  /* PCs where OSR is possible */
  void **osrEntries; /* Corresponding native entry points */
} WVMCompiledMethod;

/* Deoptimization record — captures JIT state for fallback to interpreter */
typedef struct
{
  WVMPrototype *proto;
  uint32_t pc; /* Bytecode PC to resume at */
  uint32_t regCount;
  WVMValue *regSnapshot; /* Register state at deopt point */
  WVMGuardKind failedGuard;
  uint64_t guardPayload; /* Extra info about the failure */
} WVMDeoptRecord;

/* JIT compiler state (opaque; links to WIL internally) */
typedef struct WVMJITState WVMJITState;

/* Profiling data per prototype */
typedef struct
{
  uint32_t callCount;
  uint32_t *loopCounts; /* Per back-edge iteration count */
  uint32_t loopCountLen;
  uint8_t *typeProfile; /* Per-register observed types (bitset) */
  uint32_t typeProfileLen;
} WVMProfileData;

/* ------------------------------------------------------------
   VM State
   ------------------------------------------------------------ */

typedef struct WVMState
{
  /* Register file / operand stack */
  WVMValue *stack;
  uint32_t stackSize;
  uint32_t stackCap;

  /* Control stack */
  WVMFrame *topFrame;
  uint32_t frameDepth;
  uint32_t maxFrameDepth;

  /* Open upvalues list */
  WVMUpvalue *openUpvals;

  /* Global environment */
  WVMTable *globals;

  /* Module registry */
  WVMTable *modules;
  const char **modulePaths;
  uint32_t modulePathCount;

  /* Coroutine */
  WVMCoroutine *mainCoro;
  WVMCoroutine *activeCoro;

  /* GC state */
  WVMObj *gcHead; /* All objects linked list */
  WVMGCConfig gcConfig;
  WVMGCStats gcStats;
  size_t bytesAllocated;
  uint8_t gcRunning;

  /* JIT state */
  WVMJITConfig jitConfig;
  WVMJITState *jitState; /* Opaque, links to WIL/MAL/MDS */

  /* Error handling */
  const char *lastError;
  int hasError;
  void (*errorHandler) (struct WVMState *vm, const char *msg);

  /* Allocation hooks */
  void *(*allocFn) (void *ud, void *ptr, size_t oldSize, size_t newSize);
  void *allocUD;

  /* User data */
  void *userData;
} WVMState;

/* ------------------------------------------------------------
   VM Lifecycle
   ------------------------------------------------------------ */

/* Create a new VM with default settings */
WVMState *wvmCreate (void);

/* Create a VM with custom allocator */
WVMState *wvmCreateEx (void *(*allocFn) (void *ud, void *ptr, size_t oldSize,
                                         size_t newSize),
                       void *allocUD);

/* Destroy the VM and free all resources */
void wvmDestroy (WVMState *vm);

/* Configure GC */
void wvmSetGCConfig (WVMState *vm, const WVMGCConfig *config);
void wvmGetGCConfig (const WVMState *vm, WVMGCConfig *out);
void wvmGetGCStats (const WVMState *vm, WVMGCStats *out);
void wvmForceGC (WVMState *vm);

/* Configure JIT */
void wvmSetJITConfig (WVMState *vm, const WVMJITConfig *config);
void wvmGetJITConfig (const WVMState *vm, WVMJITConfig *out);

/* Error handling */
void wvmSetErrorHandler (WVMState *vm,
                         void (*handler) (WVMState *vm, const char *msg));
const char *wvmGetLastError (const WVMState *vm);
void wvmClearError (WVMState *vm);

/* User data */
void wvmSetUserData (WVMState *vm, void *ud);
void *wvmGetUserData (const WVMState *vm);

/* ------------------------------------------------------------
   Bytecode Loading & Execution
   ------------------------------------------------------------ */

/* Load a prototype from a bytecode byte array */
WVMPrototype *wvmLoadBytecode (WVMState *vm, const uint8_t *data, size_t len);

/* Load from file */
WVMPrototype *wvmLoadFile (WVMState *vm, const char *path);

/* Load from source string (compiles to bytecode first) */
WVMPrototype *wvmLoadString (WVMState *vm, const char *source,
                             const char *name);

/* Free a prototype (if not managed by GC) */
void wvmFreePrototype (WVMState *vm, WVMPrototype *proto);

/* Execute a closure, returns number of return values */
int wvmCall (WVMState *vm, WVMClosure *closure, int argCount, WVMValue *args,
             WVMValue *results, int maxResults);

/* Execute a prototype (wraps in closure automatically) */
int wvmExecute (WVMState *vm, WVMPrototype *proto);

/* Step the interpreter by N instructions (for debugging) */
int wvmStep (WVMState *vm, uint32_t count);

/* Resume a suspended coroutine */
int wvmResume (WVMState *vm, WVMCoroutine *coro, int argCount, WVMValue *args,
               WVMValue *results, int maxResults);

/* Yield the current coroutine */
int wvmYield (WVMState *vm, int resultCount, WVMValue *results);

/* ------------------------------------------------------------
   Global Environment
   ------------------------------------------------------------ */

void wvmSetGlobal (WVMState *vm, const char *name, WVMValue val);
WVMValue wvmGetGlobal (const WVMState *vm, const char *name);
int wvmHasGlobal (const WVMState *vm, const char *name);
void wvmRemoveGlobal (WVMState *vm, const char *name);

/* Register a native function as a global */
void wvmRegisterNative (WVMState *vm, const char *name, WVMNativeFn fn,
                        uint16_t paramCount);

/* ------------------------------------------------------------
   Module System
   ------------------------------------------------------------ */

void wvmAddModulePath (WVMState *vm, const char *path);
WVMModule *wvmLoadModule (WVMState *vm, const char *name);
WVMModule *wvmLoadModuleFromFile (WVMState *vm, const char *name,
                                  const char *path);
int wvmRequire (WVMState *vm, const char *name, WVMValue *result);

/* ------------------------------------------------------------
   JIT API
   ------------------------------------------------------------ */

/* Manually trigger JIT compilation of a prototype */
int wvmJITCompile (WVMState *vm, WVMPrototype *proto, WVMJITTier tier);

/* Invalidate JIT code for a prototype */
void wvmJITInvalidate (WVMState *vm, WVMPrototype *proto);

/* Flush the entire code cache */
void wvmJITFlush (WVMState *vm);

/* Query JIT status of a prototype */
WVMJITTier wvmJITGetTier (const WVMState *vm, const WVMPrototype *proto);

/* Get profiling data for a prototype */
const WVMProfileData *wvmGetProfileData (const WVMState *vm,
                                         const WVMPrototype *proto);

/* Deoptimize: fall back from JIT to interpreter for current frame */
void wvmDeoptimize (WVMState *vm, const WVMDeoptRecord *record);

/* Query trace information */
uint32_t wvmTraceCount (const WVMState *vm);
const WVMTrace *wvmGetTrace (const WVMState *vm, uint32_t id);

/* ------------------------------------------------------------
   Archive and Binary Formats
   ------------------------------------------------------------ */

/* File format magic numbers */
#define WVM_MAGIC_WVMO 0x57564D4F /* 'WVMO' — bytecode object */
#define WVM_MAGIC_WVMA 0x57564D41 /* 'WVMA' — archive (bundle) */
#define WVM_MAGIC_WVMX 0x57564D58 /* 'WVMX' — executable + embedded runtime \
                                   */
#define WVM_MAGIC_WVMC 0x57564D43 /* 'WVMC' — AOT compiled */

/* File format version */
#define WVM_FORMAT_VERSION_MAJOR 1
#define WVM_FORMAT_VERSION_MINOR 0

/* File header (common to all formats) */
typedef struct
{
  uint32_t magic;
  uint16_t versionMajor;
  uint16_t versionMinor;
  uint32_t flags;
  uint32_t sectionCount;
  uint64_t timestamp;   /* Unix epoch */
  uint8_t checksum[32]; /* SHA-256 of payload */
} WVMFileHeader;

/* Section types within archive */
typedef enum
{
  WVM_SECTION_CODE = 0,  /* Bytecode */
  WVM_SECTION_CONST,     /* Constant pool */
  WVM_SECTION_STRING,    /* String pool */
  WVM_SECTION_DEBUG,     /* Debug info (line maps, local names) */
  WVM_SECTION_MANIFEST,  /* Metadata / manifest */
  WVM_SECTION_LINK,      /* Import/export tables */
  WVM_SECTION_NATIVE,    /* Embedded native code (AOT) */
  WVM_SECTION_RUNTIME,   /* Embedded runtime (for .wvmx) */
  WVM_SECTION_SHEBANG,   /* Shebang stub (for .wvmx on Unix) */
  WVM_SECTION_RESOURCE,  /* Embedded resources */
  WVM_SECTION_SIGNATURE, /* Cryptographic signature */
  WVM_SECTION_COUNT
} WVMSectionType;

typedef struct
{
  WVMSectionType type;
  uint32_t flags;
  uint64_t offset;           /* Byte offset in file */
  uint64_t size;             /* Byte size */
  uint64_t uncompressedSize; /* If compressed */
  uint8_t compression;       /* 0=none, 1=zstd, 2=lz4, 3=deflate */
} WVMSectionHeader;

/* Manifest entry (key-value metadata) */
typedef struct
{
  const char *key;
  const char *value;
} WVMManifestEntry;

typedef struct
{
  uint32_t entryCount;
  WVMManifestEntry *entries;
} WVMManifest;

/* Link table (imports and exports) */
typedef struct
{
  const char *name;
  uint32_t kind;  /* 0=function, 1=global, 2=module */
  uint32_t index; /* Index into relevant pool */
} WVMLinkEntry;

typedef struct
{
  uint32_t importCount;
  WVMLinkEntry *imports;
  uint32_t exportCount;
  WVMLinkEntry *exports;
} WVMLinkTable;

/* ------------------------------------------------------------
   Archive I/O
   ------------------------------------------------------------ */

/* Opaque archive handle */
typedef struct WVMArchive WVMArchive;

/* Open / Create */
WVMArchive *wvmArchiveOpen (WVMState *vm, const char *path);
WVMArchive *wvmArchiveOpenMemory (WVMState *vm, const uint8_t *data,
                                  size_t len);
WVMArchive *wvmArchiveCreate (WVMState *vm);

/* Query */
const WVMFileHeader *wvmArchiveGetHeader (const WVMArchive *ar);
uint32_t wvmArchiveSectionCount (const WVMArchive *ar);
const WVMSectionHeader *wvmArchiveGetSection (const WVMArchive *ar,
                                              uint32_t index);
const WVMSectionHeader *wvmArchiveFindSection (const WVMArchive *ar,
                                               WVMSectionType type);
const WVMManifest *wvmArchiveGetManifest (const WVMArchive *ar);
const WVMLinkTable *wvmArchiveGetLinkTable (const WVMArchive *ar);

/* Extract section data (caller must free with wvmFree) */
uint8_t *wvmArchiveExtractSection (WVMState *vm, const WVMArchive *ar,
                                   uint32_t sectionIndex, size_t *outLen);

/* Load all prototypes from archive */
uint32_t wvmArchivePrototypeCount (const WVMArchive *ar);
WVMPrototype *wvmArchiveLoadPrototype (WVMState *vm, WVMArchive *ar,
                                       uint32_t index);

/* Build archive */
void wvmArchiveAddSection (WVMArchive *ar, WVMSectionType type,
                           const uint8_t *data, size_t len,
                           uint8_t compression);
void wvmArchiveSetManifest (WVMArchive *ar, const WVMManifest *manifest);
void wvmArchiveSetLinkTable (WVMArchive *ar, const WVMLinkTable *links);
void wvmArchiveAddPrototype (WVMArchive *ar, const WVMPrototype *proto);

/* Write to file or memory buffer */
int wvmArchiveWriteFile (WVMState *vm, WVMArchive *ar, const char *path);
uint8_t *wvmArchiveWriteMemory (WVMState *vm, WVMArchive *ar, size_t *outLen);

/* Close */
void wvmArchiveClose (WVMState *vm, WVMArchive *ar);

/* ------------------------------------------------------------
   .wvmo — Bytecode Object
   Single-module bytecode file with optional debug info.
   ------------------------------------------------------------ */

/* Write a single prototype to .wvmo */
int wvmWriteObject (WVMState *vm, const WVMPrototype *proto, const char *path);
int wvmWriteObjectMemory (WVMState *vm, const WVMPrototype *proto,
                          uint8_t **outBuf, size_t *outLen);

/* Read a .wvmo back */
WVMPrototype *wvmReadObject (WVMState *vm, const char *path);
WVMPrototype *wvmReadObjectMemory (WVMState *vm, const uint8_t *data,
                                   size_t len);

/* ------------------------------------------------------------
   .wvma — Archive (Bundle)
   Multiple modules, resources, and metadata.
   ------------------------------------------------------------ */

/* Convenience: bundle multiple .wvmo files into a .wvma */
int wvmBundleCreate (WVMState *vm, const char *outputPath,
                     const char **inputPaths, uint32_t inputCount,
                     const WVMManifest *manifest);

/* Load and link all modules from a .wvma */
int wvmBundleLoad (WVMState *vm, const char *path);

/* ------------------------------------------------------------
   .wvmx — Executable with Embedded Runtime
   Self-contained executable: shebang stub + runtime + bytecode.
   On Unix, the shebang line allows `./program.wvmx` execution.
   ------------------------------------------------------------ */

/* Shebang stub configuration */
typedef struct
{
  const char *interpreterPath; /* e.g. "/usr/bin/env wvm" */
  uint8_t includeRuntime;      /* 1 = embed full runtime */
  uint8_t stripDebug;          /* 1 = remove debug sections */
  uint8_t compression;         /* Compression for payload */
} WVMExeConfig;

/* Create a .wvmx from a .wvma or .wvmo */
int wvmExeCreate (WVMState *vm, const char *inputPath, const char *outputPath,
                  const WVMExeConfig *config);

/* Create a .wvmx from an in-memory archive */
int wvmExeCreateFromArchive (WVMState *vm, WVMArchive *ar,
                             const char *outputPath,
                             const WVMExeConfig *config);

/* Extract the bytecode payload from a .wvmx */
WVMArchive *wvmExeExtract (WVMState *vm, const char *exePath);

/* Check if a file is a valid .wvmx */
int wvmExeValidate (WVMState *vm, const char *path);

/* ------------------------------------------------------------
   .wvmc — AOT Compiled
   Ahead-of-time compiled native code + bytecode fallback.
   ------------------------------------------------------------ */

typedef enum
{
  WVM_AOT_TARGET_X86_64 = 0,
  WVM_AOT_TARGET_AARCH64,
  WVM_AOT_TARGET_RISCV64,
  WVM_AOT_TARGET_WASM32,
  WVM_AOT_TARGET_WASM64,
  WVM_AOT_TARGET_COUNT
} WVMAOTTarget;

typedef struct
{
  WVMAOTTarget target;
  uint8_t optLevel;        /* 0-3 */
  uint8_t includeFallback; /* 1 = embed bytecode for deopt */
  uint8_t stripDebug;
  uint8_t pic;             /* Position-independent code */
  const char *cpuFeatures; /* Target-specific feature string */
} WVMAOTConfig;

/* Compile a prototype or archive to AOT native code */
int wvmAOTCompile (WVMState *vm, WVMPrototype *proto, const char *outputPath,
                   const WVMAOTConfig *config);
int wvmAOTCompileArchive (WVMState *vm, WVMArchive *ar, const char *outputPath,
                          const WVMAOTConfig *config);

/* Load a .wvmc and register its native code with the JIT */
int wvmAOTLoad (WVMState *vm, const char *path);

/* Query AOT target of a .wvmc file */
WVMAOTTarget wvmAOTGetTarget (WVMState *vm, const char *path);

/* ------------------------------------------------------------
   Linking
   ------------------------------------------------------------ */

/* Opaque linker state */
typedef struct WVMLinker WVMLinker;

WVMLinker *wvmLinkerCreate (WVMState *vm);
void wvmLinkerDestroy (WVMState *vm, WVMLinker *linker);

/* Add modules to the linker */
int wvmLinkerAddObject (WVMLinker *linker, const char *path);
int wvmLinkerAddArchive (WVMLinker *linker, const char *path);
int wvmLinkerAddPrototype (WVMLinker *linker, const char *name,
                           WVMPrototype *proto);
int wvmLinkerAddNative (WVMLinker *linker, const char *name, WVMNativeFn fn,
                        uint16_t paramCount);

/* Resolve all imports against exports */
int wvmLinkerResolve (WVMLinker *linker);

/* Query unresolved symbols */
uint32_t wvmLinkerUnresolvedCount (const WVMLinker *linker);
const char *wvmLinkerUnresolvedName (const WVMLinker *linker, uint32_t index);

/* Emit linked output */
int wvmLinkerEmitObject (WVMLinker *linker, const char *outputPath);
int wvmLinkerEmitArchive (WVMLinker *linker, const char *outputPath,
                          const WVMManifest *manifest);

/* Load linked result directly into VM */
int wvmLinkerLoadInto (WVMLinker *linker, WVMState *vm);

/* ------------------------------------------------------------
   Dynamic Loading (at runtime)
   ------------------------------------------------------------ */

/* Load a shared native library (.so/.dll/.dylib) */
typedef struct WVMDynLib WVMDynLib;

WVMDynLib *wvmDynOpen (WVMState *vm, const char *path);
void wvmDynClose (WVMState *vm, WVMDynLib *lib);
void *wvmDynSymbol (WVMDynLib *lib, const char *name);

/* Register all exported WVM-compatible functions from a dynlib */
int wvmDynRegisterAll (WVMState *vm, WVMDynLib *lib);

/* ------------------------------------------------------------
   Serialization Utilities
   ------------------------------------------------------------ */

/* Opaque byte buffer for serialization */
typedef struct WVMBuffer
{
  uint8_t *data;
  size_t len;
  size_t cap;
} WVMBuffer;

WVMBuffer wvmBufferCreate (WVMState *vm, size_t initialCap);
void wvmBufferDestroy (WVMState *vm, WVMBuffer *buf);
void wvmBufferWriteU8 (WVMBuffer *buf, uint8_t val);
void wvmBufferWriteU16 (WVMBuffer *buf, uint16_t val);
void wvmBufferWriteU32 (WVMBuffer *buf, uint32_t val);
void wvmBufferWriteU64 (WVMBuffer *buf, uint64_t val);
void wvmBufferWriteI32 (WVMBuffer *buf, int32_t val);
void wvmBufferWriteF64 (WVMBuffer *buf, double val);
void wvmBufferWriteBytes (WVMBuffer *buf, const uint8_t *data, size_t len);
void wvmBufferWriteString (WVMBuffer *buf, const char *str);

/* Reader (cursor over a byte array) */
typedef struct WVMReader
{
  const uint8_t *data;
  size_t len;
  size_t pos;
} WVMReader;

WVMReader wvmReaderCreate (const uint8_t *data, size_t len);
uint8_t wvmReaderReadU8 (WVMReader *r);
uint16_t wvmReaderReadU16 (WVMReader *r);
uint32_t wvmReaderReadU32 (WVMReader *r);
uint64_t wvmReaderReadU64 (WVMReader *r);
int32_t wvmReaderReadI32 (WVMReader *r);
double wvmReaderReadF64 (WVMReader *r);
int wvmReaderReadBytes (WVMReader *r, uint8_t *out, size_t len);
const char *wvmReaderReadString (WVMReader *r, uint32_t *outLen);
int wvmReaderEOF (const WVMReader *r);

/* ------------------------------------------------------------
   Memory Allocation Helpers
   ------------------------------------------------------------ */

void *wvmAlloc (WVMState *vm, size_t size);
void *wvmRealloc (WVMState *vm, void *ptr, size_t oldSize, size_t newSize);
void wvmFree (WVMState *vm, void *ptr, size_t size);

/* Allocate a GC-managed object */
WVMObj *wvmAllocObj (WVMState *vm, WVMObjType type, size_t size);

/* Pin/unpin an object to prevent GC collection */
void wvmPinObj (WVMObj *obj);
void wvmUnpinObj (WVMObj *obj);

/* ------------------------------------------------------------
   String Interning
   ------------------------------------------------------------ */

WVMString *wvmInternString (WVMState *vm, const char *str, uint32_t len);
WVMString *wvmInternCString (WVMState *vm, const char *str);
int wvmStringEqual (const WVMString *a, const WVMString *b);
uint32_t wvmStringHash (const WVMString *s);

/* ------------------------------------------------------------
   Table API
   ------------------------------------------------------------ */

WVMTable *wvmTableCreate (WVMState *vm);
void wvmTableDestroy (WVMState *vm, WVMTable *t);
WVMValue wvmTableGet (const WVMTable *t, WVMValue key);
void wvmTableSet (WVMState *vm, WVMTable *t, WVMValue key, WVMValue val);
int wvmTableHas (const WVMTable *t, WVMValue key);
void wvmTableRemove (WVMState *vm, WVMTable *t, WVMValue key);
uint32_t wvmTableLength (const WVMTable *t);

/* Iteration */
typedef struct
{
  uint32_t arrayIdx;
  uint32_t hashIdx;
} WVMTableIter;

WVMTableIter wvmTableIterBegin (void);
int wvmTableIterNext (const WVMTable *t, WVMTableIter *iter, WVMValue *key,
                      WVMValue *val);

/* ------------------------------------------------------------
   Stack Manipulation (for native functions)
   ------------------------------------------------------------ */

void wvmPush (WVMState *vm, WVMValue val);
WVMValue wvmPop (WVMState *vm);
WVMValue wvmPeek (const WVMState *vm, int offset); /* 0 = top */
void wvmSetReg (WVMState *vm, WVMReg reg, WVMValue val);
WVMValue wvmGetReg (const WVMState *vm, WVMReg reg);
uint32_t wvmStackSize (const WVMState *vm);
void wvmEnsureStack (WVMState *vm, uint32_t extra);

/* ------------------------------------------------------------
   Debugging & Introspection
   ------------------------------------------------------------ */

/* Disassemble a prototype to a string (caller frees) */
char *wvmDisassemble (WVMState *vm, const WVMPrototype *proto);

/* Disassemble a single instruction */
char *wvmDisassembleInstr (WVMInstr instr);

/* Print a value to a string (caller frees) */
char *wvmValueToString (WVMState *vm, WVMValue val);

/* Type name of a value */
const char *wvmValueTypeName (WVMValue val);

/* Stack trace (caller frees) */
char *wvmStackTrace (const WVMState *vm);

/* Frame inspection */
uint32_t wvmFrameDepth (const WVMState *vm);
const WVMFrame *wvmGetFrame (const WVMState *vm, uint32_t depth); /* 0 = top */
const char *wvmFrameFunctionName (const WVMFrame *frame);
const char *wvmFrameSourceFile (const WVMFrame *frame);
uint32_t wvmFrameLine (const WVMFrame *frame);

/* Breakpoint support */
typedef void (*WVMBreakpointFn) (WVMState *vm, WVMPrototype *proto,
                                 uint32_t pc, void *ud);

uint32_t wvmSetBreakpoint (WVMState *vm, WVMPrototype *proto, uint32_t pc,
                           WVMBreakpointFn fn, void *ud);
void wvmRemoveBreakpoint (WVMState *vm, uint32_t breakpointId);
void wvmRemoveAllBreakpoints (WVMState *vm);

/* Instruction-level tracing callback */
typedef void (*WVMTraceFn) (WVMState *vm, WVMInstr instr, uint32_t pc,
                            void *ud);

void wvmSetTraceCallback (WVMState *vm, WVMTraceFn fn, void *ud);

/* Profiling hooks */
typedef void (*WVMProfileCallFn) (WVMState *vm, WVMPrototype *proto,
                                  int isEntry, void *ud);

void wvmSetProfileCallback (WVMState *vm, WVMProfileCallFn fn, void *ud);

/* Dump VM state to FILE* */
void wvmDumpState (const WVMState *vm, void *file);

/* Dump JIT statistics */
void wvmDumpJITStats (const WVMState *vm, void *file);

/* Dump GC statistics */
void wvmDumpGCStats (const WVMState *vm, void *file);

/* ------------------------------------------------------------
   Instruction Helpers
   ------------------------------------------------------------ */

/* Get the encoding type for an opcode */
WVMEncodingType wvmGetEncoding (WVMOpcode op);

/* Get the human-readable name of an opcode */
const char *wvmOpcodeName (WVMOpcode op);

/* Get the number of register operands for an opcode */
int wvmOpcodeRegCount (WVMOpcode op);

/* Check if an opcode is a branch/jump */
int wvmOpcodeIsBranch (WVMOpcode op);

/* Check if an opcode is a guard */
int wvmOpcodeIsGuard (WVMOpcode op);

/* Check if an opcode has a side effect */
int wvmOpcodeHasSideEffect (WVMOpcode op);

/* Get the guard kind name */
const char *wvmGuardKindName (WVMGuardKind kind);

/* ------------------------------------------------------------
   Validation & Verification
   ------------------------------------------------------------ */

/* Verify bytecode integrity (type-checks, bounds, etc.) */
int wvmVerifyPrototype (WVMState *vm, const WVMPrototype *proto);

/* Verify an archive file */
int wvmVerifyFile (WVMState *vm, const char *path);

/* Verify checksum of a file header */
int wvmVerifyChecksum (const WVMFileHeader *header, const uint8_t *payload,
                       size_t payloadLen);

/* ------------------------------------------------------------
   Version & Build Info
   ------------------------------------------------------------ */

#define WVM_VERSION_MAJOR 0
#define WVM_VERSION_MINOR 1
#define WVM_VERSION_PATCH 0
#define WVM_VERSION_STRING "0.1.0"

const char *wvmVersion (void);
const char *wvmBuildInfo (void); /* Compiler, date, flags */

/* Feature flags (compile-time) */
#define WVM_FEATURE_JIT (1 << 0)
#define WVM_FEATURE_AOT (1 << 1)
#define WVM_FEATURE_DEBUG (1 << 2)
#define WVM_FEATURE_PROFILING (1 << 3)
#define WVM_FEATURE_COROUTINE (1 << 4)

uint32_t wvmFeatures (void);

WIRBLE_END_DECLS

WIRBLE_BEGIN_DECLS
#endif /* WIRBLE_WVM_H */
