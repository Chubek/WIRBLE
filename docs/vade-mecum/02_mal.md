# Chapter 2: MAL (Machine-Adjacent Language)

## Overview

MAL (Machine-Adjacent Language) is WIRBLE's mid-level intermediate representation. It sits between the high-level WIL graph and target-specific machine code, providing a register-based, linear instruction format that is target-independent but machine-aware.

## Design Philosophy

MAL bridges the gap between abstract IR and concrete machine code:

1. **Register-Based**: Uses virtual registers instead of graph edges
2. **Linear**: Instructions organized in basic blocks
3. **Target-Independent**: Not tied to specific architectures
4. **Machine-Aware**: Includes operations that map well to real hardware
5. **Optimization-Friendly**: Suitable for both high-level and low-level optimizations

## Core Types

### MALReg
```c
typedef uint32_t MALReg;
#define MAL_INVALID_REG ((MALReg) - 1)
```
Virtual register identifier. MAL uses unlimited virtual registers; register allocation happens later.

### MALBlockId
```c
typedef uint32_t MALBlockId;
#define MAL_INVALID_BLOCK ((MALBlockId) - 1)
```
Basic block identifier.

### MALInstrId
```c
typedef uint32_t MALInstrId;
#define MAL_INVALID_INST ((MALInstrId) - 1)
```
Instruction identifier within a function.

### MALIndex
```c
typedef uint32_t MALIndex;
```
General index type for accessing collections.

## Type System

### MALType

MAL supports a rich type system:

```c
typedef enum MALType {
    MAL_TYPE_VOID = 0,
    
    // Integer types
    MAL_TYPE_I1,      // Boolean
    MAL_TYPE_I8,      // 8-bit integer
    MAL_TYPE_I16,     // 16-bit integer
    MAL_TYPE_I32,     // 32-bit integer
    MAL_TYPE_I64,     // 64-bit integer
    
    // Floating-point types
    MAL_TYPE_F32,     // 32-bit float
    MAL_TYPE_F64,     // 64-bit float
    
    // Pointer and vector types
    MAL_TYPE_PTR,     // Pointer
    MAL_TYPE_VEC,     // Vector (SIMD)
    
    MAL_TYPE__COUNT
} MALType;
```

### MALVecDesc

Vector type descriptor:

```c
typedef struct MALVecDesc {
    MALType elementType;
    uint32_t count;
} MALVecDesc;
```

## Opcode Categories

MAL provides an extensive opcode set covering all major operations:

### Data Movement

```c
MAL_OP_NOP          // No operation
MAL_OP_COPY         // Register copy
MAL_OP_LOAD         // Load from memory
MAL_OP_STORE        // Store to memory
MAL_OP_LOAD_IMM     // Load immediate value
MAL_OP_MEMCPY       // Memory copy
MAL_OP_MEMSET       // Memory set
```

### Integer Arithmetic

```c
MAL_OP_ADD          // Addition
MAL_OP_SUB          // Subtraction
MAL_OP_MUL          // Multiplication
MAL_OP_UDIV         // Unsigned division
MAL_OP_SDIV         // Signed division
MAL_OP_UREM         // Unsigned remainder
MAL_OP_SREM         // Signed remainder
MAL_OP_NEG          // Negation
MAL_OP_INC          // Increment
MAL_OP_DEC          // Decrement
```

### Bitwise Operations

```c
MAL_OP_AND          // Bitwise AND
MAL_OP_OR           // Bitwise OR
MAL_OP_XOR          // Bitwise XOR
MAL_OP_NOT          // Bitwise NOT
MAL_OP_SHL          // Shift left
MAL_OP_LSHR         // Logical shift right
MAL_OP_ASHR         // Arithmetic shift right
MAL_OP_ROTL         // Rotate left
MAL_OP_ROTR         // Rotate right
```

### Floating-Point Operations

```c
MAL_OP_FADD         // Float addition
MAL_OP_FSUB         // Float subtraction
MAL_OP_FMUL         // Float multiplication
MAL_OP_FDIV         // Float division
MAL_OP_FREM         // Float remainder
MAL_OP_FNEG         // Float negation
MAL_OP_FABS         // Float absolute value
MAL_OP_FSQRT        // Float square root
MAL_OP_FMIN         // Float minimum
MAL_OP_FMAX         // Float maximum
```

