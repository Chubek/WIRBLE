#include "wrs_internal.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef WIRBLE_STDRULE_DIR
#define WIRBLE_STDRULE_DIR "stdrewrite"
#endif

static char *
wrs_strdup (const char *text)
{
  size_t size;
  char *copy;

  if (text == NULL)
    {
      return NULL;
    }

  size = strlen (text) + 1u;
  copy = (char *) malloc (size);
  if (copy != NULL)
    {
      memcpy (copy, text, size);
    }
  return copy;
}

WILPattern *
wrs_pattern_alloc (WILContext *ctx)
{
  return (WILPattern *) wil_context_alloc (ctx, sizeof (WILPattern),
                                           _Alignof (WILPattern));
}

WILRewriteRule *
wrs_rule_alloc (WILContext *ctx)
{
  return (WILRewriteRule *) wil_context_alloc (ctx, sizeof (WILRewriteRule),
                                               _Alignof (WILRewriteRule));
}

int
wrs_append_rule (WILRewriteRule ***rules, uint32_t *count, uint32_t *capacity,
                 WILRewriteRule *rule)
{
  WILRewriteRule **grown;
  uint32_t new_capacity;

  if (rules == NULL || count == NULL || capacity == NULL || rule == NULL)
    {
      return 0;
    }

  if (*count == *capacity)
    {
      new_capacity = *capacity == 0u ? 8u : (*capacity * 2u);
      grown = (WILRewriteRule **) realloc (*rules,
                                           (size_t) new_capacity
                                               * sizeof (**rules));
      if (grown == NULL)
        {
          return 0;
        }
      *rules = grown;
      *capacity = new_capacity;
    }

  (*rules)[(*count)++] = rule;
  return 1;
}

void
wrs_append_text (WirbleByteBuf *buffer, const char *text)
{
  if (buffer != NULL && text != NULL)
    {
      (void) wirble_bytebuf_append_cstr (buffer, text);
    }
}

void
wrs_append_format (WirbleByteBuf *buffer, const char *format, ...)
{
  va_list args;
  char local[256];
  int written;

  if (buffer == NULL || format == NULL)
    {
      return;
    }

  va_start (args, format);
  written = vsnprintf (local, sizeof (local), format, args);
  va_end (args);
  if (written <= 0)
    {
      return;
    }

  if ((size_t) written >= sizeof (local))
    {
      local[sizeof (local) - 1u] = '\0';
    }
  (void) wirble_bytebuf_append_cstr (buffer, local);
}

const char *
wrs_node_kind_symbol (WILNodeKind kind)
{
  return wil_node_kind_str (kind);
}

WILNodeKind
wrs_parse_node_kind_symbol (const char *symbol)
{
  WILNodeKind kind;

  if (symbol == NULL)
    {
      return (WILNodeKind) -1;
    }

  for (kind = WIL_NODE_CONST_INT; kind <= WIL_NODE_SELECT; ++kind)
    {
      if (strcmp (wil_node_kind_str (kind), symbol) == 0)
        {
          return kind;
        }
    }

  return (WILNodeKind) -1;
}

WILPattern *
wilPatVar (WILContext *ctx, const char *name)
{
  WILPattern *pat = wrs_pattern_alloc (ctx);
  if (pat != NULL)
    {
      pat->kind = WIL_PAT_VAR;
      pat->name = wil_context_strdup (ctx, name);
    }
  return pat;
}

WILPattern *
wilPatConst (WILContext *ctx, int64_t value)
{
  WILPattern *pat = wrs_pattern_alloc (ctx);
  if (pat != NULL)
    {
      pat->kind = WIL_PAT_CONST;
      pat->value_kind = WIL_PATTERN_VALUE_INT;
      pat->int_value = value;
    }
  return pat;
}

WILPattern *
wilPatConstF64 (WILContext *ctx, double value)
{
  WILPattern *pat = wrs_pattern_alloc (ctx);
  if (pat != NULL)
    {
      pat->kind = WIL_PAT_CONST;
      pat->value_kind = WIL_PATTERN_VALUE_FLOAT;
      pat->float_value = value;
    }
  return pat;
}

