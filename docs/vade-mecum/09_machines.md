# Chapter 8: Machine Specifications

## Overview

WIRBLE's retargetability is achieved through data-driven machine descriptions. This chapter covers the machine specification format, the three default targets (x86_64, AArch64, WebAssembly), and how to create custom target descriptions.

## Machine Description Format

Machine descriptions can be written in three formats:
- **JSON**: Widely supported, easy to parse
- **YAML**: Human-friendly, less verbose
- **XML**: Structured, schema-validatable

All three formats describe the same information and are functionally equivalent.

## Core Components

A complete machine description includes:

1. **Basic Information**: Name, vendor, endianness
2. **Register Model**: Register classes, aliasing, calling conventions
3. **Instruction Set**: Opcodes, operands, encoding
4. **Addressing Modes**: Memory access patterns
5. **Calling Conventions**: ABI specifications
6. **Alignment Requirements**: Data and stack alignment

## Basic Information

### Required Fields

```json
{
  "name": "x86_64",
  "vendor": "Intel",
  "endianness": "little",
  "pointerSize": 8
}
```

- **name**: Unique identifier for the target
- **vendor**: Manufacturer or organization
- **endianness**: "little" or "big"
- **pointerSize**: Size of pointers in bytes

## Register Model

### Register Classes

Registers are grouped into classes:

```json
"registerClasses": [
  {
    "name": "gpr",
    "description": "General-purpose registers",
    "bitWidth": 64
  },
  {
    "name": "vector",
    "description": "SIMD vector registers",
    "bitWidth": 128
  }
]
```

### Register Definitions

Each register specifies:

```json
{
  "id": 0,
  "name": "rax",
  "class": "gpr",
  "bitWidth": 64,
  "allocatable": true,
  "aliases": ["eax", "ax", "al", "ah"],
  "callerSaved": true,
  "calleeSaved": false,
  "isArgumentReg": false,
  "isReturnReg": true
}
```

#### Register Properties

- **id**: Unique numeric identifier
- **name**: Assembly name
- **class**: Register class membership
- **bitWidth**: Size in bits
- **allocatable**: Can be used by register allocator
- **aliases**: Sub-registers or alternate names
- **callerSaved**: Caller must save before call
- **calleeSaved**: Callee must save if used
- **isArgumentReg**: Used for passing arguments
- **isReturnReg**: Used for return values

### Register Aliasing

Example: x86_64 register aliasing

```
rax (64-bit)
 └─ eax (32-bit, bits 0-31)
     └─ ax (16-bit, bits 0-15)
         ├─ al (8-bit, bits 0-7)
         └─ ah (8-bit, bits 8-15)
```

JSON representation:

```json
{
  "id": 0,
  "name": "rax",
  "bitWidth": 64,
  "aliases": [
    {"name": "eax", "bitWidth": 32, "offset": 0},
    {"name": "ax", "bitWidth": 16, "offset": 0},
    {"name": "al", "bitWidth": 8, "offset": 0},
    {"name": "ah", "bitWidth": 8, "offset": 8}
  ]
}
```

## Instruction Set

### Instruction Definition

```json
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
      "isOutput": true,
      "bitWidth": 64
    },
    {
      "name": "src",
      "type": "reg",
      "class": "gpr",
      "isInput": true,
      "isOutput": false,
      "bitWidth": 64
    }
  ],
  "encoding": {
    "format": "rex_modrm",
    "bytes": [0x48, 0x01],
    "fields": [
      {"operand": 0, "location": "modrm_reg"},
      {"operand": 1, "location": "modrm_rm"}
    ]
  },
  "properties": {
    "mayLoad": false,
    "mayStore": false,
    "hasSideEffects": false,
    "isTerminator": false,
    "isBranch": false,
    "isCall": false,
    "isReturn": false
  },
  "scheduling": {
    "latency": 1,
    "throughput": 4
  }
}
```

### Operand Types

- **reg**: Register operand
- **imm**: Immediate value
- **mem**: Memory address
- **label**: Code label (for branches)

### Operand Constraints

```json
{
  "name": "imm8",
  "type": "imm",
  "bitWidth": 8,
  "constraints": {
    "min": -128,
    "max": 127
  }
}
```

### Encoding Formats

Common encoding formats:

- **fixed**: Fixed-length encoding
- **rex_modrm**: x86_64 REX prefix + ModR/M
- **vex**: x86 VEX prefix
- **arm_dp**: ARM data processing
- **arm_ls**: ARM load/store

## Addressing Modes

### Memory Operands

