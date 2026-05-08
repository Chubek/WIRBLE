# Chapter 3: WRS (WIRBLE Rewrite System)

## Overview

WRS (WIRBLE Rewrite System) is WIRBLE's pattern-based transformation engine. It implements a term rewriting system (TRS) based on formal rewriting theory, enabling powerful and composable optimizations on both WIL and MAL representations.

## Theoretical Foundation

WRS is based on Term Rewriting Systems (TRS) theory:

1. **Terms**: WIL nodes and MAL instructions are terms
2. **Patterns**: Constructor patterns with variables
3. **Rules**: Pairs (l, r) where l → r
4. **Matching**: Finding substitutions that make patterns equal to terms
5. **Rewriting**: Replacing matched terms with instantiated right-hand sides

### Key Properties

- **Confluence**: Multiple rewrite paths lead to the same result
- **Termination**: Rewriting eventually stops
- **Correctness**: Rewrites preserve semantics

## Core Types

### Pattern Types

```c
typedef enum {
    WIL_PAT_VAR,        // Pattern variable (e.g., ?x, ?y)
    WIL_PAT_CONST,      // Constant value
    WIL_PAT_NODE,       // WIL node kind with subpatterns
    WIL_PAT_WILDCARD,   // Bottom symbol ⊥ — matches anything
    WIL_PAT_PREDICATE,  // Custom predicate (C function)
    WIL_PAT_SEQUENCE,   // Match a sequence of nodes (for variadic ops)
    WIL_PAT_REFINEMENT  // Refinement of another pattern
} WILPatternKind;
```

### Forward Declarations

```c
typedef struct WILPattern WILPattern;
typedef struct WILRewriteRule WILRewriteRule;
typedef struct WILRewriteSystem WILRewriteSystem;
typedef struct WILMatch WILMatch;
typedef struct WILSubstitution WILSubstitution;
typedef struct WRSFile WRSFile;
typedef struct WRSSExpr WRSSExpr;
```

## Pattern Construction

### Pattern Variables

Pattern variables match any term and bind to it:

```c
WILPattern *wilPatVar(WILContext *ctx, const char *name);
```

Example: `?x` matches any node and binds it to variable `x`.

### Constant Patterns

Match specific constant values:

```c
WILPattern *wilPatConst(WILContext *ctx, int64_t value);
WILPattern *wilPatConstF64(WILContext *ctx, double value);
```

Example: `42` matches only the integer constant 42.

### Node Patterns

Match specific WIL node kinds with subpatterns:

```c
WILPattern *wilPatNode(WILContext *ctx, WILNodeKind kind, 
                       WILPattern **inputs, uint32_t inputCount);
```

Example: `ADD(?x, ?y)` matches any addition node.

### Wildcard Patterns

Match anything without binding (bottom symbol ⊥):

```c
WILPattern *wilPatWildcard(WILContext *ctx);
```

Example: `ADD(_, _)` matches any addition, ignoring operands.

### Predicate Patterns

Custom matching logic:

```c
typedef int (*WILPatternPredicate)(WILNode *node, void *userData);
WILPattern *wilPatPredicate(WILContext *ctx, WILPatternPredicate pred, 
                            void *userData);
```

Example: Match only power-of-two constants.

### Sequence Patterns

Match variable-length sequences:

```c
WILPattern *wilPatSequence(WILContext *ctx, const char *name, 
                           WILPattern *elementPattern);
```

Example: `CALL(func, ?args...)` matches calls with any number of arguments.

### Refinement Patterns

Refine existing patterns by replacing subterms with ⊥:

```c
WILPattern *wilPatRefinement(WILContext *ctx, WILPattern *base);
```

## Pattern Queries

```c
WILPatternKind wilPatternGetKind(const WILPattern *pat);
const char *wilPatternGetVarName(const WILPattern *pat);
WILNodeKind wilPatternGetNodeKind(const WILPattern *pat);
uint32_t wilPatternInputCount(const WILPattern *pat);
WILPattern *wilPatternGetInput(const WILPattern *pat, uint32_t index);
```

## Rewrite Rules

### Rule Definition

A rewrite rule is a pair (l, r) with constraints:
1. l is not a variable
2. vars(r) ⊆ vars(l) (all RHS variables appear in LHS)
3. srt(l) = srt(r) (sort-consistent)

### Creating Rules

#### With Action Function

```c
typedef WILNode *(*WILRewriteAction)(WILContext *ctx, WILMatch *match, 
                                     void *userData);

WILRewriteRule *wilRuleCreate(WILContext *ctx, const char *name,
                              WILPattern *lhs, WILRewriteAction action,
                              void *userData);
```

#### Pattern-to-Pattern Rules

```c
WILRewriteRule *wilRuleCreateSimple(WILContext *ctx, const char *name,
                                    WILPattern *lhs, WILPattern *rhs);
```

### Rule Properties

