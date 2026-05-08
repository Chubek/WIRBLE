# Chapter 6: WVM (WIRBLE Virtual Machine)

## Overview

WVM (WIRBLE Virtual Machine) is a full-featured bytecode virtual machine designed for embedding and runtime code execution. It supports JIT compilation, runtime optimization, and can be used to extend host applications with scripting capabilities.

## Design Philosophy

WVM is designed with:

1. **Register-Based**: Uses registers instead of stack for better performance
2. **Compact Encoding**: 64-bit instruction encoding
3. **JIT-Ready**: Designed for efficient JIT compilation
4. **Embeddable**: Easy to integrate into host applications
5. **Type-Flexible**: Dynamic typing with efficient representation
6. **Optimization-Friendly**: Supports guards, profiling, and specialization

## Instruction Encoding

### WVMInstr

All WVM instructions fit in 64 bits:

```c
typedef uint64_t WVMInstr;
typedef uint8_t WVMOpcode;
typedef uint16_t WVMReg;           // Register index (0-65535)
typedef uint32_t WVMOffset;        // Offset into constant/string pool
typedef int32_t WVMJumpOffset;     // Signed jump offset
```

### Encoding Formats

```c
typedef enum {
    WVM_ENC_OP,          // Opcode only (8 bits)
    WVM_ENC_OP_R,        // Opcode + 1 register (8 + 16 bits)
    WVM_ENC_OP_RR,       // Opcode + 2 registers (8 + 16 + 16 bits)
    WVM_ENC_OP_RRR,      // Opcode + 3 registers (8 + 16 + 16 + 16 bits)
    WVM_ENC_OP_R_IMM16,  // Opcode + register + 16-bit immediate
    WVM_ENC_OP_R_IMM32,  // Opcode + register + 32-bit immediate
    WVM_ENC_OP_R_CONST,  // Opcode + register + constant pool offset
    WVM_ENC_OP_RR_CONST, // Opcode + 2 registers + constant pool offset
    WVM_ENC_OP_R_STR,    // Opcode + register + string pool offset
    WVM_ENC_OP_JUMP,     // Opcode + signed jump offset (32 bits)
    WVM_ENC_OP_R_JUMP,   // Opcode + register + jump offset
    WVM_ENC_OP_RR_JUMP,  // Opcode + 2 registers + jump offset
    WVM_ENC_OP_GUARD     // Opcode + guard type + register + jump offset
} WVMEncodingType;
```

## Opcode Categories

### Control Flow

```c
WVM_OP_NOP = 0,        // No operation
WVM_OP_HALT,           // Halt execution
WVM_OP_RETURN,         // Return from function
WVM_OP_CALL,           // Call function
WVM_OP_TAILCALL,       // Tail call
WVM_OP_JUMP,           // Unconditional jump
WVM_OP_JUMP_IF_TRUE,   // Jump if true
WVM_OP_JUMP_IF_FALSE,  // Jump if false
```

### Register Operations

```c
WVM_OP_MOVE,           // R[A] = R[B]
WVM_OP_LOAD_CONST,     // R[A] = K[B]
WVM_OP_LOAD_IMM,       // R[A] = immediate
WVM_OP_LOAD_NIL,       // R[A] = nil
WVM_OP_LOAD_TRUE,      // R[A] = true
WVM_OP_LOAD_FALSE,     // R[A] = false
```

### Arithmetic

```c
WVM_OP_ADD,            // R[A] = R[B] + R[C]
WVM_OP_SUB,            // R[A] = R[B] - R[C]
WVM_OP_MUL,            // R[A] = R[B] * R[C]
WVM_OP_DIV,            // R[A] = R[B] / R[C]
WVM_OP_MOD,            // R[A] = R[B] % R[C]
WVM_OP_NEG,            // R[A] = -R[B]
WVM_OP_INC,            // R[A]++
WVM_OP_DEC,            // R[A]--
```

### Bitwise

```c
WVM_OP_AND,            // Bitwise AND
WVM_OP_OR,             // Bitwise OR
WVM_OP_XOR,            // Bitwise XOR
WVM_OP_NOT,            // Bitwise NOT
WVM_OP_SHL,            // Shift left
WVM_OP_SHR,            // Shift right
```

### Comparison

