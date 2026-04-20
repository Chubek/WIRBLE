#include "wrs_internal.h"

#include <stdlib.h>
#include <string.h>

static WILMatch *
wrs_match_alloc (WILContext *ctx)
{
  WILMatch *match;

  match = (WILMatch *) wil_context_alloc (ctx, sizeof (*match),
                                          _Alignof (WILMatch));
  if (match != NULL)
    {
      memset (match, 0, sizeof (*match));
    }
  return match;
}

static int
wrs_substitution_bind (WILSubstitution *subst, const char *name, WILNode *node)
{
  uint32_t index;
  WILBinding *grown;
  uint32_t new_capacity;

  if (subst == NULL || name == NULL)
    {
      return 0;
    }

  for (index = 0u; index < subst->count; ++index)
    {
      if (strcmp (subst->bindings[index].name, name) == 0)
        {
          return subst->bindings[index].node == node;
        }
    }

  if (subst->count == subst->capacity)
    {
      new_capacity = subst->capacity == 0u ? 8u : subst->capacity * 2u;
      grown = (WILBinding *) realloc (subst->bindings,
                                      (size_t) new_capacity
                                          * sizeof (*subst->bindings));
      if (grown == NULL)
        {
          return 0;
        }
      subst->bindings = grown;
      subst->capacity = new_capacity;
    }

  subst->bindings[subst->count].name = name;
  subst->bindings[subst->count].node = node;
  ++subst->count;
  return 1;
}

static int
wrs_match_constant (const WILPattern *pat, WILNode *node)
{
  if (pat == NULL || node == NULL || !wil_node_is_const (node))
    {
      return 0;
    }

  switch (pat->value_kind)
    {
    case WIL_PATTERN_VALUE_INT:
      return wil_node_kind (node) == WIL_NODE_CONST_INT
             && wil_node_const_int (node) == pat->int_value;
    case WIL_PATTERN_VALUE_FLOAT:
      return wil_node_kind (node) == WIL_NODE_CONST_FLOAT
             && wil_node_const_float (node) == pat->float_value;
    case WIL_PATTERN_VALUE_BOOL:
      return wil_node_kind (node) == WIL_NODE_CONST_BOOL
             && wil_node_const_bool (node) == pat->bool_value;
    default:
      return 0;
    }
}

static int
wrs_match_pattern (WILSubstitution *subst, const WILPattern *pat, WILNode *node)
{
  uint32_t index;

  if (pat == NULL)
    {
      return 0;
    }

  switch (pat->kind)
    {
    case WIL_PAT_WILDCARD:
      return 1;
    case WIL_PAT_VAR:
      return wrs_substitution_bind (subst, pat->name, node);
    case WIL_PAT_SEQUENCE:
      return wrs_substitution_bind (subst, pat->name, node);
    case WIL_PAT_CONST:
      return wrs_match_constant (pat, node);
    case WIL_PAT_PREDICATE:
      return pat->predicate == NULL ? 0 : pat->predicate (node, pat->predicate_user_data);
    case WIL_PAT_REFINEMENT:
      return wrs_match_pattern (subst, pat->base_pattern, node);
    case WIL_PAT_NODE:
      if (node == NULL || wil_node_kind (node) != pat->node_kind)
        {
          return 0;
        }
      if (wil_node_input_count (node) != pat->input_count)
        {
          return 0;
        }
      for (index = 0u; index < pat->input_count; ++index)
        {
          if (!wrs_match_pattern (subst, pat->inputs[index],
                                  wil_node_input (node, index)))
            {
              return 0;
            }
        }
      return 1;
    }

  return 0;
}

WILMatch *
wilPatternMatch (WILContext *ctx, const WILPattern *pat, WILNode *node)
{
  WILMatch *match;

  match = wrs_match_alloc (ctx);
  if (match == NULL)
    {
      return NULL;
    }

  match->node = node;
  match->succeeded = wrs_match_pattern (&match->substitution, pat, node);
  return match;
}

int wilMatchSucceeded (const WILMatch *match)
{ return match != NULL && match->succeeded; }

void
wilMatchDestroy (WILContext *ctx, WILMatch *match)
{
  (void) ctx;
  if (match != NULL)
    {
      free (match->substitution.bindings);
      match->substitution.bindings = NULL;
      match->substitution.count = 0u;
      match->substitution.capacity = 0u;
    }
}