```c
const char *wilRuleGetName(const WILRewriteRule *rule);
WILPattern *wilRuleGetLHS(const WILRewriteRule *rule);
int wilRuleIsEnabled(const WILRewriteRule *rule);
void wilRuleSetEnabled(WILRewriteRule *rule, int enabled);
```

## Rewrite Systems

### Creating a Rewrite System

```c
WILRewriteSystem *wilRewriteSystemCreate(WILContext *ctx);
```

### Adding Rules

```c
void wilRewriteSystemAddRule(WILRewriteSystem *sys, WILRewriteRule *rule);
void wilRewriteSystemRemoveRule(WILRewriteSystem *sys, WILRewriteRule *rule);
```

### Rule Ordering

Rules can be prioritized:

```c
void wilRewriteSystemSetRulePriority(WILRewriteSystem *sys, 
                                     WILRewriteRule *rule, int priority);
```

Higher priority rules are tried first.

## Pattern Matching

### Match Structure

```c
typedef struct WILMatch {
    WILNode *node;              // Matched node
    WILRewriteRule *rule;       // Matching rule
    WILSubstitution *subst;     // Variable bindings
} WILMatch;
```

### Matching API

```c
WILMatch *wilPatternMatch(WILContext *ctx, WILPattern *pattern, WILNode *node);
int wilPatternMatches(WILPattern *pattern, WILNode *node);
```

### Accessing Bindings

```c
WILNode *wilMatchGetBinding(WILMatch *match, const char *varName);
uint32_t wilMatchBindingCount(WILMatch *match);
```

## Applying Rewrites

### Single Rewrite

Apply one rule at one location:

```c
WILNode *wilRewriteApply(WILContext *ctx, WILRewriteRule *rule, WILNode *node);
```

### Exhaustive Rewriting

Apply rules until no more matches:

```c
void wilRewriteExhaustive(WILContext *ctx, WILRewriteSystem *sys, 
                          WILGraph *graph);
```

### Controlled Rewriting

Apply rules with limits:

```c
typedef struct WILRewriteConfig {
    int maxIterations;      // Maximum rewrite iterations
    int maxRuleApps;        // Maximum rule applications
    int strategy;           // Rewrite strategy
} WILRewriteConfig;

void wilRewriteWithConfig(WILContext *ctx, WILRewriteSystem *sys,
                          WILGraph *graph, WILRewriteConfig *config);
```

### Rewrite Strategies

- **Innermost**: Rewrite leaves first, then parents
- **Outermost**: Rewrite roots first, then children
- **Leftmost**: Rewrite leftmost matches first
- **Parallel**: Apply all non-overlapping matches simultaneously

## Example Rewrite Rules

### Constant Folding

```c
// Rule: ADD(CONST(a), CONST(b)) → CONST(a + b)
WILPattern *lhs = wilPatNode(ctx, WIL_NODE_ADD,
    (WILPattern*[]){
        wilPatNode(ctx, WIL_NODE_CONST_INT, NULL, 0),
        wilPatNode(ctx, WIL_NODE_CONST_INT, NULL, 0)
    }, 2);

WILNode *constantFoldAdd(WILContext *ctx, WILMatch *match, void *ud) {
    WILNode *left = wilMatchGetBinding(match, "a");
    WILNode *right = wilMatchGetBinding(match, "b");
    int64_t a = wilConstGetInt(left);
    int64_t b = wilConstGetInt(right);
    return wilConstInt(ctx, a + b);
}

WILRewriteRule *rule = wilRuleCreate(ctx, "const-fold-add", lhs, 
                                     constantFoldAdd, NULL);
```

### Algebraic Simplification

```c
// Rule: ADD(?x, 0) → ?x
WILPattern *lhs = wilPatNode(ctx, WIL_NODE_ADD,
    (WILPattern*[]){
        wilPatVar(ctx, "x"),
        wilPatConst(ctx, 0)
    }, 2);

WILPattern *rhs = wilPatVar(ctx, "x");

WILRewriteRule *rule = wilRuleCreateSimple(ctx, "add-zero", lhs, rhs);
```

### Strength Reduction

```c
// Rule: MUL(?x, 2) → SHL(?x, 1)
WILPattern *lhs = wilPatNode(ctx, WIL_NODE_MUL,
    (WILPattern*[]){
        wilPatVar(ctx, "x"),
        wilPatConst(ctx, 2)
    }, 2);

WILNode *mulToShift(WILContext *ctx, WILMatch *match, void *ud) {
    WILNode *x = wilMatchGetBinding(match, "x");
    WILNode *one = wilConstInt(ctx, 1);
    return wilShl(ctx, x, one);
}

WILRewriteRule *rule = wilRuleCreate(ctx, "mul-to-shift", lhs, 
                                     mulToShift, NULL);
```

### Dead Code Elimination