```c
WVM_OP_EQ,             // R[A] = R[B] == R[C]
WVM_OP_NE,             // Not equal
WVM_OP_LT,             // Less than
WVM_OP_LE,             // Less or equal
WVM_OP_GT,             // Greater than
WVM_OP_GE,             // Greater or equal
```

### Memory Operations

```c
WVM_OP_LOAD_GLOBAL,    // R[A] = globals[K[B]]
WVM_OP_STORE_GLOBAL,   // globals[K[A]] = R[B]
WVM_OP_LOAD_UPVAL,     // R[A] = upvalues[B]
WVM_OP_STORE_UPVAL,    // upvalues[A] = R[B]
WVM_OP_LOAD_FIELD,     // R[A] = R[B][K[C]]
WVM_OP_STORE_FIELD,    // R[A][K[B]] = R[C]
```

### Table Operations

```c
WVM_OP_NEW_TABLE,      // R[A] = {}
WVM_OP_GET_INDEX,      // R[A] = R[B][R[C]]
WVM_OP_SET_INDEX,      // R[A][R[B]] = R[C]
WVM_OP_TABLE_LEN,      // R[A] = #R[B]
```

### Type Operations

```c
WVM_OP_TYPE,           // R[A] = type(R[B])
WVM_OP_IS_NIL,         // R[A] = R[B] == nil
WVM_OP_IS_BOOL,        // R[A] = is_bool(R[B])
WVM_OP_IS_NUMBER,      // R[A] = is_number(R[B])
WVM_OP_IS_STRING,      // R[A] = is_string(R[B])
WVM_OP_IS_TABLE,       // R[A] = is_table(R[B])
WVM_OP_IS_FUNCTION,    // R[A] = is_function(R[B])
```

### Guards (for JIT)

```c
WVM_OP_GUARD_TYPE,     // Guard that R[A] has specific type
WVM_OP_GUARD_VALUE,    // Guard that R[A] has specific value
WVM_OP_GUARD_CLASS,    // Guard that R[A] has specific class
WVM_OP_GUARD_RANGE,    // Guard that R[A] is in range
```

## WVM State

### WVMState Structure

```c
struct WVMState {
    // Registers
    WVMValue *registers;
    uint32_t registerCount;
    
    // Instruction pointer
    WVMInstr *ip;
    
    // Call stack
    struct WVMCallFrame *frames;
    uint32_t frameCount;
    uint32_t frameCapacity;
    
    // Constant pools
    WVMValue *constants;
    uint32_t constantCount;
    const char **strings;
    uint32_t stringCount;
    
    // Global variables
    struct WVMTable *globals;
    
    // Memory allocator
    void *(*allocFunc)(void *ud, void *ptr, size_t oldSize, size_t newSize);
    void *allocUserData;
    
    // JIT state
    struct WVMJITState *jit;
    
    // Error handling
    int errorCode;
    const char *errorMessage;
};
```

### Creating VM State

```c
struct WVMState *wvmStateCreate(void);
void wvmStateDestroy(struct WVMState *state);
```

### Custom Allocator

```c
void wvmStateSetAllocator(struct WVMState *state,
                          void *(*allocFunc)(void *, void *, size_t, size_t),
                          void *userData);
```

## Value Representation

### WVMValue

WVM uses tagged values for dynamic typing:

```c
typedef struct WVMValue {
    union {
        double number;
        int64_t integer;
        void *pointer;
        struct {
            uint32_t tag;
            uint32_t payload;
        } tagged;
    } data;
    uint8_t type;
} WVMValue;
```

### Value Types

```c
typedef enum WVMType {
    WVM_TYPE_NIL = 0,
    WVM_TYPE_BOOL,
    WVM_TYPE_NUMBER,
    WVM_TYPE_INTEGER,
    WVM_TYPE_STRING,
    WVM_TYPE_TABLE,
    WVM_TYPE_FUNCTION,
    WVM_TYPE_USERDATA,
    WVM_TYPE_THREAD
} WVMType;
```

### Value Constructors

```c
WVMValue wvmNil(void);
WVMValue wvmBool(int value);
WVMValue wvmNumber(double value);
WVMValue wvmInteger(int64_t value);
WVMValue wvmString(struct WVMState *state, const char *str);
WVMValue wvmTable(struct WVMState *state);
```

### Value Accessors

