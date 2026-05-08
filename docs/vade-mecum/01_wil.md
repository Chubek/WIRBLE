# Chapter 1: WIL (WIRBLE Intermediate Language)

## Overview

WIL (WIRBLE Intermediate Language) is WIRBLE's high-level intermediate representation. It uses a graph-based approach inspired by Sea-of-Nodes and Graphical SSA, where nodes represent computations and edges represent dependencies.

## Design Philosophy

WIL is designed with several key goals:

1. **Direct AST Lowering**: No need to construct a CFG first
2. **Graph-Based**: Data and control flow unified in a single graph
3. **Optional SSA**: SSA form is not required for basic usage
4. **Pattern-Friendly**: Designed for pattern-based rewriting via WRS
5. **Flexible**: Supports both structured and unstructured control flow

## Core Types

### WILNodeId
```c
typedef unsigned long WILNodeId;
```
Unique identifier for nodes in the WIL graph.

### WILIndex
```c
typedef unsigned long WILIndex;
```
Index type for accessing node inputs and other collections.

### WILNodeCategory

Node categories classify nodes by their primary purpose:

```c
typedef enum {
    WIL_CAT_VALUE,      /* Produces a value (arithmetic, memory ops, etc.) */
    WIL_CAT_CONTROL,    /* Control flow (if, loop, region, etc.) */
    WIL_CAT_EFFECT,     /* Side effects (store, call, etc.) */
    WIL_CAT_CONSTANT,   /* Compile-time constants */
    WIL_CAT_PARAMETER   /* Function parameters */
} WILNodeCategory;
```

### WILNodeKind

Specific node types within each category:

#### Constants
- `WIL_NODE_CONST_INT` - Integer constant
- `WIL_NODE_CONST_FLOAT` - Floating-point constant
- `WIL_NODE_CONST_BOOL` - Boolean constant
- `WIL_NODE_UNDEF` - Undefined value

#### Parameters
- `WIL_NODE_PARAM` - Function parameter

#### Arithmetic (Binary)
- `WIL_NODE_ADD` - Addition
- `WIL_NODE_SUB` - Subtraction
- `WIL_NODE_MUL` - Multiplication
- `WIL_NODE_DIV` - Division
- `WIL_NODE_MOD` - Modulo
- `WIL_NODE_AND` - Bitwise AND
- `WIL_NODE_OR` - Bitwise OR
- `WIL_NODE_XOR` - Bitwise XOR
- `WIL_NODE_SHL` - Shift left
- `WIL_NODE_SHR` - Shift right

#### Arithmetic (Unary)
- `WIL_NODE_NEG` - Negation
- `WIL_NODE_NOT` - Bitwise NOT
- `WIL_NODE_ABS` - Absolute value

#### Comparison
- `WIL_NODE_EQ` - Equal
- `WIL_NODE_NE` - Not equal
- `WIL_NODE_LT` - Less than
- `WIL_NODE_LE` - Less than or equal
- `WIL_NODE_GT` - Greater than
- `WIL_NODE_GE` - Greater than or equal

#### Memory Operations
- `WIL_NODE_LOAD` - Load from memory
- `WIL_NODE_STORE` - Store to memory
- `WIL_NODE_ALLOCA` - Stack allocation

#### Control Flow
- `WIL_NODE_START` - Entry point of function
- `WIL_NODE_REGION` - Merge point (like basic block header)
- `WIL_NODE_IF` - Conditional branch
- `WIL_NODE_LOOP` - Loop header
- `WIL_NODE_RETURN` - Function return
- `WIL_NODE_JUMP` - Unconditional jump
- `WIL_NODE_PROJ` - Projection (extract control path from If/Loop)

#### Data Flow
- `WIL_NODE_PHI` - Merge values from multiple control paths

#### Function Calls
- `WIL_NODE_CALL` - Direct function call
- `WIL_NODE_CALL_INDIRECT` - Indirect function call

#### Type Operations
- `WIL_NODE_CAST` - Type cast
- `WIL_NODE_BITCAST` - Bitwise reinterpretation

## WIL Graph Structure

A WIL graph consists of:
- **Nodes**: Represent operations, constants, parameters, and control flow
- **Edges**: Represent data dependencies and control dependencies
- **Context**: Manages memory and graph state

### Node Inputs and Outputs

Each node has:
- **Inputs**: Dependencies (data or control)
- **Outputs**: Nodes that depend on this node
- **Type**: The type of value produced (if any)

## Building WIL Graphs

### Creating a Context

Before building WIL graphs, create a WIL context:

```c
WILContext *ctx = wilContextCreate();
```

### Creating Nodes

WIL provides builder functions for creating nodes. The exact API depends on the node type.

#### Constants

```c
WILNode *wilConstInt(WILContext *ctx, int64_t value);
WILNode *wilConstFloat(WILContext *ctx, double value);
WILNode *wilConstBool(WILContext *ctx, int value);
```

#### Arithmetic Operations

