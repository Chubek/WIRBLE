# Chapter 7: VXT (Vector Extensions)

## Overview

VXT defines semantic SIMD capabilities as machine metadata, then drives legality, canonicalization, and lowering decisions across WRS, MDS, and TOS.

## Position in the Pipeline

- WIL: expresses vector semantics (`add`, `mul`, `shuffle`, `select`) independent of ISA mnemonics;
- WRS: canonicalizes vector forms before instruction selection;
- MDS: maps semantic ops to target instruction descriptors;
- TOS: applies target-local peephole/cost cleanup.

VXT is the bridge between semantic vector IR and machine vector instruction inventories.

## Data Model

`include/wirble/wirble-vxt.h` defines:

- `wirble_vxt_vector_type_t`: lane count, bit-width, scalable/predicate traits;
- `wirble_vxt_instruction_semantic_t`: semantic op + element type + lane geometry;
- `wirble_vxt_extension_t`: extension capabilities (`masks`, `gather`, `scatter`, `predication`, `scalable`) and instruction inventory;
- `wirble_vxt_machine_t`: aggregate vector-extension state per machine;
- query/validation/lowering entry points.

## Machine Definition Integration

Machine manifests add `vector_extensions` with per-extension register and instruction sets.

```yaml
vector_extensions:
  - name: AVX2
    registers:
      - { name: ymm0, class: vr, bits: 256 }
    instructions:
      - { semantic: add, mnemonic: vaddps, lanes: 8, element_type: f32 }
```

Schema constraints live in `manifests/wirble-machine-definitions.schema.json`.

## Canonicalization Requirements

Apply vector canonicalization prior to target lowering:

- commutative normalization (`add a,b` -> canonical operand order);
- lane-shape normalization (consistent `<lanes x elem>` forms);
- predicate-form normalization for masked operations;
- fold target-infeasible patterns into legal decompositions.

This improves rewrite convergence and instruction-selection determinism.

## Lowering Contract

`wirble_vxt_lower_operation(...)` resolves semantic vector operations to machine-available instruction names subject to:

- lane/bit feasibility;
- extension availability;
- capability predicates (mask/gather/scatter/predication/scalable).

Failure indicates legality miss and must trigger fallback legalization, not silent scalarization.

## Cross-Chapter Dependencies

- Chapter 3 (WRS): vector canonical forms and rewrite staging;
- Chapter 4 (MDS): semantic-to-machine mapping tables;
- Chapter 5 (TOS): post-selection peephole normalization;
- Chapter 9 (Machines): declarative target vector inventories;
- Chapter 10 (Building): schema installation and target packaging.

## Failure Modes

- semantic/mnemonic conflation in IR;
- missing schema validation for `vector_extensions`;
- late legalization after register assignment;
- extension capability checks bypassed in rewrite/lowering path;
- inconsistent lane-width modeling between WRS and MDS.
