# WIRBLE Documentation

## Welcome

Welcome to the WIRBLE (Retargetable Compiler Infrastructure) documentation. WIRBLE is a comprehensive compiler infrastructure designed for building ahead-of-time (AOT) and just-in-time (JIT) compilers with a focus on retargetability and optimization.

## Quick Links

- [GitHub Repository](https://github.com/yourorg/wirble)
- [API Reference](annotated.html)
- [Vademecum (User Guide)](#vademecum)
- [VXT Chapter](vade-mecum/07_vxt.html)
- [Machine Definitions + VXT Schema](vade-mecum/09_machines.html)

## What is WIRBLE?

WIRBLE provides a complete toolchain from high-level intermediate representation down to machine code generation. It features:

- **Graph-Based IR (WIL)**: Sea-of-Nodes style intermediate representation
- **Machine-Adjacent IR (MAL)**: Register-based mid-level representation
- **Rewrite System (WRS)**: Pattern-based optimization framework
- **Vector Extensions (VXT)**: Semantic SIMD capability and lowering metadata
- **Machine Descriptions (MDS)**: Data-driven target specifications
- **Target Optimization (TOS)**: Peephole optimization and trace caching
- **Virtual Machine (WVM)**: Embeddable bytecode VM with JIT support

## Architecture Overview

```
Source Code
    ↓
  [WIL] ← High-level graph-based IR
    ↓
  [WRS] ← Pattern-based rewriting + canonicalization
    ↓
  [MAL] ← Machine-adjacent IR
    ↓
  [MDS] ← Instruction selection
    ↑
  [VXT] ← Vector semantics, legality, target capability metadata
    ↓
  [TOS] ← Target optimization
    ↓
Machine Code / WVM Bytecode
```

## VXT Guide Map

- **Semantics and contracts**: [Chapter 7: VXT (Vector Extensions)](vade-mecum/07_vxt.html)
- **Machine schema and `vector_extensions`**: [Chapter 9: Machine Specifications](vade-mecum/09_machines.html)
- **Rewrite staging and canonical forms**: [Chapter 3: WRS](vade-mecum/03_wrs.html)
- **Selection boundary and target mapping**: [Chapter 4: MDS](vade-mecum/04_mds.html)
- **Build/install of docs and manifests**: [Chapter 10: Building and Using WIRBLE](vade-mecum/10_building.html)

## Vademecum

The WIRBLE Vademecum is a comprehensive guide covering all aspects of using WIRBLE:

### Core Concepts

- **[Chapter 0: Introduction to WIRBLE](vade-mecum/00_intro.html)**
  - Overview of WIRBLE's architecture and philosophy
  - Key features and use cases
  - What WIRBLE does and doesn't do

- **[Chapter 1: WIL (WIRBLE Intermediate Language)](vade-mecum/01_wil.html)**
  - Graph-based IR design
  - Node types and categories
  - Building and manipulating WIL graphs
  - Control flow and data flow patterns

- **[Chapter 2: MAL (Machine-Adjacent Language)](vade-mecum/02_mal.html)**
  - Register-based IR
  - Type system and opcodes
  - Building MAL programs
  - Control flow patterns

- **[Chapter 3: WRS (WIRBLE Rewrite System)](vade-mecum/03_wrs.html)**
  - Term rewriting theory
  - Pattern construction and matching
  - Rewrite rules and strategies
  - Optimization patterns

### Target Code Generation

- **[Chapter 4: MDS (Machine Description System)](vade-mecum/04_mds.html)**
  - Register models and instruction sets
  - Instruction selection
  - Register allocation
  - Calling conventions

- **[Chapter 5: TOS (Target Optimization System)](vade-mecum/05_tos.html)**
  - Peephole optimization
  - Trace recording and caching
  - Profile-guided optimization
  - LMDB integration

- **[Chapter 6: WVM (WIRBLE Virtual Machine)](vade-mecum/06_wvm.html)**
  - Bytecode format and execution
  - JIT compilation
  - Guards and deoptimization
  - C API integration

### Infrastructure

- **[Chapter 7: VXT (Vector Extensions)](vade-mecum/07_vxt.html)**
  - semantic SIMD model and capabilities
  - legality and canonicalization boundaries
  - semantic-to-target vector lowering contracts

- **[Chapter 8: Memory Management and Support Utilities](vade-mecum/08_support.html)**

- **[Chapter 9: Machine Specifications](vade-mecum/09_machines.html)**
  - Machine description format
  - x86_64, AArch64, and WebAssembly targets
  - Creating custom targets
  - Validation and best practices

### Practical Usage

- **[Chapter 10: Building and Using WIRBLE](vade-mecum/10_building.html)**
  - Build systems (CMake, Meson, Autotools)
  - Integration into projects
  - Cross-compilation
  - Troubleshooting

- **[Chapter 11: Advanced Topics and Optimization](vade-mecum/11_advanced.html)**
  - Multi-level optimization strategies
  - Register allocation algorithms
  - Instruction scheduling
  - JIT compilation techniques
  - Debugging and profiling

- **[Chapter 12: Examples and Use Cases](vade-mecum/12_examples.html)**
  - Simple expression compiler
  - Stack-based VM
  - JIT compiler for loops
  - DSL compiler
  - Embedding WVM
  - Custom targets
  - Optimization passes
