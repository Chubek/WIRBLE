#include "wrs_internal.h"

#include <stdio.h>

static WILNode *
wrs_apply_rule (WILRewriteSystem *sys, WILRewriteRule *rule, WILNode *node)
{
  WILMatch *match;
  WILNode *replacement;

  if (sys == NULL || rule == NULL || node == NULL || !rule->enabled)
    {
      return NULL;
    }

  match = wilPatternMatch (sys->ctx, rule->lhs, node);
  if (match == NULL || !wilMatchSucceeded (match))
    {
      wilMatchDestroy (sys->ctx, match);
      return NULL;
    }

  replacement = rule->action != NULL
                    ? rule->action (sys->ctx, match, rule->user_data)
                    : wilSubstitutionApply (sys->ctx,
                                            wilMatchGetSubstitution (match),
                                            rule->rhs);

  if (replacement != NULL)
    {
      wil_node_replace_all_uses (node, replacement);
      ++sys->stats.totalRewrites;
      if (rule->stats_index < 256u)
        {
          ++sys->stats.rulesApplied[rule->stats_index];
        }
      if (sys->tracing)
        {
          fprintf (stderr, "wrs: applied %s at n%lu -> n%lu\n", rule->name,
                   (unsigned long) wil_node_id (node),
                   (unsigned long) wil_node_id (replacement));
        }
    }

  wilMatchDestroy (sys->ctx, match);
  return replacement;
}

WILNode *
wilRewriteSystemApplyRule (WILRewriteSystem *sys, const char *ruleName,
                           WILNode *node)
{
  return wrs_apply_rule (sys, wilRewriteSystemFindRule (sys, ruleName), node);
}

WILNode *
wilRewriteSystemApplyAt (WILRewriteSystem *sys, WILNode *node)
{
  uint32_t index;
  WILNode *replacement;

  if (sys == NULL || node == NULL)
    {
      return NULL;
    }

  for (index = 0u; index < sys->rule_count; ++index)
    {
      replacement = wrs_apply_rule (sys, sys->rules[index], node);
      if (replacement != NULL)
        {
          return replacement;
        }
    }

  return NULL;
}

static WILNode *
wrs_apply_recursive (WILRewriteSystem *sys, WILNode *node)
{
  WILIndex index;
  WILNode *replacement;

  if (node == NULL)
    {
      return NULL;
    }

  if (sys->strategy == WIL_STRATEGY_INNERMOST)
    {
      for (index = 0u; index < wil_node_input_count (node); ++index)
        {
          WILNode *child = wil_node_input (node, index);
          replacement = wrs_apply_recursive (sys, child);
          if (replacement != NULL)
            {
              wil_node_replace_input (node, index, replacement);
            }
        }
    }

  replacement = wilRewriteSystemApplyAt (sys, node);
  if (replacement != NULL)
    {
      return replacement;
    }

  if (sys->strategy != WIL_STRATEGY_INNERMOST)
    {
      for (index = 0u; index < wil_node_input_count (node); ++index)
        {
          WILNode *child = wil_node_input (node, index);
          replacement = wrs_apply_recursive (sys, child);
          if (replacement != NULL)
            {
              wil_node_replace_input (node, index, replacement);
            }
        }
    }

  return NULL;
}

void
wilRewriteSystemApply (WILRewriteSystem *sys, WILNode *root)
{
  (void) wrs_apply_recursive (sys, root);
}

void
wilRewriteSystemApplyToFixpoint (WILRewriteSystem *sys, WILNode *root,
                                 uint32_t maxIterations)
{
  uint32_t iteration;
  WILNode *current_root;
  WILNode *replacement;

  if (sys == NULL)
    {
      return;
    }

  current_root = root;
  for (iteration = 0u; iteration < maxIterations; ++iteration)
    {
      ++sys->stats.iterations;
      replacement = wrs_apply_recursive (sys, current_root);
      if (replacement != NULL)
        {
          current_root = replacement;
        }
      if (replacement == NULL)
        {
          ++sys->stats.normalFormsReached;
          break;
        }
    }
}

int
wilRewriteSystemIsNormalForm (const WILRewriteSystem *sys, WILNode *node)
{
  uint32_t index;
  WILMatch *match;

  if (sys == NULL || node == NULL)
    {
      return 1;
    }

  for (index = 0u; index < sys->rule_count; ++index)
    {
      if (!sys->rules[index]->enabled)
        {
          continue;
        }
      match = wilPatternMatch (sys->ctx, sys->rules[index]->lhs, node);
      if (match != NULL && wilMatchSucceeded (match))
        {
          wilMatchDestroy (sys->ctx, match);
          return 0;
        }
      wilMatchDestroy (sys->ctx, match);
    }
  return 1;
}

int
wilRewriteSystemIsGraphNormalForm (const WILRewriteSystem *sys, WILNode *root)
{
  WILIndex index;

  if (root == NULL)
    {
      return 1;
    }
  if (!wilRewriteSystemIsNormalForm (sys, root))
    {
      return 0;
    }
  for (index = 0u; index < wil_node_input_count (root); ++index)
    {
      if (!wilRewriteSystemIsGraphNormalForm (sys, wil_node_input (root, index)))
        {
          return 0;
        }
    }
  return 1;
}

WILNode *
wilRewriteSystemNormalize (WILRewriteSystem *sys, WILNode *node,
                           uint32_t maxIterations)
{
  uint32_t iteration;
  WILNode *current;
  WILNode *replacement;

  current = node;
  for (iteration = 0u; iteration < maxIterations; ++iteration)
    {
      replacement = wrs_apply_recursive (sys, current);
      if (replacement != NULL)
        {
          current = replacement;
        }
      else
        {
          break;
        }
    }
  return current;
}
