#ifndef WIRBLE_WRS_H
#define WIRBLE_WRS_H

#include <stddef.h>
#include <stdint.h>

#include "api-boilerplate.h"
#include "wirble-wil.h"

WIRBLE_BEGIN_DECLS

/* ------------------------------------------------------------
   Forward Declarations
   ------------------------------------------------------------ */

typedef struct WILPattern WILPattern;
typedef struct WILRewriteRule WILRewriteRule;
typedef struct WILRewriteSystem WILRewriteSystem;
typedef struct WILMatch WILMatch;
typedef struct WILSubstitution WILSubstitution;
typedef struct WRSFile WRSFile;
typedef struct WRSSExpr WRSSExpr;

/* ------------------------------------------------------------
   Pattern Construction

   Patterns are constructor patterns (per TRS book, page 1):
   - Built from constructor symbols (WIL node kinds)
   - May contain pattern variables (for matching)
   - May contain bottom symbols ⊥ (wildcards)
   ------------------------------------------------------------ */

typedef enum
{
  WIL_PAT_VAR = 0,   /* Pattern variable (e.g., ?x, ?y) */
  WIL_PAT_CONST,     /* Constant value */
  WIL_PAT_NODE,      /* WIL node kind with subpatterns */
  WIL_PAT_WILDCARD,  /* Bottom symbol ⊥ — matches anything */
  WIL_PAT_PREDICATE, /* Custom predicate (C function) */
  WIL_PAT_SEQUENCE,  /* Match a sequence of nodes (for variadic ops) */
  WIL_PAT_REFINEMENT /* Refinement of another pattern */
} WILPatternKind;

/* Pattern variable */
WILPattern *wilPatVar (WILContext *ctx, const char *name);

/* Constant pattern */
WILPattern *wilPatConst (WILContext *ctx, int64_t value);
WILPattern *wilPatConstF64 (WILContext *ctx, double value);

/* Node pattern: matches a specific WIL node kind with subpatterns */
WILPattern *wilPatNode (WILContext *ctx, WILNodeKind kind, WILPattern **inputs,
                        uint32_t inputCount);

/* Wildcard: matches any node (bottom symbol ⊥_S) */
WILPattern *wilPatWildcard (WILContext *ctx);

/* Predicate: custom matching function */
typedef int (*WILPatternPredicate) (WILNode *node, void *userData);
WILPattern *wilPatPredicate (WILContext *ctx, WILPatternPredicate pred,
                             void *userData);

/* Sequence: matches a variable-length sequence of nodes */
WILPattern *wilPatSequence (WILContext *ctx, const char *name,
                            WILPattern *elementPattern);

/* Refinement: pattern u' is a refinement of u if u ▶ u' (replaces some
 * subterms with ⊥) */
WILPattern *wilPatRefinement (WILContext *ctx, WILPattern *base);

/* Query pattern properties */
WILPatternKind wilPatternGetKind (const WILPattern *pat);
const char *wilPatternGetVarName (const WILPattern *pat);
WILNodeKind wilPatternGetNodeKind (const WILPattern *pat);
uint32_t wilPatternInputCount (const WILPattern *pat);
WILPattern *wilPatternGetInput (const WILPattern *pat, uint32_t index);

/* Destroy pattern */
void wilPatternDestroy (WILContext *ctx, WILPattern *pat);

/* ------------------------------------------------------------
   Rewrite Rules

   Per TRS Definition 12 (page 1):
   - A rule is a pair (l, r) where:
     1. l is not a variable
     2. vars(r) ⊆ vars(l)
     3. srt(l) = srt(r) (sort-consistent)
   ------------------------------------------------------------ */

/* Action function: given a match, produce the replacement node */
typedef WILNode *(*WILRewriteAction) (WILContext *ctx, WILMatch *match,
                                      void *userData);