```c
// Rule: If a node has no uses, remove it
WILPattern *lhs = wilPatPredicate(ctx, hasNoUses, NULL);

WILNode *removeDeadCode(WILContext *ctx, WILMatch *match, void *ud) {
    return NULL; // Remove the node
}

WILRewriteRule *rule = wilRuleCreate(ctx, "dce", lhs, removeDeadCode, NULL);
```

## Loading Rules from Files

WRS supports loading rules from external files:

```c
WRSFile *wrsLoadFile(WILContext *ctx, const char *path);
WILRewriteSystem *wrsFileToSystem(WILContext *ctx, WRSFile *file);
```

### Rule File Format

Rules can be specified in S-expression format:

```lisp
(rule add-zero
  (pattern (ADD ?x (CONST 0)))
  (replacement ?x))

(rule mul-to-shift
  (pattern (MUL ?x (CONST 2)))
  (replacement (SHL ?x (CONST 1))))

(rule const-fold-add
  (pattern (ADD (CONST ?a) (CONST ?b)))
  (action (lambda (match)
    (const (+ (get-binding match 'a)
              (get-binding match 'b))))))
```

## S-Expression Integration

WRS uses S-expressions for rule representation:

```c
typedef struct WRSSExpr WRSSExpr;

WRSSExpr *wrsSExprParse(const char *text);
WILPattern *wrsSExprToPattern(WILContext *ctx, WRSSExpr *sexpr);
WILRewriteRule *wrsSExprToRule(WILContext *ctx, WRSSExpr *sexpr);
```

## Debugging Rewrites

### Tracing

Enable rewrite tracing:

```c
void wilRewriteSystemSetTrace(WILRewriteSystem *sys, int enabled);
void wilRewriteSystemSetTraceFile(WILRewriteSystem *sys, FILE *out);
```

Output shows:
- Which rules matched
- Where they matched
- What substitutions were made
- The resulting term

### Statistics

```c
typedef struct WILRewriteStats {
    uint64_t rulesApplied;
    uint64_t matchesAttempted;
    uint64_t matchesSucceeded;
    uint64_t nodesCreated;
    uint64_t nodesDeleted;
} WILRewriteStats;

WILRewriteStats *wilRewriteSystemGetStats(WILRewriteSystem *sys);
```

## Best Practices

1. **Order Rules by Specificity**: More specific rules should have higher priority
2. **Ensure Termination**: Avoid rules that can apply indefinitely
3. **Test Rules Independently**: Verify each rule in isolation
4. **Use Predicates Sparingly**: They can be expensive
5. **Document Rule Intent**: Explain what optimization each rule performs
6. **Validate After Rewriting**: Ensure graph remains well-formed
7. **Profile Rule Performance**: Identify slow or frequently-applied rules

## Common Optimization Patterns

### Peephole Optimizations
- Constant folding
- Algebraic simplifications
- Strength reduction
- Identity elimination

### Control Flow Optimizations
- Branch elimination
- Loop invariant code motion
- Dead code elimination

### Data Flow Optimizations
- Common subexpression elimination
- Copy propagation
- Constant propagation

## Integration with WIL

Apply rewrite system to WIL graphs:

```c
WILRewriteSystem *sys = wilRewriteSystemCreate(ctx);

// Add optimization rules
wilRewriteSystemAddRule(sys, constFoldRule);
wilRewriteSystemAddRule(sys, algebraicSimplRule);
wilRewriteSystemAddRule(sys, strengthReduceRule);

// Apply to graph
wilRewriteExhaustive(ctx, sys, wilGraph);

// Validate result
wilValidateGraph(ctx, wilGraph);
```

## Integration with MAL

Similar rewrite systems can be built for MAL:

```c
MALRewriteSystem *malSys = malRewriteSystemCreate();
malRewriteSystemAddRule(malSys, peepholeRule);
malRewriteExhaustive(malSys, malFunction);
```

## Advanced Topics

### Conditional Rewriting

Rules can have conditions:

```c
WILRewriteRule *wilRuleCreateConditional(WILContext *ctx, const char *name,
                                         WILPattern *lhs, WILPattern *rhs,
                                         WILCondition *condition);
```

### Contextual Rewriting

Rules can access surrounding context:

```c
typedef WILNode *(*WILContextualAction)(WILContext *ctx, WILMatch *match,
                                        WILNode *parent, void *userData);
```

### Rewrite Strategies

Custom strategies can be implemented:

```c
typedef int (*WILRewriteStrategy)(WILRewriteSystem *sys, WILGraph *graph,
                                  void *userData);
```

## Performance Considerations

- **Pattern Compilation**: Patterns are compiled to efficient matchers
- **Indexing**: Nodes are indexed by kind for fast lookup
- **Memoization**: Match results can be cached
- **Incremental Rewriting**: Only rewrite changed parts of graph

## Conclusion

WRS provides a powerful, principled approach to IR transformation. By expressing optimizations as rewrite rules, WIRBLE enables:
- Composable optimizations
- Verifiable correctness
- Easy experimentation
- Maintainable optimization passes
