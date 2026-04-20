#include "mal_ir.h"

#include <stdlib.h>
#include <string.h>

#include "common/diagnostics.h"
#include "support/sexp_reader.h"

#ifndef WIRBLE_STDRULE_DIR
#define WIRBLE_STDRULE_DIR "stdrewrite"
#endif

static MALOpcode
mal_parse_opcode (const char *text)
{
  if (text == NULL)
    {
      return MAL_OP_TARGET;
    }
  if (strcmp (text, "copy") == 0) return MAL_OP_COPY;
  if (strcmp (text, "add") == 0) return MAL_OP_ADD;
  if (strcmp (text, "sub") == 0) return MAL_OP_SUB;
  if (strcmp (text, "mul") == 0) return MAL_OP_MUL;
  if (strcmp (text, "sdiv") == 0 || strcmp (text, "div") == 0) return MAL_OP_SDIV;
  if (strcmp (text, "and") == 0) return MAL_OP_AND;
  if (strcmp (text, "or") == 0) return MAL_OP_OR;
  if (strcmp (text, "xor") == 0) return MAL_OP_XOR;
  if (strcmp (text, "shl") == 0) return MAL_OP_SHL;
  if (strcmp (text, "ashr") == 0 || strcmp (text, "shr") == 0) return MAL_OP_ASHR;
  if (strcmp (text, "ret") == 0) return MAL_OP_RET;
  return MAL_OP_TARGET;
}

static int
mal_parse_int64 (const char *text, int64_t *value)
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

static int
mal_load_operand_pattern (const WirbleSexpNode *node, MALRuleOperandPattern *pattern)
{
  int64_t imm;

  memset (pattern, 0, sizeof (*pattern));
  if (node == NULL || !wirble_sexp_node_is_atom (node) || node->text == NULL)
    {
      return 0;
    }
  if (strcmp (node->text, "_") == 0)
    {
      pattern->kind = MAL_RULE_OPERAND_ANY;
      return 1;
    }
  if (node->text[0] == '?')
    {
      pattern->kind = MAL_RULE_OPERAND_VAR;
      pattern->name = mal_xstrdup (node->text + 1u);
      return pattern->name != NULL;
    }
  if (mal_parse_int64 (node->text, &imm))
    {
      pattern->kind = MAL_RULE_OPERAND_IMM_INT;
      pattern->operand = mal_operand_imm_int (imm, MAL_TYPE_I32);
      return 1;
    }
  return 0;
}

static int
mal_load_pattern_list (const WirbleSexpNode *list, MALOpcode *opcode,
                       MALRuleOperandPattern **patterns, uint32_t *count)
{
  const WirbleSexpNode *head;
  uint32_t i;

  if (list == NULL || !wirble_sexp_node_is_list (list))
    {
      return 0;
    }
  head = wirble_sexp_node_child (list, 0u);
  if (head == NULL || !wirble_sexp_node_is_atom (head))
    {
      return 0;
    }
  *opcode = mal_parse_opcode (head->text);
  if (*opcode == MAL_OP_TARGET)
    {
      return 0;
    }
  *count = (uint32_t) wirble_sexp_node_child_count (list);
  if (*count == 0u)
    {
      return 0;
    }
  --(*count);
  *patterns = *count == 0u ? NULL
                           : (MALRuleOperandPattern *) calloc (*count, sizeof (**patterns));
  if (*count != 0u && *patterns == NULL)
    {
      return 0;
    }
  for (i = 0u; i < *count; ++i)
    {
      if (!mal_load_operand_pattern (wirble_sexp_node_child (list, i + 1u), &(*patterns)[i]))
        {
          return 0;
        }
    }
  return 1;
}

static void
mal_rule_free_members (MALRewriteRule *rule)
{
  uint32_t i;
  if (rule == NULL)
    {
      return;
    }
  for (i = 0u; i < rule->matchOperandCount; ++i)
    {
      free ((char *) rule->matchOperands[i].name);
    }
  for (i = 0u; i < rule->replaceOperandCount; ++i)
    {
      free ((char *) rule->replaceOperands[i].name);
    }
  free (rule->matchOperands);
  free (rule->replaceOperands);
  free ((char *) rule->name);
}

