#include "wrs_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *
wrs_strdup_local (const char *text)
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

static WILPattern *wrs_pattern_from_sexp_node (WILContext *ctx,
                                               const WirbleSexpNode *node);

static int
wrs_atom_is (const WirbleSexpNode *node, const char *text)
{
  return wirble_sexp_node_is_atom (node) && node->text != NULL
         && strcmp (node->text, text) == 0;
}

static int
wrs_parse_int64 (const char *text, int64_t *value)
{
  char *endptr;
  long long parsed;

  if (text == NULL || value == NULL)
    {
      return 0;
    }

  parsed = strtoll (text, &endptr, 10);
  if (endptr == text || *endptr != '\0')
    {
      return 0;
    }
  *value = (int64_t) parsed;
  return 1;
}

static WILPattern *
wrs_const_pattern_from_atom (WILContext *ctx, const WirbleSexpNode *node)
{
  int64_t int_value;

  if (node == NULL || node->text == NULL)
    {
      return NULL;
    }

  if (strcmp (node->text, "true") == 0 || strcmp (node->text, "false") == 0)
    {
      WILPattern *pat = wrs_pattern_alloc (ctx);
      if (pat != NULL)
        {
          pat->kind = WIL_PAT_CONST;
          pat->value_kind = WIL_PATTERN_VALUE_BOOL;
          pat->bool_value = strcmp (node->text, "true") == 0;
        }
      return pat;
    }

  if (wrs_parse_int64 (node->text, &int_value))
    {
      return wilPatConst (ctx, int_value);
    }

  return NULL;
}

static WILPattern *
wrs_pattern_from_list (WILContext *ctx, const WirbleSexpNode *node)
{
  const WirbleSexpNode *head;
  WILPattern **inputs;
  WILNodeKind kind;
  uint32_t count;
  uint32_t index;

  if (!wirble_sexp_node_is_list (node))
    {
      return NULL;
    }

  head = wirble_sexp_node_child (node, 0u);
  if (wrs_atom_is (head, "seq"))
    {
      const WirbleSexpNode *var = wirble_sexp_node_child (node, 1u);
      const WirbleSexpNode *element = wirble_sexp_node_child (node, 2u);
      const char *var_name = var == NULL ? NULL : var->text;
      if (var_name != NULL && var_name[0] == '?')
        {
          ++var_name;
        }
      return wilPatSequence (ctx, var_name,
                             wrs_pattern_from_sexp_node (ctx, element));
    }
  if (wrs_atom_is (head, "refine"))
    {
      return wilPatRefinement (ctx,
                               wrs_pattern_from_sexp_node (
                                   ctx, wirble_sexp_node_child (node, 1u)));
    }
  if (wrs_atom_is (head, "pred"))
    {
      return wilPatWildcard (ctx);
    }

  kind = wrs_parse_node_kind_symbol (head == NULL ? NULL : head->text);
  if ((int) kind < 0)
    {
      return NULL;
    }

  count = (uint32_t) wirble_sexp_node_child_count (node);
  if (count <= 1u)
    {
      return wilPatNode (ctx, kind, NULL, 0u);
    }

  inputs = (WILPattern **) wil_context_alloc (
      ctx, (size_t) (count - 1u) * sizeof (*inputs), _Alignof (WILPattern *));
  if (inputs == NULL)
    {
      return NULL;
    }

  for (index = 1u; index < count; ++index)
    {
      inputs[index - 1u]
          = wrs_pattern_from_sexp_node (ctx, wirble_sexp_node_child (node, index));
    }

  return wilPatNode (ctx, kind, inputs, count - 1u);
}