### Comparison Operations

```c
MAL_OP_CMP_EQ       // Equal
MAL_OP_CMP_NE       // Not equal
MAL_OP_CMP_SLT      // Signed less than
MAL_OP_CMP_SLE      // Signed less or equal
MAL_OP_CMP_SGT      // Signed greater than
MAL_OP_CMP_SGE      // Signed greater or equal
MAL_OP_CMP_ULT      // Unsigned less than
MAL_OP_CMP_ULE      // Unsigned less or equal
MAL_OP_CMP_UGT      // Unsigned greater than
MAL_OP_CMP_UGE      // Unsigned greater or equal
```

### Control Flow

```c
MAL_OP_JUMP         // Unconditional jump
MAL_OP_BRANCH       // Conditional branch
MAL_OP_CALL         // Function call
MAL_OP_RETURN       // Function return
MAL_OP_SWITCH       // Multi-way branch
```

### Type Conversions

```c
MAL_OP_TRUNC        // Truncate to smaller type
MAL_OP_ZEXT         // Zero extend
MAL_OP_SEXT         // Sign extend
MAL_OP_FPTRUNC      // Float truncate
MAL_OP_FPEXT        // Float extend
MAL_OP_FPTOSI       // Float to signed int
MAL_OP_FPTOUI       // Float to unsigned int
MAL_OP_SITOFP       // Signed int to float
MAL_OP_UITOFP       // Unsigned int to float
MAL_OP_BITCAST      // Bitwise reinterpretation
```

### Vector Operations

```c
MAL_OP_VEC_SPLAT    // Broadcast scalar to vector
MAL_OP_VEC_EXTRACT  // Extract element from vector
MAL_OP_VEC_INSERT   // Insert element into vector
MAL_OP_VEC_SHUFFLE  // Shuffle vector elements
```

### Atomic Operations

```c
MAL_OP_ATOMIC_LOAD      // Atomic load
MAL_OP_ATOMIC_STORE     // Atomic store
MAL_OP_ATOMIC_XCHG      // Atomic exchange
MAL_OP_ATOMIC_CAS       // Compare-and-swap
MAL_OP_ATOMIC_ADD       // Atomic add
MAL_OP_ATOMIC_SUB       // Atomic subtract
```

### Special Operations

```c
MAL_OP_PHI          // SSA phi node
MAL_OP_SELECT       // Conditional select (ternary)
MAL_OP_UNDEF        // Undefined value
```

## MAL Instructions

### Instruction Structure

Each MAL instruction consists of:
- **Opcode**: The operation to perform
- **Destination**: Output register (if any)
- **Operands**: Input registers or immediate values
- **Type**: Data type for the operation
- **Metadata**: Optional flags and annotations

### Operand Types

MAL instructions can have different operand types:
- **Register**: Virtual register reference
- **Immediate**: Compile-time constant
- **Block**: Basic block reference (for branches)
- **Function**: Function reference (for calls)

## MAL Modules and Functions

### MALModule

A MAL module contains:
- Functions
- Global variables
- Type definitions
- External declarations

### MALFunction

A MAL function contains:
- Basic blocks
- Instructions
- Virtual registers
- Parameter and return types

### MALBlock

A basic block contains:
- Sequential instructions
- Terminator instruction (branch, return, etc.)
- Predecessor and successor blocks

## Building MAL Code

### Creating a Module

```c
MALModule *module = malModuleCreate();
```

### Creating a Function

```c
MALFunction *func = malFunctionCreate(module, "myFunction", returnType);
```

### Creating Basic Blocks

```c
MALBlock *entry = malBlockCreate(func);
MALBlock *loop = malBlockCreate(func);
MALBlock *exit = malBlockCreate(func);
```

### Emitting Instructions

