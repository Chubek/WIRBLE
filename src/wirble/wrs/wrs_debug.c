#include "wrs_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void wrs_pattern_to_buffer (WirbleByteBuf *buffer, const WILPattern *pat);

static char *
wrs_dup_buffer (WirbleByteBuf *buffer)
{
  char *out;
  (void) wirble_bytebuf_append_byte (buffer, 0u);
  out = (char *) buffer->data;
  return out;
}

static void
wrs_pattern_to_buffer (WirbleByteBuf *buffer, const WILPattern *pat)
{
  uint32_t index;
  char local[64];

  if (pat == NULL)
    {
      wrs_append_text (buffer, "<null>");
      return;
    }

  switch (pat->kind)
    {
    case WIL_PAT_VAR:
      wrs_append_text (buffer, "?");
      wrs_append_text (buffer, pat->name);
      break;
    case WIL_PAT_WILDCARD:
      wrs_append_text (buffer, "_");
      break;
    case WIL_PAT_CONST:
      switch (pat->value_kind)
        {
        case WIL_PATTERN_VALUE_INT:
          snprintf (local, sizeof (local), "%lld", (long long) pat->int_value);
          wrs_append_text (buffer, local);
          break;
        case WIL_PATTERN_VALUE_FLOAT:
          snprintf (local, sizeof (local), "%g", pat->float_value);
          wrs_append_text (buffer, local);
          break;
        case WIL_PATTERN_VALUE_BOOL:
          wrs_append_text (buffer, pat->bool_value ? "true" : "false");
          break;
        default:
          wrs_append_text (buffer, "<const>");
          break;
        }
      break;
    case WIL_PAT_NODE:
      wrs_append_text (buffer, "(");
      wrs_append_text (buffer, wrs_node_kind_symbol (pat->node_kind));
      for (index = 0u; index < pat->input_count; ++index)
        {
          wrs_append_text (buffer, " ");
          wrs_pattern_to_buffer (buffer, pat->inputs[index]);
        }
      wrs_append_text (buffer, ")");
      break;
    case WIL_PAT_SEQUENCE:
      wrs_append_text (buffer, "(seq ?");
      wrs_append_text (buffer, pat->name);
      wrs_append_text (buffer, " ");
      wrs_pattern_to_buffer (buffer, pat->element_pattern);
      wrs_append_text (buffer, ")");
      break;
    case WIL_PAT_REFINEMENT:
      wrs_append_text (buffer, "(refine ");
      wrs_pattern_to_buffer (buffer, pat->base_pattern);
      wrs_append_text (buffer, ")");
      break;
    case WIL_PAT_PREDICATE:
      wrs_append_text (buffer, "(pred)");
      break;
    }
}

char *
wilPatternToString (WILContext *ctx, const WILPattern *pat)
{
  WirbleByteBuf buffer;
  (void) ctx;
  wirble_bytebuf_init (&buffer);
  wrs_pattern_to_buffer (&buffer, pat);
  return wrs_dup_buffer (&buffer);
}

char *
wilRuleToString (WILContext *ctx, const WILRewriteRule *rule)
{
  WirbleByteBuf buffer;
  char *lhs;
  char *rhs;
  (void) ctx;

  if (rule == NULL)
    {
      return NULL;
    }

  lhs = wilPatternToString (ctx, rule->lhs);
  rhs = wilPatternToString (ctx, rule->rhs);
  wirble_bytebuf_init (&buffer);
  wrs_append_text (&buffer, "(rule :name ");
  wrs_append_text (&buffer, rule->name);
  wrs_append_text (&buffer, " :match ");
  wrs_append_text (&buffer, lhs);
  wrs_append_text (&buffer, " :replace ");
  wrs_append_text (&buffer, rhs);
  wrs_append_text (&buffer, ")");
  free (lhs);
  free (rhs);
  return wrs_dup_buffer (&buffer);
}

char *
wilRewriteSystemToString (WILContext *ctx, const WILRewriteSystem *sys)
{
  WirbleByteBuf buffer;
  uint32_t index;
  char *rule_text;
  (void) ctx;

  wirble_bytebuf_init (&buffer);
  for (index = 0u; sys != NULL && index < sys->rule_count; ++index)
    {
      rule_text = wilRuleToString (ctx, sys->rules[index]);
      wrs_append_text (&buffer, rule_text);
      wrs_append_text (&buffer, "\n");
      free (rule_text);
    }
  return wrs_dup_buffer (&buffer);
}

char *
wilSubstitutionToString (WILContext *ctx, const WILSubstitution *subst)
{
  WirbleByteBuf buffer;
  uint32_t index;
  char local[64];
  (void) ctx;

  wirble_bytebuf_init (&buffer);
  wrs_append_text (&buffer, "{");
  for (index = 0u; subst != NULL && index < subst->count; ++index)
    {
      if (index != 0u)
        {
          wrs_append_text (&buffer, ", ");
        }
      wrs_append_text (&buffer, subst->bindings[index].name);
      wrs_append_text (&buffer, "=n");
      snprintf (local, sizeof (local), "%lu",
                (unsigned long) wil_node_id (subst->bindings[index].node));
      wrs_append_text (&buffer, local);
    }
  wrs_append_text (&buffer, "}");
  return wrs_dup_buffer (&buffer);
}

char *
wilMatchToString (WILContext *ctx, const WILMatch *match)
{
  WirbleByteBuf buffer;
  char *subst_text;
  (void) ctx;

  wirble_bytebuf_init (&buffer);
  wrs_append_text (&buffer, match != NULL && match->succeeded ? "match " : "no-match ");
  subst_text = wilSubstitutionToString (ctx,
                                        match == NULL ? NULL : &match->substitution);
  wrs_append_text (&buffer, subst_text == NULL ? "{}" : subst_text);
  free (subst_text);
  return wrs_dup_buffer (&buffer);
}