static WILPattern *
wrs_pattern_from_sexp_node (WILContext *ctx, const WirbleSexpNode *node)
{
  WILPattern *constant;
  WILNodeKind kind;

  if (node == NULL)
    {
      return NULL;
    }

  if (wirble_sexp_node_is_atom (node))
    {
      if (strcmp (node->text, "_") == 0)
        {
          return wilPatWildcard (ctx);
        }
      if (node->text[0] == '?')
        {
          return wilPatVar (ctx, node->text + 1u);
        }
      constant = wrs_const_pattern_from_atom (ctx, node);
      if (constant != NULL)
        {
          return constant;
        }
      kind = wrs_parse_node_kind_symbol (node->text);
      if ((int) kind >= 0)
        {
          return wilPatNode (ctx, kind, NULL, 0u);
        }
      return NULL;
    }

  return wrs_pattern_from_list (ctx, node);
}

WILPattern *
wrsPatternFromSExpr (WILContext *ctx, const WRSSExpr *expr)
{
  (void) ctx;
  (void) expr;
  return NULL;
}

static WILRewriteStrategy
wrs_parse_strategy_atom (const char *text)
{
  if (text == NULL)
    {
      return WIL_STRATEGY_INNERMOST;
    }
  if (strcmp (text, "outermost") == 0)
    {
      return WIL_STRATEGY_OUTERMOST;
    }
  if (strcmp (text, "leftmost") == 0)
    {
      return WIL_STRATEGY_LEFTMOST;
    }
  if (strcmp (text, "parallel") == 0)
    {
      return WIL_STRATEGY_PARALLEL;
    }
  return WIL_STRATEGY_INNERMOST;
}

WILRewriteStrategy
wrsStrategyFromSExpr (const WRSSExpr *expr)
{
  if (expr == NULL || expr->kind != WRS_SEXPR_ATOM)
    {
      return WIL_STRATEGY_INNERMOST;
    }
  return wrs_parse_strategy_atom (expr->atom);
}

static WILRewriteRule *
wrs_rule_from_tree (WILContext *ctx, const WirbleSexpNode *form)
{
  WirbleSexpRuleView view;
  WILPattern *lhs;
  WILPattern *rhs;
  WILRewriteRule *rule;

  if (!wirble_sexp_extract_rule (form, &view))
    {
      return NULL;
    }

  lhs = wrs_pattern_from_sexp_node (ctx, view.match);
  rhs = wrs_pattern_from_sexp_node (ctx, view.replace);
  rule = wilRuleCreateSimple (ctx,
                              view.name == NULL ? NULL : view.name->text, lhs,
                              rhs);
  if (rule != NULL && view.condition != NULL)
    {
      rule->guard = wrs_sexpr_from_tree (ctx, view.condition);
    }
  return rule;
}

WILRewriteRule *
wrsRuleFromSExpr (WILContext *ctx, const WRSSExpr *expr)
{
  (void) ctx;
  (void) expr;
  return NULL;
}

static WRSFile *
wrs_file_alloc (WILContext *ctx)
{
  WRSFile *file;

  file = (WRSFile *) wil_context_alloc (ctx, sizeof (*file), _Alignof (WRSFile));
  if (file != NULL)
    {
      memset (file, 0, sizeof (*file));
      file->ctx = ctx;
      file->strategy = WIL_STRATEGY_INNERMOST;
    }
  return file;
}

static WRSFile *
wrs_file_load_tree (WILContext *ctx, WirbleSexpTree *tree)
{
  const WirbleSexpNode *root;
  const WirbleSexpNode *form;
  WRSFile *file;

  file = wrs_file_alloc (ctx);
  if (file == NULL)
    {
      return NULL;
    }

  root = wirble_sexp_tree_root (tree);
  for (form = wirble_sexp_node_child (root, 0u); form != NULL;
       form = wirble_sexp_node_next (form))
    {
      if (wirble_sexp_node_is_list (form)
          && wrs_atom_is (wirble_sexp_node_child (form, 0u), "rule"))
        {
          WILRewriteRule *rule = wrs_rule_from_tree (ctx, form);
          if (rule != NULL)
            {
              (void) wrs_append_rule (&file->rules, &file->rule_count,
                                      &file->rule_capacity, rule);
            }
        }
      else if (wirble_sexp_node_is_list (form)
               && wrs_atom_is (wirble_sexp_node_child (form, 0u), "strategy"))
        {
          const WirbleSexpNode *atom = wirble_sexp_node_child (form, 1u);
          file->strategy = wrs_parse_strategy_atom (atom == NULL ? NULL : atom->text);
        }
    }

  return file;
}