/* Create a rewrite rule: pattern -> action */
WILRewriteRule *wilRuleCreate (WILContext *ctx, const char *name,
                               WILPattern *lhs, WILRewriteAction action,
                               void *userData);

/* Create a simple pattern-to-pattern rule (no custom action) */
WILRewriteRule *wilRuleCreateSimple (WILContext *ctx, const char *name,
                                     WILPattern *lhs, WILPattern *rhs);

/* Query rule properties */
const char *wilRuleGetName (const WILRewriteRule *rule);
WILPattern *wilRuleGetLHS (const WILRewriteRule *rule);
WILRewriteAction wilRuleGetAction (const WILRewriteRule *rule);

/* Validate rule (checks vars(r) ⊆ vars(l), l is not a variable) */
int wilRuleValidate (const WILRewriteRule *rule);

/* Destroy rule */
void wilRuleDestroy (WILContext *ctx, WILRewriteRule *rule);

/* ------------------------------------------------------------
   Rewrite System

   A collection of rewrite rules with strategies for application.
   ------------------------------------------------------------ */

WILRewriteSystem *wilRewriteSystemCreate (WILContext *ctx);
void wilRewriteSystemDestroy (WILContext *ctx, WILRewriteSystem *sys);

/* Add/remove rules */
void wilRewriteSystemAddRule (WILRewriteSystem *sys, WILRewriteRule *rule);
void wilRewriteSystemRemoveRule (WILRewriteSystem *sys, const char *ruleName);
void wilRewriteSystemClearRules (WILRewriteSystem *sys);

/* Query rules */
uint32_t wilRewriteSystemRuleCount (const WILRewriteSystem *sys);
WILRewriteRule *wilRewriteSystemGetRule (const WILRewriteSystem *sys,
                                         uint32_t index);
WILRewriteRule *wilRewriteSystemFindRule (const WILRewriteSystem *sys,
                                          const char *name);

/* Enable/disable specific rules */
void wilRewriteSystemEnableRule (WILRewriteSystem *sys, const char *name);
void wilRewriteSystemDisableRule (WILRewriteSystem *sys, const char *name);
int wilRewriteSystemIsRuleEnabled (const WILRewriteSystem *sys,
                                   const char *name);

/* ------------------------------------------------------------
   Pattern Matching

   Per TRS Definition 13 (page 1):
   - Match t/p ≡ lσ where σ : X → T is a substitution
   ------------------------------------------------------------ */

/* Match a pattern against a node, returning a match object with substitution σ
 */
WILMatch *wilPatternMatch (WILContext *ctx, const WILPattern *pat,
                           WILNode *node);

/* Check if a match succeeded */
int wilMatchSucceeded (const WILMatch *match);

/* Destroy match */
void wilMatchDestroy (WILContext *ctx, WILMatch *match);

/* ------------------------------------------------------------
   Substitution Queries

   Substitution σ : X → T maps pattern variables to matched nodes.
   ------------------------------------------------------------ */

WILSubstitution *wilMatchGetSubstitution (const WILMatch *match);

/* Look up a variable binding in the substitution */
WILNode *wilSubstitutionLookup (const WILSubstitution *subst,
                                const char *varName);
int wilSubstitutionHas (const WILSubstitution *subst, const char *varName);
uint32_t wilSubstitutionVarCount (const WILSubstitution *subst);
const char *wilSubstitutionVarName (const WILSubstitution *subst,
                                    uint32_t index);

/* Apply substitution to a pattern to produce a node */
WILNode *wilSubstitutionApply (WILContext *ctx, const WILSubstitution *subst,
                               const WILPattern *pat);

/* ------------------------------------------------------------
   Rewrite Application Strategies

   Different traversal orders for applying rewrites.
   ------------------------------------------------------------ */