WILPattern *
wilPatNode (WILContext *ctx, WILNodeKind kind, WILPattern **inputs,
            uint32_t inputCount)
{
  WILPattern *pat;
  uint32_t index;

  pat = wrs_pattern_alloc (ctx);
  if (pat == NULL)
    {
      return NULL;
    }

  pat->kind = WIL_PAT_NODE;
  pat->node_kind = kind;
  pat->input_count = inputCount;
  if (inputCount != 0u)
    {
      pat->inputs = (WILPattern **) wil_context_alloc (
          ctx, (size_t) inputCount * sizeof (*pat->inputs),
          _Alignof (WILPattern *));
      if (pat->inputs == NULL)
        {
          return NULL;
        }
      for (index = 0u; index < inputCount; ++index)
        {
          pat->inputs[index] = inputs[index];
        }
    }
  return pat;
}

WILPattern *
wilPatWildcard (WILContext *ctx)
{
  WILPattern *pat = wrs_pattern_alloc (ctx);
  if (pat != NULL)
    {
      pat->kind = WIL_PAT_WILDCARD;
    }
  return pat;
}

WILPattern *
wilPatPredicate (WILContext *ctx, WILPatternPredicate pred, void *userData)
{
  WILPattern *pat = wrs_pattern_alloc (ctx);
  if (pat != NULL)
    {
      pat->kind = WIL_PAT_PREDICATE;
      pat->predicate = pred;
      pat->predicate_user_data = userData;
    }
  return pat;
}

WILPattern *
wilPatSequence (WILContext *ctx, const char *name, WILPattern *elementPattern)
{
  WILPattern *pat = wrs_pattern_alloc (ctx);
  if (pat != NULL)
    {
      pat->kind = WIL_PAT_SEQUENCE;
      pat->name = wil_context_strdup (ctx, name);
      pat->element_pattern = elementPattern;
    }
  return pat;
}

WILPattern *
wilPatRefinement (WILContext *ctx, WILPattern *base)
{
  WILPattern *pat = wrs_pattern_alloc (ctx);
  if (pat != NULL)
    {
      pat->kind = WIL_PAT_REFINEMENT;
      pat->base_pattern = base;
    }
  return pat;
}

WILPatternKind wilPatternGetKind (const WILPattern *pat)
{ return pat == NULL ? WIL_PAT_WILDCARD : pat->kind; }
const char *wilPatternGetVarName (const WILPattern *pat)
{ return pat == NULL ? NULL : pat->name; }
WILNodeKind wilPatternGetNodeKind (const WILPattern *pat)
{ return pat == NULL ? (WILNodeKind) -1 : pat->node_kind; }
uint32_t wilPatternInputCount (const WILPattern *pat)
{ return pat == NULL ? 0u : pat->input_count; }
WILPattern *wilPatternGetInput (const WILPattern *pat, uint32_t index)
{ return (pat == NULL || index >= pat->input_count) ? NULL : pat->inputs[index]; }
void wilPatternDestroy (WILContext *ctx, WILPattern *pat)
{ (void) ctx; (void) pat; }

WILRewriteRule *
wilRuleCreate (WILContext *ctx, const char *name, WILPattern *lhs,
               WILRewriteAction action, void *userData)
{
  WILRewriteRule *rule = wrs_rule_alloc (ctx);
  if (rule != NULL)
    {
      rule->name = wil_context_strdup (ctx, name);
      rule->lhs = lhs;
      rule->action = action;
      rule->user_data = userData;
      rule->enabled = 1;
      rule->stats_index = 0u;
    }
  return rule;
}

WILRewriteRule *
wilRuleCreateSimple (WILContext *ctx, const char *name, WILPattern *lhs,
                     WILPattern *rhs)
{
  WILRewriteRule *rule = wilRuleCreate (ctx, name, lhs, NULL, NULL);
  if (rule != NULL)
    {
      rule->rhs = rhs;
    }
  return rule;
}

const char *wilRuleGetName (const WILRewriteRule *rule)
{ return rule == NULL ? NULL : rule->name; }
WILPattern *wilRuleGetLHS (const WILRewriteRule *rule)
{ return rule == NULL ? NULL : rule->lhs; }
WILRewriteAction wilRuleGetAction (const WILRewriteRule *rule)
{ return rule == NULL ? NULL : rule->action; }

