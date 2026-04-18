

Let me pull the relevant details from the attached files to ground this guide properly.
# WIRBLE-WRS: Rewrite System Language Guide

**WIRBLE-WRS** is a declarative term rewriting system (TRS) for specifying optimizations and transformations on WIRBLE Intermediate Language (WIL) graphs. It provides a high-level S-expression-based DSL for defining pattern-based rewrite rules, guards, and application strategies.

---

## Overview

WIRBLE-WRS sits between the WIL IR and optimization passes in the WIRBLE compiler pipeline:

Source → Parser → WIL IR → WRS Optimizer → Scheduled WIL → Backend


Instead of writing imperative C code for each optimization, you write declarative `.wrs` files that specify:
- **Patterns** to match against WIL nodes
- **Replacement** templates to construct new nodes
- **Guards** to conditionally enable rules
- **Strategies** to control rule application order

The WRS engine applies these rules to the Sea-of-Nodes graph, leveraging the explicit data-flow structure for efficient pattern matching and rewriting.

---

## Theoretical Foundation

WIRBLE-WRS is grounded in formal Term Rewriting System theory (from `algo-term-rewrite-system-trs.pdf`, page 1):

- **Rewrite rule**: A pair **(l, r)** where:
  - `l` (LHS) is not a variable
  - Variables in `r` (RHS) must occur in `l`
  - Sorts match: `srt(l) = srt(r)`

- **Single-step reduction**: If a subterm matches `l` under substitution `σ`, the redex is replaced by `rσ`

- **Normal form**: A term is in normal form if no rule can be applied

- **Confluence (CR)**: If `t →→→ s` and `t →→→ s'`, then there exists `u` with `s →→→ u` and `s' →→→ u`

WRS operates on **unscheduled** Sea-of-Nodes graphs where control-flow is encoded via region nodes (CFG extraction per `semantics-of-sea-of-nodes-ir-il.pdf`, page 8, Definition 3.3) and data dependencies are explicit edges.

---

## File Format

`.wrs` files use S-expressions with three top-level forms:

### 1. `defrule` — Define a rewrite rule

```scheme
(defrule rule-name
  (pattern <pattern-expr>)
  (replacement <pattern-expr>)
  [(guard <guard-expr>)]
  [(priority <int>)])
```

**Example: Constant folding for addition**
```scheme
(defrule fold-add-const
  (pattern (Add (Const ?x) (Const ?y)))
  (replacement (Const (+ ?x ?y))))
```

**Example: Algebraic identity with guard**
```scheme
(defrule mul-by-zero
  (pattern (Mul ?x (Const ?c)))
  (replacement (Const 0))
  (guard (eq ?c 0)))
```

### 2. `defstrategy` — Define rule application strategy

```scheme
(defstrategy strategy-name <kind> <rule-name>...
  [(max-iterations <int>)])
```

**Kinds:**
- `innermost` — Bottom-up: rewrite children first, then parent
- `outermost` — Top-down: rewrite parent first, then children
- `leftmost` — Left-to-right: stop at first match
- `parallel` — Apply all rules simultaneously (experimental)
- `fixpoint` — Repeat until no rule applies (with iteration limit)

**Example: Peephole optimization strategy**
```scheme
(defstrategy peephole innermost
  fold-add-const
  fold-sub-const
  mul-by-zero
  mul-by-one
  (max-iterations 100))
```

### 3. `defpredicate` — Define custom guard predicates

```scheme
(defpredicate predicate-name
  (params <var>...)
  (body <pattern-expr>))
```

**Example: Check if value is power of two**
```scheme
(defpredicate is-power-of-two
  (params ?n)
  (body (and (gt ?n 0)
             (eq (and ?n (sub ?n 1)) 0))))
```

---

## Pattern Language

Patterns match WIL nodes and bind variables for use in replacements and guards.

### Pattern Forms