WILSubstitution *wilMatchGetSubstitution (const WILMatch *match)
{ return match == NULL ? NULL : (WILSubstitution *) &match->substitution; }
WILNode *wilSubstitutionLookup (const WILSubstitution *subst, const char *varName)
{
  uint32_t index;
  if (subst == NULL || varName == NULL)
    {
      return NULL;
    }
  for (index = 0u; index < subst->count; ++index)
    {
      if (strcmp (subst->bindings[index].name, varName) == 0)
        {
          return subst->bindings[index].node;
        }
    }
  return NULL;
}
int wilSubstitutionHas (const WILSubstitution *subst, const char *varName)
{ return wilSubstitutionLookup (subst, varName) != NULL; }
uint32_t wilSubstitutionVarCount (const WILSubstitution *subst)
{ return subst == NULL ? 0u : subst->count; }
const char *wilSubstitutionVarName (const WILSubstitution *subst, uint32_t index)
{ return (subst == NULL || index >= subst->count) ? NULL : subst->bindings[index].name; }

static WILNode *
wrs_apply_pattern (WILContext *ctx, const WILSubstitution *subst,
                   const WILPattern *pat)
{
  WILNode **inputs;
  uint32_t index;

  if (pat == NULL)
    {
      return NULL;
    }

  switch (pat->kind)
    {
    case WIL_PAT_VAR:
    case WIL_PAT_SEQUENCE:
      return wilSubstitutionLookup (subst, pat->name);
    case WIL_PAT_WILDCARD:
      return NULL;
    case WIL_PAT_CONST:
      switch (pat->value_kind)
        {
        case WIL_PATTERN_VALUE_INT:
          return wil_make_const_int (ctx, pat->int_value, WIL_TYPE_I32);
        case WIL_PATTERN_VALUE_FLOAT:
          return wil_make_const_float (ctx, pat->float_value, WIL_TYPE_F64);
        case WIL_PATTERN_VALUE_BOOL:
          return wil_make_const_bool (ctx, pat->bool_value);
        default:
          return NULL;
        }
    case WIL_PAT_REFINEMENT:
      return wrs_apply_pattern (ctx, subst, pat->base_pattern);
    case WIL_PAT_NODE:
    {
      WILNode *result;
      inputs = NULL;
      if (pat->input_count != 0u)
        {
          inputs = (WILNode **) malloc ((size_t) pat->input_count * sizeof (*inputs));
          if (inputs == NULL)
            {
              return NULL;
            }
          for (index = 0u; index < pat->input_count; ++index)
            {
              inputs[index] = wrs_apply_pattern (ctx, subst, pat->inputs[index]);
            }
        }
      switch (pat->node_kind)
        {
        case WIL_NODE_ADD: result = wil_make_add (ctx, inputs[0], inputs[1]); break;
        case WIL_NODE_SUB: result = wil_make_sub (ctx, inputs[0], inputs[1]); break;
        case WIL_NODE_MUL: result = wil_make_mul (ctx, inputs[0], inputs[1]); break;
        case WIL_NODE_DIV: result = wil_make_div (ctx, inputs[0], inputs[1]); break;
        case WIL_NODE_MOD: result = wil_make_mod (ctx, inputs[0], inputs[1]); break;
        case WIL_NODE_AND: result = wil_make_and (ctx, inputs[0], inputs[1]); break;
        case WIL_NODE_OR: result = wil_make_or (ctx, inputs[0], inputs[1]); break;
        case WIL_NODE_XOR: result = wil_make_xor (ctx, inputs[0], inputs[1]); break;
        case WIL_NODE_SHL: result = wil_make_shl (ctx, inputs[0], inputs[1]); break;
        case WIL_NODE_SHR: result = wil_make_shr (ctx, inputs[0], inputs[1]); break;
        case WIL_NODE_NEG: result = wil_make_neg (ctx, inputs[0]); break;
        case WIL_NODE_NOT: result = wil_make_not (ctx, inputs[0]); break;
        case WIL_NODE_ABS: result = wil_make_abs (ctx, inputs[0]); break;
        case WIL_NODE_EQ: result = wil_make_eq (ctx, inputs[0], inputs[1]); break;
        case WIL_NODE_NE: result = wil_make_ne (ctx, inputs[0], inputs[1]); break;
        case WIL_NODE_LT: result = wil_make_lt (ctx, inputs[0], inputs[1]); break;
        case WIL_NODE_LE: result = wil_make_le (ctx, inputs[0], inputs[1]); break;
        case WIL_NODE_GT: result = wil_make_gt (ctx, inputs[0], inputs[1]); break;
        case WIL_NODE_GE: result = wil_make_ge (ctx, inputs[0], inputs[1]); break;
        case WIL_NODE_SELECT:
          result = wil_make_select (ctx, inputs[0], inputs[1], inputs[2]);
          break;
        default:
          result = NULL;
          break;
        }
      free (inputs);
      return result;
    }
    default:
      return NULL;
    }
}

WILNode *
wilSubstitutionApply (WILContext *ctx, const WILSubstitution *subst,
                      const WILPattern *pat)
{
  return wrs_apply_pattern (ctx, subst, pat);
}