```c
// Allocate virtual registers
MALReg r0 = malAllocReg(func);
MALReg r1 = malAllocReg(func);
MALReg r2 = malAllocReg(func);

// Emit instructions
malEmitLoadImm(entry, r0, 42, MAL_TYPE_I32);
malEmitLoadImm(entry, r1, 10, MAL_TYPE_I32);
malEmitAdd(entry, r2, r0, r1, MAL_TYPE_I32);
malEmitReturn(entry, r2);
```

## Example: Simple Function

```c
// Create module and function
MALModule *module = malModuleCreate();
MALFunction *func = malFunctionCreate(module, "add", MAL_TYPE_I32);

// Add parameters
MALReg param0 = malAddParameter(func, MAL_TYPE_I32);
MALReg param1 = malAddParameter(func, MAL_TYPE_I32);

// Create entry block
MALBlock *entry = malBlockCreate(func);

// Allocate result register
MALReg result = malAllocReg(func);

// Emit add instruction
malEmitAdd(entry, result, param0, param1, MAL_TYPE_I32);

// Emit return
malEmitReturn(entry, result);

// Validate
if (!malValidateFunction(func)) {
    fprintf(stderr, "Invalid MAL function\n");
}

// Print for debugging
malPrintFunction(stdout, func);
```

## Control Flow Patterns

### If-Then-Else

```c
MALBlock *entry = malBlockCreate(func);
MALBlock *thenBlock = malBlockCreate(func);
MALBlock *elseBlock = malBlockCreate(func);
MALBlock *merge = malBlockCreate(func);

// Entry: test condition
MALReg cond = /* ... */;
malEmitBranch(entry, cond, thenBlock, elseBlock);

// Then block
MALReg thenValue = /* ... */;
malEmitJump(thenBlock, merge);

// Else block
MALReg elseValue = /* ... */;
malEmitJump(elseBlock, merge);

// Merge with PHI
MALReg result = malAllocReg(func);
malEmitPhi(merge, result, 
    (MALReg[]){thenValue, elseValue},
    (MALBlock*[]){thenBlock, elseBlock}, 2);
```

### Loops

```c
MALBlock *entry = malBlockCreate(func);
MALBlock *header = malBlockCreate(func);
MALBlock *body = malBlockCreate(func);
MALBlock *exit = malBlockCreate(func);

// Entry: initialize
MALReg init = /* ... */;
malEmitJump(entry, header);

// Header: PHI and condition
MALReg i = malAllocReg(func);
malEmitPhi(header, i, 
    (MALReg[]){init, /* updated i */},
    (MALBlock*[]){entry, body}, 2);

MALReg cond = /* ... */;
malEmitBranch(header, cond, body, exit);

// Body: update and loop back
MALReg updated = /* ... */;
malEmitJump(body, header);
```

## Validation

MAL code should be validated:

```c
int malValidateModule(MALModule *module);
int malValidateFunction(MALFunction *func);
```

Validation checks:
- All registers defined before use
- Type consistency
- Control flow well-formedness
- Terminator instructions at block ends
- PHI node correctness

## Printing and Debugging

```c
void malPrintModule(FILE *out, const MALModule *module);
void malPrintFunction(FILE *out, const MALFunction *func);
void malPrintBlock(FILE *out, const MALBlock *block);
void malPrintInst(FILE *out, const MALInst *inst);
```

## Optimization on MAL

MAL is suitable for various optimizations:
- Constant folding
- Dead code elimination
- Common subexpression elimination
- Strength reduction
- Loop optimizations

These can be implemented using the MRS (MAL Rewrite System) or custom passes.

## Lowering to MDS

Once MAL is optimized, it undergoes instruction selection to MDS:

```c
MDSProgram *mdsProgram = malSelectInstructions(module, machine);
```

The instruction selection process:
1. Matches MAL patterns to machine instructions
2. Performs register allocation
3. Handles calling conventions
4. Generates target-specific code

See Chapter 4 for details on MDS and instruction selection.

## Best Practices

1. **Use Virtual Registers Freely**: Register allocation happens later
2. **Keep Blocks Simple**: One terminator per block
3. **Validate Frequently**: Catch errors early
4. **Use Appropriate Types**: Match types to target capabilities
5. **Document Complex Patterns**: Comment non-obvious instruction sequences
6. **Leverage PHI Nodes**: Use for SSA form when beneficial
