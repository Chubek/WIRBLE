# Chapter 0: Introduction to WIRBLE

## What is WIRBLE?

WIRBLE (Retargetable Compiler Infrastructure) is a comprehensive compiler infrastructure designed for building ahead-of-time (AOT) and just-in-time (JIT) compilers. It provides a complete toolchain from high-level intermediate representation down to machine code generation, with a focus on retargetability and optimization.

## Core Philosophy

WIRBLE is built around several key principles:

1. **Graph-Based IR**: Uses a Sea-of-Nodes style intermediate representation (WIL) that combines data flow and control flow in a unified graph structure
2. **Retargetability**: Machine descriptions are specified in XML, YAML, or JSON files, making it easy to target new architectures
3. **Rewrite-Based Optimization**: Employs a term rewriting system (WRS) for pattern-based transformations
4. **Layered Design**: Clear separation between high-level IR (WIL), machine-adjacent IR (MAL), and machine-specific code (MDS)
5. **JIT-First**: Designed primarily for AOT/JIT compilation scenarios with facilities like trace recording, caching, and runtime optimization

## Architecture Overview

WIRBLE consists of several interconnected subsystems:

### WIL (WIRBLE Intermediate Language)
- High-level graph-based IR
- Sea-of-Nodes representation with optional SSA
- Direct AST-to-WIL lowering without requiring CFG construction
- Pattern-based rewriting support

### MAL (Machine-Adjacent Language)
- Mid-level IR closer to machine code
- Register-based representation
- Target-independent but machine-aware
- Bridge between WIL and target-specific code

### WRS (WIRBLE Rewrite System)
- Term rewriting system for IR transformations
- Pattern matching and substitution
- Used for optimization passes on both WIL and MAL

### MDS (Machine Description System)
- Instruction selection from MAL to target instructions
- Machine description language for specifying target architectures
- Pattern-based instruction selection
- Register allocation and scheduling

### TOS (Target Optimization System)
- Target-specific optimizations
- Peephole optimization
- Trace recording and caching
- Profile-guided optimization using LMDB

### WVM (WIRBLE Virtual Machine)
- Full-featured bytecode VM
- JIT compilation support
- Runtime optimization
- Can be embedded for program extension

## Compilation Pipeline

A typical WIRBLE compilation flow:

1. **Frontend** → Parse source and build AST
2. **WIL Generation** → Lower AST to WIL graph
3. **WIL Optimization** → Apply rewrite rules via WRS
4. **MAL Lowering** → Convert WIL to MAL instructions
5. **MAL Optimization** → Apply machine-aware optimizations
6. **Instruction Selection** → Select target instructions via MDS
7. **Target Optimization** → Apply peephole optimizations via TOS
8. **Code Generation** → Emit machine code or WVM bytecode
9. **Caching** → Serialize for reuse (optional)

## Key Features

### Retargetability
WIRBLE ships with three default target specifications:
- **x86_64** (JSON format)
- **AArch64** (YAML format)
- **WebAssembly** (XML format)

New targets can be added by providing machine description files.

### Memory Management
- Arena-based allocation for fast bulk deallocation
- String interning via `wirble_strpool`
- Diagnostic system with source location tracking

### Extensibility
- WVM can be embedded in host applications
- Custom rewrite rules can be added
- Machine descriptions are data-driven

### Optimization Infrastructure
- Pattern-based rewriting at multiple IR levels
- Trace recording for hot path optimization
- Profile-guided optimization with persistent storage
- Peephole optimization database

## Use Cases

WIRBLE is designed for:

- **Language Implementers**: Building compilers for new or existing languages
- **JIT Compilers**: Runtime code generation with optimization
- **DSL Compilers**: Domain-specific language compilation
- **Program Extension**: Embedding WVM for scriptable applications
- **Research**: Experimenting with compiler optimizations and code generation

## What WIRBLE Does NOT Do

As of the current version, WIRBLE:
- Cannot write assembly files directly
- Does not link binaries (use external linker)
- Is not designed for whole-program compilation (focuses on AOT/JIT chunks)
- Does not include a frontend parser (bring your own AST)

## Getting Started

To use WIRBLE in your project:

1. Include the appropriate headers from `include/wirble/`
2. Link against the WIRBLE libraries
3. Choose your compilation path (WIL → MAL → MDS or direct to WVM)
4. Configure target machine descriptions
5. Implement your frontend to generate WIL or MAL

The following chapters cover each subsystem and their cross-layer contracts, including vector-extension semantics and machine declarative metadata.

## Documentation Structure

This vademecum is organized as follows:

- **Chapters 1-6**: Core subsystems (WIL, MAL, WRS, MDS, TOS, WVM)
- **Chapter 7**: VXT (Vector Extensions)
- **Chapter 8**: Support utilities and memory management
- **Chapter 9**: Machine specifications and retargetability
- **Chapter 10**: Building and integration
- **Chapter 11**: Advanced optimization techniques
- **Chapter 12**: Practical examples and use cases

Each chapter includes API references with correct symbol names from the codebase, usage patterns, and best practices.