static int
mal_system_add_rule (MALRewriteSystem *mrs, const MALRewriteRule *rule)
{
  MALRewriteRule *grown;
  grown = (MALRewriteRule *) realloc (mrs->rules,
                                      (size_t) (mrs->ruleCount + 1u) * sizeof (*mrs->rules));
  if (grown == NULL)
    {
      return 0;
    }
  mrs->rules = grown;
  mrs->rules[mrs->ruleCount++] = *rule;
  return 1;
}

static int
mal_load_rule (MALRewriteSystem *mrs, const WirbleSexpNode *form)
{
  WirbleSexpRuleView ruleView;
  MALRewriteRule rule;

  memset (&rule, 0, sizeof (rule));
  if (!wirble_sexp_extract_rule (form, &ruleView))
    {
      return 0;
    }
  rule.name = mal_xstrdup (ruleView.name == NULL ? "rule" : ruleView.name->text);
  if (rule.name == NULL
      || !mal_load_pattern_list (ruleView.match, &rule.matchOpcode, &rule.matchOperands,
                                 &rule.matchOperandCount)
      || !mal_load_pattern_list (ruleView.replace, &rule.replaceOpcode,
                                 &rule.replaceOperands, &rule.replaceOperandCount))
    {
      mal_rule_free_members (&rule);
      return 0;
    }
  if (!mal_system_add_rule (mrs, &rule))
    {
      mal_rule_free_members (&rule);
      return 0;
    }
  return 1;
}

static MALRewriteSystem *
mal_load_tree (WirbleSexpTree *tree)
{
  MALRewriteSystem *mrs;
  const WirbleSexpNode *root;
  const WirbleSexpNode *form;

  mrs = (MALRewriteSystem *) calloc (1u, sizeof (*mrs));
  if (mrs == NULL)
    {
      return NULL;
    }
  root = wirble_sexp_tree_root (tree);
  for (form = wirble_sexp_node_child (root, 0u); form != NULL;
       form = wirble_sexp_node_next (form))
    {
      if (wirble_sexp_node_is_list (form)
          && wirble_sexp_node_child (form, 0u) != NULL
          && wirble_sexp_atom_equals (wirble_sexp_node_child (form, 0u), "rule"))
        {
          if (!mal_load_rule (mrs, form))
            {
              mal_mrs_destroy (mrs);
              return NULL;
            }
        }
    }
  return mrs;
}

MALRewriteSystem *
mal_mrs_load (const char *path)
{
  WirbleDiagnostics diag;
  WirbleSexpTree tree;
  MALRewriteSystem *mrs;
  char localPath[1024];
  const char *requested = path == NULL ? "mal_peephole.wrs" : path;
  const char *slash;

  wirble_diag_init (&diag, stderr, "mrs");
  wirble_sexp_tree_init (&tree, &diag);
  slash = strrchr (requested, '/');
  if (slash == NULL
      && snprintf (localPath, sizeof (localPath), "%s/%s", WIRBLE_STDRULE_DIR,
                   requested)
             < (int) sizeof (localPath)
      && wirble_sexp_tree_parse_file (&tree, localPath))
    {
    }
  else if (!wirble_sexp_tree_parse_file (&tree, requested))
    {
      if (snprintf (localPath, sizeof (localPath), "%s/%s", WIRBLE_STDRULE_DIR,
                    requested)
          >= (int) sizeof (localPath)
          || !wirble_sexp_tree_parse_file (&tree, localPath))
        {
          wirble_sexp_tree_destroy (&tree);
          return NULL;
        }
    }
  mrs = mal_load_tree (&tree);
  wirble_sexp_tree_destroy (&tree);
  return mrs;
}

void
mal_mrs_destroy (MALRewriteSystem *mrs)
{
  uint32_t i;
  if (mrs == NULL)
    {
      return;
    }
  for (i = 0u; i < mrs->ruleCount; ++i)
    {
      mal_rule_free_members (&mrs->rules[i]);
    }
  free (mrs->rules);
  free (mrs);
}
