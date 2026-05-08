# Chapter 4: MDS (Machine Description System)

## Overview

MDS (Machine Description System) is WIRBLE's instruction selection and target code generation subsystem. It bridges the gap between target-independent MAL code and concrete machine instructions through pattern-based instruction selection.

## Design Philosophy

MDS enables retargetability through:

1. **Data-Driven**: Machine descriptions in XML, YAML, or JSON
2. **Pattern-Based Selection**: Match MAL patterns to target instructions
3. **Register Model**: Detailed register class and aliasing information
4. **Encoding Specification**: Instruction encoding for code generation
5. **Calling Conventions**: ABI and calling convention support

## Core Types

### MDSRegId
```c
typedef uint32_t MDSRegId;
```
Physical register identifier.

### MDSInstrId
```c
typedef uint32_t MDSInstrId;
```
Instruction identifier in the target ISA.

### MDSIndex
```c
typedef uint32_t MDSIndex;
```
General index type.

### MDSPatternId
```c
typedef uint32_t MDSPatternId;
```
Instruction selection pattern identifier.

### MDSTargetKind

Predefined target architectures:

```c
typedef enum MDSTargetKind {
    MDS_TARGET_GENERIC = 0,
    MDS_TARGET_X86_64,
    MDS_TARGET_AARCH64,
    MDS_TARGET_WASM
} MDSTargetKind;
```

## Register Model

### MDSRegisterClass

Register classes group registers with similar properties:

```c
typedef enum MDSRegisterClass {
    MDS_REGCLASS_GPR = 0,      // General-purpose registers
    MDS_REGCLASS_FP,           // Floating-point registers
    MDS_REGCLASS_VECTOR,       // Vector/SIMD registers
    MDS_REGCLASS_SPECIAL,      // Special-purpose registers
    MDS_REGCLASS__COUNT
} MDSRegisterClass;
```

### MDSRegister

Complete register description:

```c
typedef struct MDSRegister {
    MDSRegId id;
    const char *name;              // e.g., "rax", "xmm0", "v0"
    MDSRegisterClass regClass;
    uint32_t bitWidth;
    int allocatable;               // Can be used by RA
    
    // Register aliasing (e.g., al/ax/eax/rax on x86)
    MDSRegId *aliases;
    uint32_t aliasCount;
    
    // Sub-register relationships
    MDSRegId parentReg;            // 0 if none
    uint32_t subRegOffset;         // Bit offset within parent
    
    // Calling convention info
    int isCallerSaved;
    int isCalleeSaved;
    int isArgumentReg;
    int isReturnReg;
    uint32_t argumentIndex;        // If isArgumentReg
} MDSRegister;
```

### Register Aliasing

Example: x86_64 register aliasing
- `rax` (64-bit) contains `eax` (32-bit)
- `eax` contains `ax` (16-bit)
- `ax` contains `al` (8-bit low) and `ah` (8-bit high)

## Operand Model

### MDSOperandType

```c
typedef enum MDSOperandType {
    MDS_OPERAND_REG,      // Register
    MDS_OPERAND_IMM,      // Immediate value
    MDS_OPERAND_MEM,      // Memory address
    MDS_OPERAND_LABEL     // Code label
} MDSOperandType;
```

### MDSOperandDesc

Operand descriptor for instruction templates:

```c
typedef struct MDSOperandDesc {
    const char *name;              // e.g., "$dst", "$src1"
    MDSOperandType type;
    MDSRegisterClass regClass;     // If type == REG
    uint32_t bitWidth;
    int isInput;
    int isOutput;
    int isEarlyClobber;            // Output written before inputs read
    int isTiedTo;                  // Index of tied operand, -1 if none
    
    // Immediate constraints
    int64_t immMin;
    int64_t immMax;
    
    // Memory addressing mode constraints
    int allowsBaseReg;
    int allowsIndexReg;
    int allowsDisplacement;
    int allowsScale;
} MDSOperandDesc;
```

## Instruction Encoding

### Encoding Format

Instructions are encoded as byte sequences:

```c
typedef struct MDSEncoding {
    uint8_t *bytes;                // Encoding template
    uint32_t length;               // Length in bytes
    
    // Bit fields for operands
    struct {
        uint32_t byteOffset;       // Byte offset in encoding
        uint32_t bitOffset;        // Bit offset within byte
        uint32_t bitWidth;         // Width in bits
        int operandIndex;          // Which operand
    } *fields;
    uint32_t fieldCount;
} MDSEncoding;
```

### Encoding Examples

x86_64 ADD instruction:
```
ADD r64, r64  →  48 01 /r
  - 48: REX.W prefix
  - 01: ADD opcode
  - /r: ModR/M byte (register-register)
```

AArch64 ADD instruction:
```
ADD Xd, Xn, Xm  →  1001 1011 000 Xm 000000 Xn Xd
  - Fixed bits: 10011011000
  - Xm: source register 2 (bits 16-20)
  - Xn: source register 1 (bits 5-9)
  - Xd: destination register (bits 0-4)
```