typedef enum
{
  WIL_STRATEGY_INNERMOST = 0, /* Bottom-up: rewrite leaves first */
  WIL_STRATEGY_OUTERMOST,     /* Top-down: rewrite root first */
  WIL_STRATEGY_LEFTMOST,      /* Left-to-right depth-first */
  WIL_STRATEGY_PARALLEL, /* Apply all non-overlapping rewrites simultaneously
                          */
  WIL_STRATEGY_CUSTOM    /* User-defined strategy function */
} WILRewriteStrategy;

/* Custom strategy function: given a node, return the order to visit children
 */
typedef void (*WILCustomStrategy) (WILNode *node, WILNode **visitOrder,
                                   uint32_t *visitCount, void *userData);

/* Set the rewrite strategy */
void wilRewriteSystemSetStrategy (WILRewriteSystem *sys,
                                  WILRewriteStrategy strategy);
void wilRewriteSystemSetCustomStrategy (WILRewriteSystem *sys,
                                        WILCustomStrategy fn, void *userData);

/* ------------------------------------------------------------
   Applying Rewrites

   Per TRS Definition 13 (page 1):
   - Single-step reduction: t →_p s where s ≡ t{p ↦ rσ}
   ------------------------------------------------------------ */

/* Apply one rewrite step at a specific node (returns new node or NULL if no
 * match) */
WILNode *wilRewriteSystemApplyAt (WILRewriteSystem *sys, WILNode *node);

/* Apply rewrites to an entire graph (single pass) */
void wilRewriteSystemApply (WILRewriteSystem *sys, WILNode *root);

/* Apply rewrites to fixpoint (iterate until no more rewrites apply) */
void wilRewriteSystemApplyToFixpoint (WILRewriteSystem *sys, WILNode *root,
                                      uint32_t maxIterations);

/* Apply a single named rule to a node */
WILNode *wilRewriteSystemApplyRule (WILRewriteSystem *sys,
                                    const char *ruleName, WILNode *node);

/* ------------------------------------------------------------
   Normal Forms

   Per TRS book (page 1):
   - A term is in normal form if no rewrite rules apply.
   - WN (weakly normalizing): every proper ground term has a normal form.
   - UN (uniquely normalizing): the normal form is uniquely determined.
   - CR (infinitarily confluent): if t →→→ s and t →→→ s', then ∃u: s →→→ u ∧
   s' →→→ u.
   ------------------------------------------------------------ */

/* Check if a node is in normal form (no rules apply) */
int wilRewriteSystemIsNormalForm (const WILRewriteSystem *sys, WILNode *node);

/* Check if the entire graph rooted at node is in normal form */
int wilRewriteSystemIsGraphNormalForm (const WILRewriteSystem *sys,
                                       WILNode *root);

/* Compute the normal form of a node (apply to fixpoint) */
WILNode *wilRewriteSystemNormalize (WILRewriteSystem *sys, WILNode *node,
                                    uint32_t maxIterations);

/* ------------------------------------------------------------
   Confluence and Termination Checks (Heuristic)

   These are heuristic checks, not formal proofs.
   ------------------------------------------------------------ */

/* Check if the rewrite system appears to be confluent (heuristic) */
int wilRewriteSystemCheckConfluence (const WILRewriteSystem *sys);

/* Check if the rewrite system appears to terminate (heuristic) */
int wilRewriteSystemCheckTermination (const WILRewriteSystem *sys);

/* Check for critical pairs (potential confluence issues) */
typedef struct
{
  WILRewriteRule *rule1;
  WILRewriteRule *rule2;
  WILPattern *overlap;
} WILCriticalPair;

uint32_t wilRewriteSystemFindCriticalPairs (const WILRewriteSystem *sys,
                                            WILCriticalPair **outPairs);
void wilCriticalPairsFree (WILContext *ctx, WILCriticalPair *pairs,
                           uint32_t count);

/* ------------------------------------------------------------
   Built-in Optimization Rules

   Common algebraic simplifications and optimizations.
   ------------------------------------------------------------ */

/* Add standard algebraic simplification rules */
void wilRewriteSystemAddAlgebraicRules (WILRewriteSystem *sys);

