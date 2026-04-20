# WIRBLE

**WIRBLE** is a modular, rewrite‑driven compiler infrastructure designed for experimentation with intermediate representations, program rewriting systems, and virtual machine backends. The project provides a complete compilation pipeline from high‑level source languages to a portable bytecode runtime.

WIRBLE emphasizes **declarative optimization**, **IR layering**, and **composable transformations**, making it suitable for building research compilers, language prototypes, and advanced optimization pipelines.

---

# Overview

The WIRBLE toolchain is structured as a sequence of intermediate representations and transformation stages:

Source Language  
→ WIL  
→ WRS Rewrites  
→ MAL  
→ MDS  
→ TOS  
→ WVM Bytecode

Each stage isolates a specific concern of compilation such as semantic normalization, optimization, instruction selection, or bytecode generation.

The design encourages:

• modular compiler construction  
• declarative optimization via rewrite rules  
• reproducible compilation pipelines  
• separation of IR semantics from backend implementation  

---

# Key Features

• Multi‑stage compiler pipeline  
• Declarative rewrite system (WRS)  
• Standard rewrite library (`stdrewrite`)  
• Peephole optimizer for machine IR  
• Target‑agnostic operation stream  
• Portable virtual machine backend (WVM)  
• Interactive interpreter example  
• Ahead‑of‑time compiler example  

---

# Architecture

## IR Pipeline

### WIL — Wirble Intermediate Language

WIL is the high‑level internal representation used after parsing and semantic lowering. It is designed to represent structured program semantics and maintain high‑level constructs.

Responsibilities:

• represent control flow  
• represent typed expressions  
• normalize language frontend output  
• act as the input for rewrite‑based optimization  

---

### WRS — Rewrite System

WRS applies declarative pattern‑based transformations to WIL.

Rules are expressed using **S‑expressions** and stored in `.wrs` files.

Example rule:

```
(rule add-zero
  (add ?x 0)
  ?x)
```

These rules allow transformations such as:

• algebraic simplification  
• canonicalization  
• lowering preparation  

Rewrite execution typically runs until a **fixpoint** is reached.

---

### stdrewrite — Standard Rewrite Library

`stdrewrite` contains reusable rewrite rules distributed with WIRBLE.

Examples include:

• arithmetic identities  
• logical simplifications  
• canonicalization passes  

Example files:

```
src/wirble/stdrewrite/
  arithmetic.wrs
  logic.wrs
  canonical.wrs
```

These rules are automatically loaded into the rewrite engine and applied during optimization passes.

---

### MAL — Machine Abstraction Layer

MAL represents a **lower‑level instruction form** suitable for machine‑like operations while still being platform independent.

Features:

• explicit operations  
• explicit data movement  
• simplified control flow  

The MAL Rewrite System (MRS) provides peephole optimization.

Example transformations:

```
add r1, 0 → r1
mul r2, 1 → r2
```

---

### MDS — Machine Description System

MDS converts MAL instructions into target‑specific operations.

Responsibilities:

• instruction selection  
• target capability mapping  
• lowering abstract operations  

MDS isolates backend differences and enables retargetable compilation.

---

### TOS — Target Operation Stream

TOS represents the final ordered instruction stream before encoding.

Responsibilities:

• instruction ordering  
• operation scheduling  
• final stream validation  

This stage produces a sequence suitable for emission.

---

### WVM — Wirble Virtual Machine

WVM is a lightweight portable virtual machine used as the primary execution backend.

Capabilities:

• stack‑based execution  
• bytecode encoding  
• runtime execution environment  

The WVM backend allows programs compiled with WIRBLE to run on any supported host platform.

---

# Repository Structure

```
src/
  wirble/
    common/       Core utilities
    wil/          WIL IR implementation
    wrs/          Rewrite system
    mal/          Machine abstraction layer
    mds/          Machine description system
    tos/          Target operation stream
    wvm/          Virtual machine backend
    stdrewrite/   Standard rewrite rules
    support/      Parsing and helper utilities

include/
  wirble/
    wirble-wil.h
    wirble-wrs.h
    wirble-mal.h
    wirble-mds.h
    wirble-tos.h
    wirble-wvm.h

examples/
  wirblecc/
  iwirble/

third_party/
  dparser/
  lmdb/
  sfsexp/
```