```c
int wvmIsNil(WVMValue value);
int wvmIsBool(WVMValue value);
int wvmIsNumber(WVMValue value);
int wvmIsString(WVMValue value);

int wvmToBool(WVMValue value);
double wvmToNumber(WVMValue value);
int64_t wvmToInteger(WVMValue value);
const char *wvmToString(WVMValue value);
```

## Bytecode Compilation

### Chunk Structure

```c
typedef struct WVMChunk {
    WVMInstr *instructions;
    uint32_t instructionCount;
    uint32_t instructionCapacity;
    
    WVMValue *constants;
    uint32_t constantCount;
    
    const char **strings;
    uint32_t stringCount;
    
    // Debug info
    uint32_t *lineNumbers;
    const char *sourceName;
} WVMChunk;
```

### Creating Chunks

```c
WVMChunk *wvmChunkCreate(void);
void wvmChunkDestroy(WVMChunk *chunk);
```

### Emitting Instructions

```c
void wvmChunkEmit(WVMChunk *chunk, WVMInstr instr);
void wvmChunkEmitOp(WVMChunk *chunk, WVMOpcode op);
void wvmChunkEmitOpR(WVMChunk *chunk, WVMOpcode op, WVMReg r);
void wvmChunkEmitOpRR(WVMChunk *chunk, WVMOpcode op, WVMReg r1, WVMReg r2);
void wvmChunkEmitOpRRR(WVMChunk *chunk, WVMOpcode op, WVMReg r1, WVMReg r2, WVMReg r3);
```

### Adding Constants

```c
uint32_t wvmChunkAddConstant(WVMChunk *chunk, WVMValue value);
uint32_t wvmChunkAddString(WVMChunk *chunk, const char *str);
```

## Execution

### Interpreter

```c
int wvmExecute(struct WVMState *state, WVMChunk *chunk);
```

### Call Frames

```c
typedef struct WVMCallFrame {
    WVMInstr *returnIP;
    WVMReg *registers;
    uint32_t registerBase;
    WVMChunk *chunk;
} WVMCallFrame;
```

### Function Calls

```c
int wvmCall(struct WVMState *state, WVMValue func, WVMValue *args, uint32_t argCount);
WVMValue wvmGetReturnValue(struct WVMState *state);
```

## JIT Compilation

### JIT State

```c
typedef struct WVMJITState {
    int enabled;
    uint32_t hotThreshold;         // Executions before JIT
    
    // Compiled traces
    struct WVMTrace *traces;
    uint32_t traceCount;
    
    // Profiling data
    uint32_t *executionCounts;
    
    // Code cache
    void *codeCache;
    size_t codeCacheSize;
} WVMJITState;
```

### Enabling JIT

```c
void wvmEnableJIT(struct WVMState *state);
void wvmDisableJIT(struct WVMState *state);
void wvmSetJITThreshold(struct WVMState *state, uint32_t threshold);
```

### Trace Recording

```c
typedef struct WVMTrace {
    uint32_t id;
    WVMInstr *startIP;
    WVMInstr *instructions;
    uint32_t instructionCount;
    
    // Guards
    struct WVMGuard *guards;
    uint32_t guardCount;
    
    // Compiled code
    void *nativeCode;
    size_t nativeCodeSize;
} WVMTrace;
```

### Compiling Traces

```c
int wvmCompileTrace(struct WVMState *state, WVMTrace *trace);
```

## Guards and Deoptimization

### Guard Types

```c
typedef enum WVMGuardType {
    WVM_GUARD_TYPE,        // Type guard
    WVM_GUARD_VALUE,       // Value guard
    WVM_GUARD_CLASS,       // Class guard
    WVM_GUARD_RANGE,       // Range guard
    WVM_GUARD_NOT_NIL      // Not-nil guard
} WVMGuardType;
```

### Guard Structure

```c
typedef struct WVMGuard {
    WVMGuardType type;
    WVMReg reg;
    WVMValue expectedValue;
    WVMInstr *deoptTarget;
} WVMGuard;
```

### Deoptimization

When a guard fails, execution falls back to interpreter:

```c
void wvmDeoptimize(struct WVMState *state, WVMTrace *trace, WVMGuard *guard);
```

## C API Integration

### Registering C Functions