```json
{
  "name": "mem",
  "type": "mem",
  "addressingModes": [
    {
      "name": "base_disp",
      "format": "[base + disp]",
      "components": {
        "base": {"type": "reg", "class": "gpr"},
        "displacement": {"type": "imm", "bitWidth": 32}
      }
    },
    {
      "name": "base_index_scale_disp",
      "format": "[base + index*scale + disp]",
      "components": {
        "base": {"type": "reg", "class": "gpr"},
        "index": {"type": "reg", "class": "gpr"},
        "scale": {"type": "imm", "values": [1, 2, 4, 8]},
        "displacement": {"type": "imm", "bitWidth": 32}
      }
    }
  ]
}
```

## Calling Conventions

### Convention Definition

```json
{
  "name": "System V AMD64 ABI",
  "integerArgs": ["rdi", "rsi", "rdx", "rcx", "r8", "r9"],
  "floatArgs": ["xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7"],
  "integerReturn": ["rax", "rdx"],
  "floatReturn": ["xmm0", "xmm1"],
  "calleeSaved": ["rbx", "rbp", "r12", "r13", "r14", "r15"],
  "stackAlignment": 16,
  "stackGrowsDown": true,
  "returnAddressOnStack": true,
  "shadowSpace": 0
}
```

### Multiple Conventions

A target can support multiple calling conventions:

```json
"callingConventions": [
  {
    "name": "sysv_amd64",
    "description": "System V AMD64 ABI",
    ...
  },
  {
    "name": "win64",
    "description": "Windows x64 calling convention",
    ...
  }
]
```

## Alignment Requirements

```json
"alignment": {
  "stack": 16,
  "function": 16,
  "data": {
    "i8": 1,
    "i16": 2,
    "i32": 4,
    "i64": 8,
    "f32": 4,
    "f64": 8,
    "ptr": 8
  }
}
```

## x86_64 Target

### Overview

The x86_64 target (machines/x86_64.json) provides:
- 16 general-purpose registers (rax-r15)
- 16 SSE/AVX vector registers (xmm0-xmm15)
- Complex addressing modes
- Variable-length instruction encoding
- System V and Windows calling conventions

### Key Features

```json
{
  "name": "x86_64",
  "vendor": "Intel",
  "endianness": "little",
  "pointerSize": 8,
  "features": [
    "sse", "sse2", "sse3", "ssse3", "sse4.1", "sse4.2",
    "avx", "avx2", "bmi", "bmi2", "popcnt"
  ]
}
```

### Register Set

- **GPR**: rax, rbx, rcx, rdx, rsi, rdi, rbp, rsp, r8-r15
- **Vector**: xmm0-xmm15 (128-bit), ymm0-ymm15 (256-bit)
- **Special**: rip (instruction pointer), rflags

### Instruction Examples

```json
{
  "mnemonic": "mov",
  "variants": [
    {"operands": ["reg64", "reg64"], "encoding": [0x48, 0x89]},
    {"operands": ["reg64", "imm32"], "encoding": [0x48, 0xC7]},
    {"operands": ["reg64", "mem64"], "encoding": [0x48, 0x8B]},
    {"operands": ["mem64", "reg64"], "encoding": [0x48, 0x89]}
  ]
}
```

## AArch64 Target

### Overview

The AArch64 target (machines/aarch64.yaml) provides:
- 31 general-purpose registers (x0-x30)
- 32 SIMD/FP registers (v0-v31)
- Fixed-length 32-bit instruction encoding
- Load/store architecture
- AAPCS64 calling convention

### Key Features

```yaml
name: aarch64
vendor: ARM
endianness: little
pointerSize: 8
features:
  - neon
  - fp
  - crypto
  - crc
```

### Register Set

- **GPR**: x0-x30 (64-bit), w0-w30 (32-bit)
- **Special**: sp (stack pointer), xzr/wzr (zero register)
- **Vector**: v0-v31 (128-bit), with d/s/h/b views

### Instruction Format

All instructions are 32 bits:

```yaml
instructions:
  - mnemonic: add
    format: |
      31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10 09 08 07 06 05 04 03 02 01 00
      sf  0  0  0  1  0  1  1  0  0  0     Rm                0  0  0  0  0  0     Rn           Rd
    operands:
      - {name: Rd, type: reg, class: gpr}
      - {name: Rn, type: reg, class: gpr}
      - {name: Rm, type: reg, class: gpr}
```

## WebAssembly Target

### Overview

The WebAssembly target (machines/wasm.xml) provides:
- Stack-based execution model
- Unlimited virtual registers
- Structured control flow
- Linear memory model

### Key Features

```xml
<machine>
  <name>wasm</name>
  <vendor>W3C</vendor>
  <endianness>little</endianness>
  <pointerSize>4</pointerSize>
  <features>
    <feature>mvp</feature>
    <feature>simd</feature>
    <feature>threads</feature>
  </features>
</machine>
```

### Value Types

