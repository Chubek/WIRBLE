#ifndef WIRBLE_WRS_H
#define WIRBLE_WRS_H

#include "api-boilerplate.h"
#include "wirble-wil.h"

/* ------------------------------------------------------------
   WIL Rewrite System (WRS)
   Pattern-based term rewriting for Sea of Nodes optimization
   ------------------------------------------------------------ */

typedef struct WILPattern WILPattern;
typedef struct WILRewriteRule WILRewriteRule;
typedef struct WILRewriteSystem WILRewriteSystem;
typedef struct WILMatch WILMatch;
typedef struct WILSubstitution WILSubstitution;

WIRBLE_BEGIN_DECLS

/* ------------------------------------------------------------
   Pattern Construction
   ------------------------------------------------------------ */

/* Pattern variables bind to any node */
WILPattern *wilPatternVar (const char *name);

/* Pattern constants match specific values */
WILPattern *wilPatternConst (double value);

/* Pattern operators match specific opcodes with subpatterns */
WILPattern *wilPatternOp (WILOpCode opcode, WILPattern **subpatterns,
                          WILIndex count);

/* Pattern wildcards match any single node */
WILPattern *wilPatternWildcard (void);

/* Pattern predicates: match only if predicate returns true */
typedef int (*WILPatternPredicate) (const WILNode *node, void *userData);
WILPattern *wilPatternWithPredicate (WILPattern *base,
                                     WILPatternPredicate pred, void *userData);

/* Destroy pattern */
void wilDestroyPattern (WILPattern *pattern);

/* ------------------------------------------------------------
   Rewrite Rule Construction
   ------------------------------------------------------------ */

/* Create a rewrite rule: pattern -> action */
typedef WILNode *(*WILRewriteAction) (WILContext *ctx,
                                      const WILSubstitution *subst,
                                      void *userData);

WILRewriteRule *wilCreateRule (const char *name, WILPattern *pattern,
                               WILRewriteAction action, void *userData);

void wilDestroyRule (WILRewriteRule *rule);

/* ------------------------------------------------------------
   Rewrite System Management
   ------------------------------------------------------------ */

WILRewriteSystem *wilCreateRewriteSystem (void);
void wilDestroyRewriteSystem (WILRewriteSystem *sys);

/* Add rule to system */
void wilAddRule (WILRewriteSystem *sys, WILRewriteRule *rule);

/* Remove rule by name */
void wilRemoveRule (WILRewriteSystem *sys, const char *name);

/* ------------------------------------------------------------
   Pattern Matching
   ------------------------------------------------------------ */

/* Match pattern against a node, returns NULL if no match */
WILMatch *wilMatchPattern (const WILPattern *pattern, const WILNode *node);

/* Destroy match */
void wilDestroyMatch (WILMatch *match);

/* Extract substitution from match */
WILSubstitution *wilMatchSubstitution (const WILMatch *match);

/* ------------------------------------------------------------
   Substitution Queries
   ------------------------------------------------------------ */

/* Look up bound node by variable name */
WILNode *wilSubstLookup (const WILSubstitution *subst, const char *varName);

/* Get all variable names in substitution */
WILIndex wilSubstVarCount (const WILSubstitution *subst);
const char *wilSubstVarName (const WILSubstitution *subst, WILIndex index);

/* ------------------------------------------------------------
   Rewrite Strategies
   ------------------------------------------------------------ */

typedef enum
{
  WIL_STRATEGY_INNERMOST, /* Bottom-up: rewrite leaves first */
  WIL_STRATEGY_OUTERMOST, /* Top-down: rewrite root first */
  WIL_STRATEGY_LEFTMOST,  /* Left-to-right traversal */
  WIL_STRATEGY_PARALLEL   /* Apply all non-overlapping rules simultaneously */
} WILRewriteStrategy;

/* Apply rewrite system to graph with given strategy */
/* Returns number of rewrites performed */
WILIndex wilApplyRewrites (WILContext *ctx, WILRewriteSystem *sys,
                           WILRewriteStrategy strategy);

/* Apply single rewrite step (one rule application) */
/* Returns 1 if a rewrite was performed, 0 otherwise */
int wilApplyOneRewrite (WILContext *ctx, WILRewriteSystem *sys,
                        WILRewriteStrategy strategy);

/* Apply rewrites until no more rules match (fixpoint) */
/* Returns total number of rewrites performed */
WILIndex wilApplyRewritesToFixpoint (WILContext *ctx, WILRewriteSystem *sys,
                                     WILRewriteStrategy strategy,
                                     WILIndex maxIterations /* 0 = unlimited */
);

/* ------------------------------------------------------------
   Normal Form and Confluence Checking
   ------------------------------------------------------------ */

/* Check if node is in normal form (no rules apply) */
int wilIsNormalForm (const WILRewriteSystem *sys, const WILNode *node);

/* Check if entire graph is in normal form */
int wilIsGraphNormalForm (const WILRewriteSystem *sys, const WILContext *ctx);

/* Check if rewrite system is confluent (Church-Rosser property) */
/* Warning: may be expensive or undecidable for complex systems */
int wilCheckConfluence (const WILRewriteSystem *sys);

/* Check if rewrite system is terminating */
/* Warning: undecidable in general, uses heuristics */
int wilCheckTermination (const WILRewriteSystem *sys);

/* ------------------------------------------------------------
   Built-in Optimization Rules
   ------------------------------------------------------------ */

/* Add standard algebraic simplification rules */
void wilAddAlgebraicRules (WILRewriteSystem *sys);

/* Add constant folding rules */
void wilAddConstantFoldingRules (WILRewriteSystem *sys);

/* Add strength reduction rules (e.g., x*2 -> x<<1) */
void wilAddStrengthReductionRules (WILRewriteSystem *sys);

/* Add identity elimination rules (e.g., x+0 -> x) */
void wilAddIdentityRules (WILRewriteSystem *sys);

/* ------------------------------------------------------------
   Debug and Introspection
   ------------------------------------------------------------ */

/* Print pattern */
void wilPrintPattern (const WILPattern *pattern);

/* Print rule */
void wilPrintRule (const WILRewriteRule *rule);

/* Print all rules in system */
void wilPrintRewriteSystem (const WILRewriteSystem *sys);

/* Print match and substitution */
void wilPrintMatch (const WILMatch *match);
void wilPrintSubstitution (const WILSubstitution *subst);

/* Enable/disable rewrite tracing for debugging */
void wilSetRewriteTrace (WILRewriteSystem *sys, int enable);

WIRBLE_END_DECLS

#endif /* WIRBLE_WRS_H */