/* Add constant folding rules */
void wilRewriteSystemAddConstantFoldingRules (WILRewriteSystem *sys);

/* Add strength reduction rules (e.g., x * 2 -> x << 1) */
void wilRewriteSystemAddStrengthReductionRules (WILRewriteSystem *sys);

/* Add identity elimination rules (e.g., x + 0 -> x, x * 1 -> x) */
void wilRewriteSystemAddIdentityRules (WILRewriteSystem *sys);

/* Add all built-in optimization rules */
void wilRewriteSystemAddAllBuiltinRules (WILRewriteSystem *sys);

/* ------------------------------------------------------------
   S-Expression Representation

   Internal representation for parsing `.wrs` files.
   ------------------------------------------------------------ */

typedef enum
{
  WRS_SEXPR_ATOM = 0, /* Atom (symbol or literal) */
  WRS_SEXPR_LIST,     /* List (nested S-expression) */
  WRS_SEXPR_STRING,   /* String literal */
  WRS_SEXPR_NUMBER    /* Numeric literal */
} WRSSExprKind;

struct WRSSExpr
{
  WRSSExprKind kind;
  union
  {
    char *atom;      /* For ATOM and STRING */
    int64_t intVal;  /* For NUMBER (integer) */
    double floatVal; /* For NUMBER (float) */
    struct
    {
      WRSSExpr **elements;
      uint32_t count;
    } list; /* For LIST */
  };
};

/* Parse S-expression from string */
WRSSExpr *wrsSExprParse (WILContext *ctx, const char *source);

/* Destroy S-expression */
void wrsSExprDestroy (WILContext *ctx, WRSSExpr *expr);

/* Query S-expression */
WRSSExprKind wrsSExprGetKind (const WRSSExpr *expr);
const char *wrsSExprGetAtom (const WRSSExpr *expr);
int64_t wrsSExprGetInt (const WRSSExpr *expr);
double wrsSExprGetFloat (const WRSSExpr *expr);
uint32_t wrsSExprListLength (const WRSSExpr *expr);
WRSSExpr *wrsSExprListGet (const WRSSExpr *expr, uint32_t index);

/* Print S-expression to string (caller frees) */
char *wrsSExprToString (WILContext *ctx, const WRSSExpr *expr);

/* ------------------------------------------------------------
   .wrs File Format

   S-expression-based rewrite specification files.

   Example syntax:

   (defrule add-zero
     (pattern (Add ?x (Const 0)))
     (replacement ?x))

   (defrule mul-by-two
     (pattern (Mul ?x (Const 2)))
     (replacement (Shl ?x (Const 1))))

   (defrule constant-fold-add
     (pattern (Add (Const ?a) (Const ?b)))
     (replacement (Const (+ ?a ?b))))

   (defstrategy innermost)

   (defpredicate is-power-of-two
     (lambda (?x) (and (is-const ?x) (power-of-two? (const-value ?x)))))
   ------------------------------------------------------------ */

/* Load a .wrs file and populate a rewrite system */
WRSFile *wrsFileLoad (WILContext *ctx, const char *path);
WRSFile *wrsFileLoadFromString (WILContext *ctx, const char *source);

/* Query .wrs file contents */
uint32_t wrsFileRuleCount (const WRSFile *file);
WILRewriteRule *wrsFileGetRule (const WRSFile *file, uint32_t index);
WILRewriteStrategy wrsFileGetStrategy (const WRSFile *file);

/* Apply all rules from a .wrs file to a rewrite system */
void wrsFileApplyToSystem (const WRSFile *file, WILRewriteSystem *sys);

/* Destroy .wrs file */
void wrsFileDestroy (WILContext *ctx, WRSFile *file);

/* Validate a .wrs file (check syntax and rule validity) */
int wrsFileValidate (WILContext *ctx, const char *path, char **errorMsg);