static int
wrs_rule_collect_vars (const WILPattern *pat, const char **vars, uint32_t *count,
                       uint32_t capacity)
{
  uint32_t index;

  if (pat == NULL)
    {
      return 1;
    }

  if (pat->kind == WIL_PAT_VAR || pat->kind == WIL_PAT_SEQUENCE)
    {
      for (index = 0u; index < *count; ++index)
        {
          if (strcmp (vars[index], pat->name) == 0)
            {
              return 1;
            }
        }
      if (*count >= capacity)
        {
          return 0;
        }
      vars[(*count)++] = pat->name;
      return 1;
    }

  for (index = 0u; index < pat->input_count; ++index)
    {
      if (!wrs_rule_collect_vars (pat->inputs[index], vars, count, capacity))
        {
          return 0;
        }
    }

  if (pat->element_pattern != NULL)
    {
      return wrs_rule_collect_vars (pat->element_pattern, vars, count, capacity);
    }
  if (pat->base_pattern != NULL)
    {
      return wrs_rule_collect_vars (pat->base_pattern, vars, count, capacity);
    }
  return 1;
}

int
wilRuleValidate (const WILRewriteRule *rule)
{
  const char *lhs_vars[128];
  const char *rhs_vars[128];
  uint32_t lhs_count;
  uint32_t rhs_count;
  uint32_t rhs_index;
  uint32_t lhs_index;
  int found;

  if (rule == NULL || rule->lhs == NULL)
    {
      return 0;
    }
  if (rule->lhs->kind == WIL_PAT_VAR)
    {
      return 0;
    }
  if (rule->rhs == NULL && rule->action == NULL)
    {
      return 0;
    }
  if (rule->rhs == NULL)
    {
      return 1;
    }

  lhs_count = 0u;
  rhs_count = 0u;
  if (!wrs_rule_collect_vars (rule->lhs, lhs_vars, &lhs_count, 128u)
      || !wrs_rule_collect_vars (rule->rhs, rhs_vars, &rhs_count, 128u))
    {
      return 0;
    }

  for (rhs_index = 0u; rhs_index < rhs_count; ++rhs_index)
    {
      found = 0;
      for (lhs_index = 0u; lhs_index < lhs_count; ++lhs_index)
        {
          if (strcmp (rhs_vars[rhs_index], lhs_vars[lhs_index]) == 0)
            {
              found = 1;
              break;
            }
        }
      if (!found)
        {
          return 0;
        }
    }

  return 1;
}

void wilRuleDestroy (WILContext *ctx, WILRewriteRule *rule)
{ (void) ctx; (void) rule; }

WILRewriteSystem *
wilRewriteSystemCreate (WILContext *ctx)
{
  WILRewriteSystem *sys;

  sys = (WILRewriteSystem *) wil_context_alloc (ctx, sizeof (*sys),
                                                _Alignof (WILRewriteSystem));
  if (sys != NULL)
    {
      memset (sys, 0, sizeof (*sys));
      sys->ctx = ctx;
      sys->strategy = WIL_STRATEGY_INNERMOST;
    }
  return sys;
}

void
wilRewriteSystemDestroy (WILContext *ctx, WILRewriteSystem *sys)
{
  (void) ctx;
  if (sys != NULL)
    {
      free (sys->rules);
      sys->rules = NULL;
      sys->rule_count = 0u;
      sys->rule_capacity = 0u;
    }
}

void
wilRewriteSystemAddRule (WILRewriteSystem *sys, WILRewriteRule *rule)
{
  if (sys == NULL || rule == NULL)
    {
      return;
    }

  rule->stats_index = sys->rule_count < 256u ? sys->rule_count : 255u;
  (void) wrs_append_rule (&sys->rules, &sys->rule_count, &sys->rule_capacity,
                          rule);
}

void
wilRewriteSystemRemoveRule (WILRewriteSystem *sys, const char *ruleName)
{
  uint32_t index;

  if (sys == NULL || ruleName == NULL)
    {
      return;
    }

  for (index = 0u; index < sys->rule_count; ++index)
    {
      if (sys->rules[index] != NULL
          && strcmp (sys->rules[index]->name, ruleName) == 0)
        {
          memmove (&sys->rules[index], &sys->rules[index + 1u],
                   (size_t) (sys->rule_count - index - 1u)
                       * sizeof (*sys->rules));
          --sys->rule_count;
          return;
        }
    }
}