```xml
<types>
  <type name="i32" bitWidth="32"/>
  <type name="i64" bitWidth="64"/>
  <type name="f32" bitWidth="32"/>
  <type name="f64" bitWidth="64"/>
  <type name="v128" bitWidth="128"/>
</types>
```

### Instruction Examples

```xml
<instruction>
  <mnemonic>i32.add</mnemonic>
  <opcode>0x6A</opcode>
  <operands>
    <operand type="stack" valueType="i32"/>
    <operand type="stack" valueType="i32"/>
  </operands>
  <result type="i32"/>
</instruction>
```

## Creating Custom Targets

### Step 1: Define Basic Information

```json
{
  "name": "my_arch",
  "vendor": "MyCompany",
  "endianness": "little",
  "pointerSize": 4
}
```

### Step 2: Define Register Classes

```json
"registerClasses": [
  {
    "name": "gpr",
    "description": "General-purpose registers",
    "bitWidth": 32,
    "count": 16
  }
]
```

### Step 3: Define Registers

```json
"registers": [
  {"id": 0, "name": "r0", "class": "gpr", "allocatable": true},
  {"id": 1, "name": "r1", "class": "gpr", "allocatable": true},
  ...
  {"id": 15, "name": "sp", "class": "gpr", "allocatable": false}
]
```

### Step 4: Define Instructions

```json
"instructions": [
  {
    "id": 0,
    "mnemonic": "add",
    "operands": [
      {"name": "rd", "type": "reg", "class": "gpr", "isOutput": true},
      {"name": "rs1", "type": "reg", "class": "gpr", "isInput": true},
      {"name": "rs2", "type": "reg", "class": "gpr", "isInput": true}
    ],
    "encoding": {
      "format": "r_type",
      "opcode": 0x33,
      "funct3": 0x0,
      "funct7": 0x00
    }
  }
]
```

### Step 5: Define Calling Convention

```json
"callingConventions": [
  {
    "name": "default",
    "integerArgs": ["r0", "r1", "r2", "r3"],
    "integerReturn": ["r0"],
    "calleeSaved": ["r4", "r5", "r6", "r7"],
    "stackAlignment": 8
  }
]
```

### Step 6: Validate

```c
MDSMachine *machine = mdsLoadMachineJSON("my_arch.json");
if (!machine) {
    fprintf(stderr, "Failed to load machine description\n");
    return -1;
}

if (!mdsValidateMachine(machine)) {
    fprintf(stderr, "Invalid machine description\n");
    return -1;
}
```

## Validation Rules

A valid machine description must:

1. **Unique IDs**: All register and instruction IDs must be unique
2. **Valid References**: Register classes must exist before use
3. **Consistent Encoding**: Encoding formats must match operand types
4. **Complete Calling Convention**: All required registers must be specified
5. **Proper Aliasing**: Register aliases must be consistent
6. **Valid Constraints**: Immediate ranges must be valid

## Best Practices

1. **Start Simple**: Begin with minimal instruction set
2. **Test Incrementally**: Validate after each addition
3. **Document Thoroughly**: Include descriptions for all components
4. **Follow Conventions**: Use standard naming (e.g., "gpr", "fpr")
5. **Specify Encoding Precisely**: Match ISA manual exactly
6. **Include All Variants**: Cover all instruction forms
7. **Test with Real Code**: Generate code and verify correctness

## Tools and Utilities

### Validation

```c
int mdsValidateMachine(MDSMachine *machine);
void mdsPrintValidationErrors(FILE *out, MDSMachine *machine);
```

### Conversion

```c
MDSMachine *mdsConvertJSONToYAML(const char *jsonPath, const char *yamlPath);
MDSMachine *mdsConvertYAMLToXML(const char *yamlPath, const char *xmlPath);
```

### Querying

```c
MDSRegister *mdsFindRegisterByName(MDSMachine *machine, const char *name);
MDSInstruction *mdsFindInstructionByMnemonic(MDSMachine *machine, const char *mnemonic);
MDSCallingConv *mdsFindCallingConv(MDSMachine *machine, const char *name);
```

## Conclusion

WIRBLE's machine description system provides a flexible, data-driven approach to retargetability. By separating target-specific information from the compiler infrastructure, new architectures can be supported by simply providing a machine description file, without modifying the compiler itself.


## Vector Extensions

Machine descriptions include `vector_extensions` for SIMD capability declaration, register classes, and semantic instruction inventories.

- keep semantic op names target-agnostic;
- encode capability bits (`masks`, `gather`, `scatter`, `predication`, `scalable`);
- include lane geometry and element type metadata for legalization/lowering.

The VXT contract is consumed by WRS canonicalization and MDS selection; do not encode backend policy directly into IR rules.