## Instruction Description

### MDSInstruction

Complete instruction specification:

```c
typedef struct MDSInstruction {
    MDSInstrId id;
    const char *mnemonic;          // e.g., "add", "mov"
    const char *asmTemplate;       // e.g., "add %0, %1"
    
    MDSOperandDesc *operands;
    uint32_t operandCount;
    
    MDSEncoding encoding;
    
    // Instruction properties
    int isTerminator;              // Ends basic block
    int isBranch;
    int isCall;
    int isReturn;
    int hasDelaySlot;
    int mayLoad;
    int mayStore;
    int hasSideEffects;
    
    // Scheduling info
    uint32_t latency;
    uint32_t throughput;
    
    // Pattern for instruction selection
    struct MALPattern *pattern;
} MDSInstruction;
```

## Machine Description

### MDSMachine

Top-level machine description:

```c
typedef struct MDSMachine {
    const char *name;              // e.g., "x86_64", "aarch64"
    const char *vendor;            // e.g., "Intel", "ARM"
    MDSTargetKind kind;
    
    // Endianness
    int isLittleEndian;
    
    // Pointer size
    uint32_t pointerSize;          // In bytes
    
    // Registers
    MDSRegister *registers;
    uint32_t registerCount;
    
    // Instructions
    MDSInstruction *instructions;
    uint32_t instructionCount;
    
    // Calling conventions
    struct MDSCallingConv *callingConvs;
    uint32_t callingConvCount;
    
    // Alignment requirements
    uint32_t stackAlignment;
    uint32_t maxAlignment;
} MDSMachine;
```

## Loading Machine Descriptions

### From Files

```c
MDSMachine *mdsLoadMachine(const char *path);
MDSMachine *mdsLoadMachineJSON(const char *path);
MDSMachine *mdsLoadMachineYAML(const char *path);
MDSMachine *mdsLoadMachineXML(const char *path);
```

### Predefined Machines

```c
MDSMachine *mdsGetX86_64Machine(void);
MDSMachine *mdsGetAArch64Machine(void);
MDSMachine *mdsGetWasmMachine(void);
```

## Instruction Selection

### Pattern Matching

Instruction selection uses pattern matching to map MAL to target instructions:

```c
typedef struct MALPattern {
    MALOpcode opcode;
    MDSOperandType *operandTypes;
    uint32_t operandCount;
    
    // Constraints
    int (*constraint)(MALInst *inst, void *userData);
    void *userData;
} MALPattern;
```

### Selection Process

```c
MDSProgram *mdsSelectInstructions(MALModule *module, MDSMachine *machine);
```

The selection process:
1. **Pattern Matching**: Find matching instruction patterns for each MAL instruction
2. **Cost Calculation**: Compute cost for each match
3. **Selection**: Choose lowest-cost instruction
4. **Operand Mapping**: Map MAL registers to machine registers
5. **Legalization**: Handle operations not directly supported

### Selection Strategies

- **Greedy**: Select best instruction for each MAL instruction independently
- **Tree Covering**: Use dynamic programming for optimal instruction sequences
- **Graph Covering**: Handle DAG patterns

## Instruction Selector

### MDSInstrSelector

```c
typedef struct MDSInstrSelector {
    MDSMachine *machine;
    
    // Pattern database
    struct MDSPatternTable *patterns;
    
    // Selection strategy
    int strategy;
    
    // Cost model
    int (*costFunc)(MDSInstruction *inst, MALInst *malInst);
} MDSInstrSelector;
```

### Creating a Selector

```c
MDSInstrSelector *mdsSelectorCreate(MDSMachine *machine);
void mdsSelectorDestroy(MDSInstrSelector *selector);
```

### Adding Patterns

```c
void mdsSelectorAddPattern(MDSInstrSelector *selector, 
                          MALPattern *pattern,
                          MDSInstruction *instruction);
```

### Running Selection

```c
MDSProgram *mdsSelectorSelect(MDSInstrSelector *selector, MALFunction *func);
```

## Register Allocation

After instruction selection, virtual registers must be mapped to physical registers:

```c
int mdsAllocateRegisters(MDSProgram *program, MDSMachine *machine);
```

### Allocation Strategies

- **Linear Scan**: Fast, simple allocation
- **Graph Coloring**: Higher quality, slower
- **PBQP**: Optimal for complex constraints

### Spilling

When registers run out, values are spilled to stack:

```c
typedef struct MDSSpillInfo {
    MALReg virtualReg;
    int32_t stackOffset;
    uint32_t spillCount;
} MDSSpillInfo;
```

## Calling Conventions

### MDSCallingConv