void wilRewriteSystemClearRules (WILRewriteSystem *sys)
{ if (sys != NULL) sys->rule_count = 0u; }
uint32_t wilRewriteSystemRuleCount (const WILRewriteSystem *sys)
{ return sys == NULL ? 0u : sys->rule_count; }
WILRewriteRule *wilRewriteSystemGetRule (const WILRewriteSystem *sys, uint32_t index)
{ return (sys == NULL || index >= sys->rule_count) ? NULL : sys->rules[index]; }
WILRewriteRule *wilRewriteSystemFindRule (const WILRewriteSystem *sys, const char *name)
{
  uint32_t index;
  if (sys == NULL || name == NULL)
    {
      return NULL;
    }
  for (index = 0u; index < sys->rule_count; ++index)
    {
      if (sys->rules[index] != NULL && strcmp (sys->rules[index]->name, name) == 0)
        {
          return sys->rules[index];
        }
    }
  return NULL;
}

void wilRewriteSystemEnableRule (WILRewriteSystem *sys, const char *name)
{ WILRewriteRule *rule = wilRewriteSystemFindRule (sys, name); if (rule != NULL) rule->enabled = 1; }
void wilRewriteSystemDisableRule (WILRewriteSystem *sys, const char *name)
{ WILRewriteRule *rule = wilRewriteSystemFindRule (sys, name); if (rule != NULL) rule->enabled = 0; }
int wilRewriteSystemIsRuleEnabled (const WILRewriteSystem *sys, const char *name)
{ WILRewriteRule *rule = wilRewriteSystemFindRule (sys, name); return rule != NULL && rule->enabled; }

void wilRewriteSystemSetStrategy (WILRewriteSystem *sys, WILRewriteStrategy strategy)
{ if (sys != NULL) sys->strategy = strategy; }
void wilRewriteSystemSetCustomStrategy (WILRewriteSystem *sys, WILCustomStrategy fn,
                                        void *userData)
{ if (sys != NULL) { sys->custom_strategy = fn; sys->custom_strategy_user_data = userData; } }
int wilRewriteSystemCheckConfluence (const WILRewriteSystem *sys)
{ (void) sys; return 1; }
int wilRewriteSystemCheckTermination (const WILRewriteSystem *sys)
{ (void) sys; return 1; }
uint32_t wilRewriteSystemFindCriticalPairs (const WILRewriteSystem *sys,
                                            WILCriticalPair **outPairs)
{ (void) sys; if (outPairs != NULL) *outPairs = NULL; return 0u; }
void wilCriticalPairsFree (WILContext *ctx, WILCriticalPair *pairs, uint32_t count)
{ (void) ctx; (void) pairs; (void) count; }
void wilRewriteSystemSetTracing (WILRewriteSystem *sys, int enabled)
{ if (sys != NULL) sys->tracing = enabled != 0; }
const WILRewriteStats *wilRewriteSystemGetStats (const WILRewriteSystem *sys)
{ return sys == NULL ? NULL : &sys->stats; }
void wilRewriteSystemResetStats (WILRewriteSystem *sys)
{ if (sys != NULL) memset (&sys->stats, 0, sizeof (sys->stats)); }
void wilRewriteSystemDump (const WILRewriteSystem *sys, void *file)
{ (void) sys; (void) file; }

static void
wrs_add_rules_from_builtin_file (WILRewriteSystem *sys, const char *filename)
{
  WRSFile *file;
  char path[1024];

  if (sys == NULL || filename == NULL)
    {
      return;
    }

  if (snprintf (path, sizeof (path), "%s/%s", WIRBLE_STDRULE_DIR, filename)
      >= (int) sizeof (path))
    {
      return;
    }
  file = wrsFileLoad (sys->ctx, path);
  if (file == NULL)
    {
      file = wrsFileLoad (sys->ctx, filename);
    }
  if (file == NULL)
    {
      return;
    }
  wrsFileApplyToSystem (file, sys);
}

void wilRewriteSystemAddAlgebraicRules (WILRewriteSystem *sys)
{ wrs_add_rules_from_builtin_file (sys, "arithmetic.wrs"); }
void wilRewriteSystemAddConstantFoldingRules (WILRewriteSystem *sys)
{ (void) sys; }
void wilRewriteSystemAddStrengthReductionRules (WILRewriteSystem *sys)
{ wrs_add_rules_from_builtin_file (sys, "canonical.wrs"); }
void wilRewriteSystemAddIdentityRules (WILRewriteSystem *sys)
{ wrs_add_rules_from_builtin_file (sys, "logic.wrs"); }
void wilRewriteSystemAddAllBuiltinRules (WILRewriteSystem *sys)
{
  wilRewriteSystemAddAlgebraicRules (sys);
  wilRewriteSystemAddIdentityRules (sys);
  wilRewriteSystemAddStrengthReductionRules (sys);
}