| Form | Meaning | Example |
|------|---------|---------|
| `?var` | Variable (binds to any node) | `?x` |
| `_` | Wildcard (matches but doesn't bind) | `(Add _ _)` |
| `(NodeKind ...)` | Node with specific kind and children | `(Add ?x ?y)` |
| `(Const value)` | Constant literal | `(Const 42)` |

### WIL Node Kinds

Common node kinds you can match:

**Arithmetic:**
- `Add`, `Sub`, `Mul`, `Div`, `Mod`
- `Neg`, `Abs`

**Comparison:**
- `Eq`, `Ne`, `Lt`, `Le`, `Gt`, `Ge`

**Logical:**
- `And`, `Or`, `Xor`, `Not`

**Memory:**
- `Load`, `Store`, `Alloca`

**Control:**
- `Region`, `If`, `Loop`, `Phi`, `Return`

**Constants:**
- `Const`, `Undef`

---

## Guard Language

Guards are boolean expressions that conditionally enable rules.

### Guard Forms

| Form | Meaning |
|------|---------|
| `(and <g1> <g2>)` | Logical AND |
| `(or <g1> <g2>)` | Logical OR |
| `(not <g>)` | Logical NOT |
| `(eq <a> <b>)` | Equality |
| `(ne <a> <b>)` | Inequality |
| `(lt <a> <b>)` | Less than |
| `(le <a> <b>)` | Less than or equal |
| `(gt <a> <b>)` | Greater than |
| `(ge <a> <b>)` | Greater than or equal |
| `(typeof <var> <type>)` | Type check |
| `(predicate <name> <args>...)` | Custom predicate |

**Example: Strength reduction for multiplication**
```scheme
(defrule mul-to-shift
  (pattern (Mul ?x (Const ?c)))
  (replacement (Shl ?x (Const (log2 ?c))))
  (guard (predicate is-power-of-two ?c)))
```

---

## Substitution and Variable Binding

When a pattern matches, variables are bound to WIL nodes. The substitution `σ` maps pattern variables to concrete nodes:

Pattern:      (Add ?x (Const ?c))
Matches:      (Add n42 (Const 10))
Substitution: σ = {?x ↦ n42, ?c ↦ 10}
Replacement:  (Const (+ ?x ?c)) → (Const (+ n42 10)) → (Const n52)


Variables in the replacement are substituted with their bound values. This follows the formal TRS definition where a redex at position `p` is replaced by `rσ`.

---

## Strategies and Traversal Order

Strategies control how rules are applied to the Sea-of-Nodes graph. Since WIL is an unscheduled graph (per `lazy-sparse-prop-in-sea-of-nodes-ir-il.pdf`), traversal order matters for efficiency and termination.

### Innermost (Bottom-Up)

Rewrites children before parents. Guarantees that when a rule fires, its operands are already in normal form.

**Use case:** Constant folding, algebraic simplification

```scheme
(defstrategy algebraic-opt innermost
  fold-add-const
  fold-mul-const
  add-zero
  mul-one)
```

### Outermost (Top-Down)

Rewrites parents before children. Useful when parent rewrites expose new opportunities in children.

**Use case:** Inlining, loop unrolling

```scheme
(defstrategy inline-expand outermost
  inline-small-function
  unroll-small-loop)
```

### Fixpoint

Repeatedly applies rules until no rule can fire (normal form reached) or iteration limit hit.

**Use case:** Iterative optimization passes

```scheme
(defstrategy optimize-fixpoint fixpoint
  fold-add-const
  fold-mul-const
  strength-reduce
  (max-iterations 1000))
```

### Leftmost

Applies rules left-to-right in the graph, stopping at the first match. Useful for deterministic single-pass optimizations.

---

## Integration with WIRBLE Pipeline

### 1. Load WRS File

```c
WRSFile *wrs = wrs_file_load("optimizations.wrs");
if (!wrs) {
    fprintf(stderr, "Failed to load WRS file\n");
    return -1;
}

if (!wrs_file_validate(wrs)) {
    WRSDiagContext *diag = wrs_file_diag(wrs);
    wrs_diag_print_all(diag, stderr);
    return -1;
}
```

### 2. Apply Strategy to WIL Graph

```c
WILGraph *graph = /* ... your WIL graph ... */;

const WRSStrategy *strat = wrs_file_find_strategy(wrs, "peephole");
if (!strat) {
    fprintf(stderr, "Strategy 'peephole' not found\n");
    return -1;
}

/* Apply to all nodes in graph */
for (int i = 0; i < wil_graph_num_nodes(graph); i++) {
    WILNode *node = wil_graph_get_node(graph, i);
    WILNode *rewritten = wrs_strategy_apply(strat, node, graph, wrs);
    if (rewritten && rewritten != node) {
        wil_node_replace_all_uses(node, rewritten);
    }
}
```

### 3. Check Normal Form

```c
int all_normal = 1;
for (int i = 0; i < wil_graph_num_nodes(graph); i++) {
    WILNode *node = wil_graph_get_node(graph, i);
    if (!wrs_is_normal_form(node, wrs)) {
        all_normal = 0;
        break;
    }
}

if (all_normal) {
    printf("Graph is in normal form (fully optimized)\n");
}
```

### 4. Cleanup

```c
wrs_file_destroy(wrs);
```

---

## Example: Complete Optimization Pass

**File: `peephole.wrs`**
```scheme
;; Constant folding
(defrule fold-add
  (pattern (Add (Const ?x) (Const ?y)))
  (replacement (Const (+ ?x ?y))))

(defrule fold-mul
  (pattern (Mul (Const ?x) (Const ?y)))
  (replacement (Const (* ?x ?y))))

;; Algebraic identities
(defrule add-zero
  (pattern (Add ?x (Const 0)))
  (replacement ?x))

(defrule mul-one
  (pattern (Mul ?x (Const 1)))
  (replacement ?x))

(defrule mul-zero
  (pattern (Mul ?x (Const 0)))
  (replacement (Const 0)))

;; Strength reduction
(defpredicate is-power-of-two
  (params ?n)
  (body (and (gt ?n 0)
             (eq (and ?n (sub ?n 1)) 0))))

(defrule mul-to-shift
  (pattern (Mul ?x (Const ?c)))
  (replacement (Shl ?x (Const (log2 ?c))))
  (guard (predicate is-power-of-two ?c)))

;; Strategy: apply all rules bottom-up until fixpoint
(defstrategy peephole fixpoint
  fold-add
  fold-mul
  add-zero
  mul-one
  mul-zero
  mul-to-shift
  (max-iterations 100))
```

**Usage in compiler:**
```c
WRSFile *wrs = wrs_file_load("peephole.wrs");
const WRSStrategy *strat = wrs_file_find_strategy(wrs, "peephole");

/* Apply to entire graph */
for (int i = 0; i < wil_graph_num_nodes(graph); i++) {
    WILNode *node = wil_graph_get_node(graph, i);
    wrs_strategy_apply(strat, node, graph, wrs);
}

wrs_file_destroy(wrs);
```

---

## Best Practices

### 1. Rule Ordering
Place more specific rules before general ones in strategies. The first matching rule fires.

### 2. Termination
Always use `max-iterations` with `fixpoint` strategies to prevent infinite loops.

### 3. Guards for Safety
Use guards to prevent invalid rewrites:
```scheme
(defrule div-to-shift
  (pattern (Div ?x (Const ?c)))
  (replacement (Shr ?x (Const (log2 ?c))))
  (guard (and (predicate is-power-of-two ?c)
              (typeof ?x unsigned))))
```

### 4. Priority for Conflicts
When multiple rules match, higher priority wins:
```scheme
(defrule specific-case
  (pattern (Add (Mul ?x (Const 2)) ?x))
  (replacement (Mul ?x (Const 3)))
  (priority 10))

(defrule general-case
  (pattern (Add ?x ?x))
  (replacement (Mul ?x (Const 2)))
  (priority 5))
```

### 5. Validation
Always validate WRS files before use:
```c
if (!wrs_file_validate(wrs)) {
    wrs_diag_print_all(wrs_file_diag(wrs), stderr);
    exit(1);
}
```

---

## Formal Properties

A well-formed WRS specification should satisfy:

- **Weak Normalization (WN)**: Every term has a normal form
- **Unique Normalization (UN)**: Normal forms are unique
- **Confluence (CR)**: Rewrite order doesn't affect final result

WIRBLE-WRS does not automatically verify these properties. Use `fixpoint` strategies with iteration limits to ensure termination, and design rules carefully to maintain confluence.

---

## Limitations

- **Pattern matching** is structural only (no semantic analysis)
- **Guards** are evaluated at match time (no constraint solving)
- **Parallel strategies** are experimental (may not preserve semantics)
- **Type checking** in guards is limited to WIL type tags
- **Custom predicates** require manual implementation in C

---

## Summary

WIRBLE-WRS provides a declarative, maintainable way to specify optimizations on Sea-of-Nodes IR. By separating pattern matching, guards, and strategies, it enables rapid prototyping of optimization passes while maintaining formal TRS semantics. The S-expression syntax integrates naturally with the WIRBLE toolchain and supports both simple peephole optimizations and complex multi-rule transformations.