---

# Example Programs

## wirblecc

`wirblecc` is a traditional ahead‑of‑time compiler demonstrating the full WIRBLE pipeline.

Pipeline:

```
Source Code
  ↓
dparser frontend
  ↓
AST → WIL
  ↓
WRS rewrites
  ↓
MAL lowering
  ↓
MDS selection
  ↓
TOS generation
  ↓
WVM bytecode
```

Purpose:

• reference compiler implementation  
• demonstration of full pipeline integration  
• testing rewrite optimization

---

## iwirble

`iwirble` is an **interactive interpreter** that compiles and executes expressions immediately.

Features:

• REPL interface  
• incremental compilation  
• direct execution on WVM  

Architecture:

```
User Input
  ↓
Parser (dparser)
  ↓
AST → WIL
  ↓
Rewrite + Lowering
  ↓
WVM bytecode
  ↓
Immediate execution
```

The REPL interface uses **microrl**.

---

# Rewrite Rules

Rewrite rules use S‑expression syntax.

Example:

```
(rule mul-one
  (mul ?x 1)
  ?x)
```

Pattern variables begin with `?`.

Rules can match:

• arithmetic expressions  
• logical operators  
• control structures  

Rewrite execution continues until no further rules match.

---

# Build Dependencies

External libraries used by WIRBLE:

• **dparser** — parsing frontend  
• **lmdb** — persistent storage / caching  
• **sfsexp** — S‑expression parsing  

These libraries are located in `third_party/`.

---

# Toolchain Configuration

The WIRBLE toolchain is defined using several manifest files:

```
SOURCE-MANIFEST.yaml
TOOLCHAIN-MANIFEST.yaml
IMPLEMENTATION-STAGES.yaml
```

These files define:

• source layout  
• compiler pipeline tools  
• staged implementation roadmap  

Agents and contributors must treat these files as **the authoritative description of the project architecture**.

---

# Development Workflow

Development follows **manifest‑driven architecture**.

Typical workflow:

1. Identify the current implementation stage.
2. Implement required subsystem features.
3. Update the manifests if architecture changes.
4. Record progress in `IMPLEMENTATION-PROGRESS.md`.

Each stage should result in a buildable system.

---

# Implementation Stages

The project is implemented in **12 stages**, beginning with foundational utilities and progressing to full compiler functionality.

Major milestones include:

1. Repository foundation
2. Runtime utilities
3. S‑expression parsing
4. WIL IR
5. WRS rewrite engine
6. Standard rewrite library
7. MAL + peephole optimization
8. Instruction selection
9. Target operation stream
10. WVM backend
11. Drivers and examples
12. Optimization and validation

---

# Design Philosophy

WIRBLE follows several core principles.

### Rewrite‑Driven Optimization

Rather than hardcoding transformations, most optimizations are expressed as rewrite rules.

Benefits:

• easier experimentation  
• better reasoning about transformations  
• simpler extensibility

---

### Layered IR Design

Each IR stage focuses on a single responsibility.

Advantages:

• easier debugging  
• modular optimization passes  
• simpler backend portability

---

### Declarative Transformations

The system favors **data‑driven transformations** over procedural logic whenever possible.

---

# Intended Use Cases

WIRBLE is designed for:

• compiler research  
• language prototyping  
• optimization experimentation  
• IR experimentation  
• teaching compiler design

---

# License

License details should be placed here.

---

# Contributing

When contributing to WIRBLE:

• follow subsystem boundaries  
• update manifest files when modifying architecture  
• add rewrite rules to `stdrewrite` when possible  
• update `IMPLEMENTATION-PROGRESS.md` when completing stages  

---

# Future Work

Potential extensions include:

• additional virtual machine backends  
• SSA‑based optimizations  
• JIT compilation  
• advanced instruction scheduling  
• profiling‑guided optimization

---

If you want, I can also write a **much stronger “research‑grade” README** (the kind LLVM/MLIR projects use) that explains the **IR semantics, rewrite theory, and pipeline invariants**. That version would be significantly more useful for AI agents and compiler developers.