WRSFile *
wrsFileLoad (WILContext *ctx, const char *path)
{
  WirbleDiagnostics diag;
  WirbleSexpTree tree;
  WRSFile *file;

  wirble_diag_init (&diag, stderr, "wrs");
  wirble_sexp_tree_init (&tree, &diag);
  if (!wirble_sexp_tree_parse_file (&tree, path))
    {
      wirble_sexp_tree_destroy (&tree);
      return NULL;
    }

  file = wrs_file_load_tree (ctx, &tree);
  wirble_sexp_tree_destroy (&tree);
  return file;
}

WRSFile *
wrsFileLoadFromString (WILContext *ctx, const char *source)
{
  WirbleDiagnostics diag;
  WirbleSexpTree tree;
  WRSFile *file;

  wirble_diag_init (&diag, stderr, "wrs");
  wirble_sexp_tree_init (&tree, &diag);
  if (!wirble_sexp_tree_parse_cstr (&tree, "<string>", source))
    {
      wirble_sexp_tree_destroy (&tree);
      return NULL;
    }
  file = wrs_file_load_tree (ctx, &tree);
  wirble_sexp_tree_destroy (&tree);
  return file;
}

uint32_t wrsFileRuleCount (const WRSFile *file)
{ return file == NULL ? 0u : file->rule_count; }
WILRewriteRule *wrsFileGetRule (const WRSFile *file, uint32_t index)
{ return (file == NULL || index >= file->rule_count) ? NULL : file->rules[index]; }
WILRewriteStrategy wrsFileGetStrategy (const WRSFile *file)
{ return file == NULL ? WIL_STRATEGY_INNERMOST : file->strategy; }
void wrsFileApplyToSystem (const WRSFile *file, WILRewriteSystem *sys)
{
  uint32_t index;
  if (file == NULL || sys == NULL)
    {
      return;
    }
  wilRewriteSystemSetStrategy (sys, file->strategy);
  for (index = 0u; index < file->rule_count; ++index)
    {
      wilRewriteSystemAddRule (sys, file->rules[index]);
    }
}
void wrsFileDestroy (WILContext *ctx, WRSFile *file)
{ (void) ctx; if (file != NULL) free (file->rules); }
int wrsFileValidate (WILContext *ctx, const char *path, char **errorMsg)
{
  WRSFile *file = wrsFileLoad (ctx, path);
  uint32_t index;
  if (file == NULL)
    {
      if (errorMsg != NULL)
        {
          *errorMsg = wrs_strdup_local ("failed to load .wrs file");
        }
      return 0;
    }
  for (index = 0u; index < file->rule_count; ++index)
    {
      if (!wilRuleValidate (file->rules[index]))
        {
          if (errorMsg != NULL)
            {
              *errorMsg = wrs_strdup_local ("invalid rewrite rule");
            }
          wrsFileDestroy (ctx, file);
          return 0;
        }
    }
  wrsFileDestroy (ctx, file);
  return 1;
}

int wrsFileWrite (WILContext *ctx, const WILRewriteSystem *sys, const char *path)
{ (void) ctx; (void) sys; (void) path; return 0; }
char *wrsFileToString (WILContext *ctx, const WILRewriteSystem *sys)
{ (void) ctx; (void) sys; return NULL; }
WRSSExpr *wrsRuleToSExpr (WILContext *ctx, const WILRewriteRule *rule)
{ (void) ctx; (void) rule; return NULL; }
WRSSExpr *wrsPatternToSExpr (WILContext *ctx, const WILPattern *pat)
{ (void) ctx; (void) pat; return NULL; }