```c
WILNode *wilAdd(WILContext *ctx, WILNode *lhs, WILNode *rhs);
WILNode *wilSub(WILContext *ctx, WILNode *lhs, WILNode *rhs);
WILNode *wilMul(WILContext *ctx, WILNode *lhs, WILNode *rhs);
WILNode *wilDiv(WILContext *ctx, WILNode *lhs, WILNode *rhs);
```

#### Control Flow

```c
WILNode *wilStart(WILContext *ctx);
WILNode *wilRegion(WILContext *ctx, WILNode **inputs, uint32_t count);
WILNode *wilIf(WILContext *ctx, WILNode *control, WILNode *condition);
WILNode *wilReturn(WILContext *ctx, WILNode *control, WILNode *value);
```

#### Memory Operations

```c
WILNode *wilLoad(WILContext *ctx, WILNode *address, WILNode *effect);
WILNode *wilStore(WILContext *ctx, WILNode *address, WILNode *value, WILNode *effect);
WILNode *wilAlloca(WILContext *ctx, WILNode *size);
```

## Graph Traversal and Analysis

### Querying Node Properties

```c
WILNodeKind wilNodeGetKind(const WILNode *node);
WILNodeCategory wilNodeGetCategory(const WILNode *node);
uint32_t wilNodeInputCount(const WILNode *node);
WILNode *wilNodeGetInput(const WILNode *node, uint32_t index);
```

### Iterating Over Nodes

WIL graphs can be traversed in various orders:
- **Topological order**: Respects dependencies
- **Reverse postorder**: Common for control flow analysis
- **Custom traversal**: Using visitor patterns

## Validation

WIL graphs should be validated before lowering:

```c
int wilValidateGraph(WILContext *ctx, WILGraph *graph);
```

Validation checks:
- All inputs are defined before use
- Type consistency
- Control flow well-formedness
- No cycles in data dependencies (unless through PHI nodes)

## Printing and Debugging

WIL provides utilities for visualizing graphs:

```c
void wilPrintGraph(FILE *out, const WILGraph *graph);
void wilPrintNode(FILE *out, const WILNode *node);
```

Output formats:
- Human-readable text
- DOT format for Graphviz visualization

## Example: Building a Simple Function

```c
WILContext *ctx = wilContextCreate();

// Create function entry
WILNode *start = wilStart(ctx);

// Create parameters
WILNode *param0 = wilParam(ctx, 0);
WILNode *param1 = wilParam(ctx, 1);

// Compute: result = param0 + param1
WILNode *sum = wilAdd(ctx, param0, param1);

// Return the result
WILNode *ret = wilReturn(ctx, start, sum);

// Validate
if (!wilValidateGraph(ctx, graph)) {
    fprintf(stderr, "Invalid WIL graph\n");
}

// Print for debugging
wilPrintGraph(stdout, graph);

// Clean up
wilContextDestroy(ctx);
```

## Control Flow Patterns

### If-Then-Else

```c
WILNode *start = wilStart(ctx);
WILNode *condition = /* ... */;

// Create if node
WILNode *ifNode = wilIf(ctx, start, condition);

// Project true and false branches
WILNode *trueProj = wilProj(ctx, ifNode, 0);
WILNode *falseProj = wilProj(ctx, ifNode, 1);

// Compute in each branch
WILNode *trueValue = /* ... */;
WILNode *falseValue = /* ... */;

// Merge control flow
WILNode *merge = wilRegion(ctx, (WILNode*[]){trueProj, falseProj}, 2);

// Merge values with PHI
WILNode *result = wilPhi(ctx, merge, (WILNode*[]){trueValue, falseValue}, 2);
```

### Loops

```c
WILNode *start = wilStart(ctx);

// Create loop header
WILNode *loop = wilLoop(ctx, start);

// Loop body
WILNode *loopBody = /* ... */;

// Back edge
wilLoopSetBackEdge(ctx, loop, loopBody);

// Exit condition
WILNode *exitCond = /* ... */;
WILNode *loopExit = wilProj(ctx, loop, 1);
```

## Best Practices

1. **Validate Early**: Run validation after building graphs
2. **Use Projections**: Extract control paths explicitly with PROJ nodes
3. **Minimize PHI Nodes**: Only use where necessary for merging values
4. **Document Node Purpose**: Use comments to explain complex graph structures
5. **Leverage Patterns**: Design graphs with rewrite patterns in mind

## Integration with WRS

WIL graphs are designed to work seamlessly with the WIRBLE Rewrite System (WRS):
- Nodes match pattern constructors
- Graph structure enables efficient pattern matching
- Rewrite rules can transform subgraphs

See Chapter 3 for details on applying rewrite rules to WIL graphs.

## Lowering to MAL

Once WIL graphs are optimized, they can be lowered to MAL:

```c
MALModule *malModule = wilLowerToMAL(ctx, wilGraph);
```

The lowering process:
1. Schedules nodes into basic blocks
2. Converts graph edges to register dependencies
3. Generates MAL instructions
4. Preserves semantics while making control flow explicit

See Chapter 2 for details on MAL representation.