```c
typedef struct MDSCallingConv {
    const char *name;              // e.g., "System V AMD64 ABI"
    
    // Argument passing
    MDSRegId *intArgRegs;
    uint32_t intArgRegCount;
    MDSRegId *fpArgRegs;
    uint32_t fpArgRegCount;
    
    // Return values
    MDSRegId *intRetRegs;
    uint32_t intRetRegCount;
    MDSRegId *fpRetRegs;
    uint32_t fpRetRegCount;
    
    // Callee-saved registers
    MDSRegId *calleeSavedRegs;
    uint32_t calleeSavedRegCount;
    
    // Stack layout
    int stackGrowsDown;
    uint32_t stackAlignment;
    int32_t returnAddressOffset;
} MDSCallingConv;
```

### Applying Calling Convention

```c
void mdsApplyCallingConv(MDSProgram *program, MDSCallingConv *conv);
```

This generates:
- Prologue: Save callee-saved registers, allocate stack frame
- Epilogue: Restore registers, deallocate frame, return
- Argument marshalling: Move arguments to correct locations
- Return value handling: Place return values in correct registers

## Code Generation

### MDSProgram

Selected and allocated program:

```c
typedef struct MDSProgram {
    MDSMachine *machine;
    
    struct MDSFunction *functions;
    uint32_t functionCount;
    
    struct MDSGlobal *globals;
    uint32_t globalCount;
} MDSProgram;
```

### MDSFunction

```c
typedef struct MDSFunction {
    const char *name;
    
    struct MDSBlock *blocks;
    uint32_t blockCount;
    
    MDSInstruction **instructions;
    uint32_t instructionCount;
    
    // Register allocation info
    uint32_t frameSize;
    MDSSpillInfo *spills;
    uint32_t spillCount;
} MDSFunction;
```

### Emitting Machine Code

```c
typedef struct MDSCodeBuffer {
    uint8_t *data;
    uint32_t size;
    uint32_t capacity;
} MDSCodeBuffer;

MDSCodeBuffer *mdsEmitCode(MDSProgram *program);
```

### Relocation

```c
typedef struct MDSRelocation {
    uint32_t offset;               // Offset in code buffer
    uint32_t type;                 // Relocation type
    const char *symbol;            // Symbol name
    int32_t addend;                // Addend
} MDSRelocation;
```

## Assembly Output

Generate human-readable assembly:

```c
void mdsPrintAssembly(FILE *out, MDSProgram *program);
void mdsPrintFunction(FILE *out, MDSFunction *func);
```

Example output:
```asm
myFunction:
    push    rbp
    mov     rbp, rsp
    sub     rsp, 16
    mov     eax, edi
    add     eax, esi
    leave
    ret
```

## Example: Complete Pipeline

```c
// Load machine description
MDSMachine *machine = mdsLoadMachineJSON("machines/x86_64.json");

// Create instruction selector
MDSInstrSelector *selector = mdsSelectorCreate(machine);

// Select instructions from MAL
MDSProgram *program = mdsSelectorSelect(selector, malFunction);

// Allocate registers
mdsAllocateRegisters(program, machine);

// Apply calling convention
MDSCallingConv *conv = mdsGetCallingConv(machine, "sysv_amd64");
mdsApplyCallingConv(program, conv);

// Emit machine code
MDSCodeBuffer *code = mdsEmitCode(program);

// Or print assembly
mdsPrintAssembly(stdout, program);

// Clean up
mdsCodeBufferDestroy(code);
mdsProgramDestroy(program);
mdsSelectorDestroy(selector);
mdsMachineDestroy(machine);
```

## Machine Description Format

### JSON Example

```json
{
  "name": "x86_64",
  "vendor": "Intel",
  "endianness": "little",
  "pointerSize": 8,
  "registers": [
    {
      "id": 0,
      "name": "rax",
      "class": "gpr",
      "bitWidth": 64,
      "allocatable": true,
      "aliases": ["eax", "ax", "al"]
    }
  ],
  "instructions": [
    {
      "id": 0,
      "mnemonic": "add",
      "asmTemplate": "add %0, %1",
      "operands": [
        {
          "name": "dst",
          "type": "reg",
          "class": "gpr",
          "isInput": true,
          "isOutput": true
        }
      ]
    }
  ]
}
```

## Best Practices

1. **Validate Machine Descriptions**: Check for consistency and completeness
2. **Test Instruction Selection**: Verify correct instruction choices
3. **Profile Register Allocation**: Identify spilling hotspots
4. **Document Calling Conventions**: Clearly specify ABI requirements
5. **Handle Edge Cases**: Test with unusual operand combinations
6. **Optimize Common Patterns**: Provide efficient instruction sequences
7. **Maintain Encoding Accuracy**: Verify against ISA manuals

## Integration with TOS

After MDS generates target code, TOS can apply target-specific optimizations:

```c
TOSProgram *tosProgram = tosBuildFromMDS(mdsProgram);
tosApplyPeepholeOpts(tosProgram);
```

See Chapter 5 for details on TOS optimizations.