/* ------------------------------------------------------------
   .wrs File Writing

   Generate .wrs files from a rewrite system.
   ------------------------------------------------------------ */

/* Write a rewrite system to a .wrs file */
int wrsFileWrite (WILContext *ctx, const WILRewriteSystem *sys,
                  const char *path);

/* Convert a rewrite system to a .wrs string (caller frees) */
char *wrsFileToString (WILContext *ctx, const WILRewriteSystem *sys);

/* Convert a single rule to S-expression */
WRSSExpr *wrsRuleToSExpr (WILContext *ctx, const WILRewriteRule *rule);

/* Convert a pattern to S-expression */
WRSSExpr *wrsPatternToSExpr (WILContext *ctx, const WILPattern *pat);

/* ------------------------------------------------------------
   .wrs Syntax Specification

   Top-level forms:

   (defrule <name> <pattern> <replacement> [<guard>])
     Define a rewrite rule.
     - <name>: symbol
     - <pattern>: pattern S-expression
     - <replacement>: pattern S-expression or (action <code>)
     - <guard>: optional predicate

   (defstrategy <strategy>)
     Set the rewrite strategy.
     - <strategy>: innermost | outermost | leftmost | parallel

   (defpredicate <name> <lambda>)
     Define a custom predicate.
     - <name>: symbol
     - <lambda>: (lambda (<args>) <body>)

   Pattern syntax:

   ?var                  Pattern variable
   (Const <value>)       Constant pattern
   (<NodeKind> <args>)   Node pattern
   _                     Wildcard (⊥)
   (seq ?var <pattern>)  Sequence pattern
   (refine <pattern>)    Refinement pattern
   (pred <name> <args>)  Predicate pattern

   Replacement syntax:

   ?var                  Substitute matched variable
   (Const <value>)       Constant node
   (<NodeKind> <args>)   Construct node
   (action <code>)       Custom action (C function name)

   Guard syntax:

   (guard <predicate>)   Apply predicate to match
   ------------------------------------------------------------ */

/* Parse a pattern from S-expression */
WILPattern *wrsPatternFromSExpr (WILContext *ctx, const WRSSExpr *expr);

/* Parse a rule from S-expression */
WILRewriteRule *wrsRuleFromSExpr (WILContext *ctx, const WRSSExpr *expr);

/* Parse a strategy from S-expression */
WILRewriteStrategy wrsStrategyFromSExpr (const WRSSExpr *expr);

/* ------------------------------------------------------------
   Debugging Utilities
   ------------------------------------------------------------ */

/* Print a pattern to a string (caller frees) */
char *wilPatternToString (WILContext *ctx, const WILPattern *pat);

/* Print a rule to a string (caller frees) */
char *wilRuleToString (WILContext *ctx, const WILRewriteRule *rule);

/* Print a rewrite system to a string (caller frees) */
char *wilRewriteSystemToString (WILContext *ctx, const WILRewriteSystem *sys);

/* Print a match to a string (caller frees) */
char *wilMatchToString (WILContext *ctx, const WILMatch *match);

/* Print a substitution to a string (caller frees) */
char *wilSubstitutionToString (WILContext *ctx, const WILSubstitution *subst);

/* Enable/disable rewrite tracing (prints each rewrite step) */
void wilRewriteSystemSetTracing (WILRewriteSystem *sys, int enabled);

/* Get rewrite statistics */
typedef struct
{
  uint32_t totalRewrites;
  uint32_t rulesApplied[256]; /* Per-rule application count */
  uint32_t iterations;
  uint32_t normalFormsReached;
} WILRewriteStats;

const WILRewriteStats *wilRewriteSystemGetStats (const WILRewriteSystem *sys);
void wilRewriteSystemResetStats (WILRewriteSystem *sys);

/* Dump rewrite system to FILE* */
void wilRewriteSystemDump (const WILRewriteSystem *sys, void *file);

WIRBLE_END_DECLS

#endif /* WIRBLE_WRS_H */