```c
typedef WVMValue (*WVMCFunction)(struct WVMState *state, WVMValue *args, uint32_t argCount);

void wvmRegisterCFunction(struct WVMState *state, const char *name, WVMCFunction func);
```

### Calling WVM from C

```c
WVMValue wvmCallFunction(struct WVMState *state, const char *name, 
                         WVMValue *args, uint32_t argCount);
```

### Accessing Globals

```c
void wvmSetGlobal(struct WVMState *state, const char *name, WVMValue value);
WVMValue wvmGetGlobal(struct WVMState *state, const char *name);
```

## Memory Management

### Garbage Collection

WVM uses a garbage collector for automatic memory management:

```c
void wvmCollectGarbage(struct WVMState *state);
void wvmSetGCThreshold(struct WVMState *state, size_t threshold);
```

### GC Statistics

```c
typedef struct WVMGCStats {
    size_t totalAllocated;
    size_t totalFreed;
    size_t currentUsage;
    uint32_t collectionCount;
} WVMGCStats;

WVMGCStats *wvmGetGCStats(struct WVMState *state);
```

## Serialization

### Chunk Serialization

```c
int wvmSerializeChunk(WVMChunk *chunk, const char *path);
WVMChunk *wvmDeserializeChunk(const char *path);
```

### Binary Format

WVM bytecode can be saved to disk for caching:
- Magic number: `WVM\x01`
- Version: 4 bytes
- Instruction count: 4 bytes
- Instructions: variable
- Constant pool: variable
- String pool: variable

## Example: Complete WVM Usage

```c
// Create VM state
struct WVMState *state = wvmStateCreate();

// Enable JIT
wvmEnableJIT(state);
wvmSetJITThreshold(state, 100);

// Create chunk
WVMChunk *chunk = wvmChunkCreate();

// Emit bytecode: function add(a, b) { return a + b; }
// R0 = param 0, R1 = param 1, R2 = result
wvmChunkEmitOpRRR(chunk, WVM_OP_ADD, 2, 0, 1);
wvmChunkEmitOpR(chunk, WVM_OP_RETURN, 2);

// Execute
int result = wvmExecute(state, chunk);
if (result != 0) {
    fprintf(stderr, "Error: %s\n", state->errorMessage);
}

// Register C function
wvmRegisterCFunction(state, "print", myPrintFunction);

// Call from C
WVMValue args[] = { wvmNumber(42.0) };
WVMValue ret = wvmCallFunction(state, "myFunction", args, 1);

// Serialize for caching
wvmSerializeChunk(chunk, "cached.wvm");

// Clean up
wvmChunkDestroy(chunk);
wvmStateDestroy(state);
```

## Debugging

### Disassembly

```c
void wvmDisassembleChunk(FILE *out, WVMChunk *chunk);
void wvmDisassembleInstruction(FILE *out, WVMInstr instr);
```

### Tracing Execution

```c
void wvmSetTraceExecution(struct WVMState *state, int enabled);
void wvmSetTraceFile(struct WVMState *state, FILE *out);
```

### Profiling

```c
typedef struct WVMProfile {
    uint32_t *instructionCounts;
    uint64_t *instructionCycles;
    uint32_t hotLoopCount;
    uint32_t traceCount;
} WVMProfile;

WVMProfile *wvmGetProfile(struct WVMState *state);
```

## Best Practices

1. **Use JIT for Hot Code**: Enable JIT for performance-critical paths
2. **Minimize Guard Failures**: Design code to avoid frequent deoptimization
3. **Batch Operations**: Reduce VM/C boundary crossings
4. **Cache Compiled Code**: Serialize bytecode for faster startup
5. **Profile Before Optimizing**: Use profiling to identify bottlenecks
6. **Tune GC**: Adjust GC threshold based on memory usage patterns
7. **Use Guards Wisely**: Balance specialization with deoptimization cost

## Integration with WIRBLE Pipeline

WVM can be a compilation target:

```c
// Compile WIL to WVM bytecode
WVMChunk *chunk = wilCompileToWVM(wilGraph);

// Or compile MAL to WVM
WVMChunk *chunk = malCompileToWVM(malFunction);

// Execute
wvmExecute(state, chunk);
```

## Conclusion

WVM provides a complete virtual machine suitable for embedding in applications, with JIT compilation support for high performance. Its integration with WIRBLE's compilation pipeline makes it a versatile target for dynamic code generation.
