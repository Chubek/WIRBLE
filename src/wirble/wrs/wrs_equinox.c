#include "wrs_internal.h"

#include <equinox/eclass.h>
#include <equinox/egraph.h>
#include <equinox/rewrite.h>

#include <stdlib.h>
#include <string.h>

typedef struct WRSEGraphBuild
{
  WILNode **nodes;
  eqx_eclass_id_t *ids;
  uint32_t count;
  uint32_t capacity;
} WRSEGraphBuild;

typedef struct WRSEntry
{
  WILNode *node;
  double cost;
  int valid;
} WRSEntry;

static uint32_t
wrs_op_of_kind (WILNodeKind kind)
{
  return (uint32_t) kind + 1u;
}

static int
wrs_eqx_arity_supported (WILIndex arity)
{
  return arity <= 4u;
}

static int
wrs_is_eqx_rewritable_kind (WILNodeKind kind)
{
  switch (kind)
    {
    case WIL_NODE_ADD:
    case WIL_NODE_MUL:
    case WIL_NODE_AND:
    case WIL_NODE_OR:
    case WIL_NODE_XOR:
      return 1;
    default:
      return 0;
    }
}

static int
wrs_build_lookup (const WRSEGraphBuild *build, WILNode *node,
                  eqx_eclass_id_t *out_id)
{
  uint32_t index;
  if (build == NULL || node == NULL)
    {
      return 0;
    }
  for (index = 0u; index < build->count; ++index)
    {
      if (build->nodes[index] == node)
        {
          if (out_id != NULL)
            {
              *out_id = build->ids[index];
            }
          return 1;
        }
    }
  return 0;
}

static int
wrs_build_record (WRSEGraphBuild *build, WILNode *node, eqx_eclass_id_t id)
{
  WILNode **grown_nodes;
  eqx_eclass_id_t *grown_ids;
  uint32_t new_capacity;

  if (build == NULL || node == NULL)
    {
      return 0;
    }

  if (build->count == build->capacity)
    {
      new_capacity = build->capacity == 0u ? 16u : build->capacity * 2u;
      grown_nodes = (WILNode **) realloc (build->nodes,
                                          (size_t) new_capacity
                                              * sizeof (*grown_nodes));
      if (grown_nodes == NULL)
        {
          return 0;
        }
      build->nodes = grown_nodes;

      grown_ids = (eqx_eclass_id_t *) realloc (build->ids,
                                               (size_t) new_capacity
                                                   * sizeof (*grown_ids));
      if (grown_ids == NULL)
        {
          return 0;
        }
      build->ids = grown_ids;
      build->capacity = new_capacity;
    }

  build->nodes[build->count] = node;
  build->ids[build->count] = id;
  ++build->count;
  return 1;
}

static eqx_eclass_id_t
wrs_add_node (eqx_egraph_t *graph, WRSEGraphBuild *build, WILNode *node,
              int *ok)
{
  eqx_eclass_id_t cached;
  eqx_eclass_id_t ids[4];
  WILIndex index;
  WILIndex input_count;
  eqx_eclass_id_t id;

  if (ok == NULL || !*ok || graph == NULL || node == NULL)
    {
      return EQX_ECLASS_ID_INVALID;
    }

  if (wrs_build_lookup (build, node, &cached))
    {
      return cached;
    }

  input_count = wil_node_input_count (node);
  if (!wrs_eqx_arity_supported (input_count))
    {
      *ok = 0;
      return EQX_ECLASS_ID_INVALID;
    }

  for (index = 0u; index < input_count; ++index)
    {
      ids[index] = wrs_add_node (graph, build, wil_node_input (node, index), ok);
      if (!*ok || ids[index] == EQX_ECLASS_ID_INVALID)
        {
          *ok = 0;
          return EQX_ECLASS_ID_INVALID;
        }
    }

  id = eqx_egraph_add (graph, wrs_op_of_kind (wil_node_kind (node)),
                       (size_t) input_count, input_count == 0u ? NULL : ids);
  if (id == EQX_ECLASS_ID_INVALID)
    {
      *ok = 0;
      return EQX_ECLASS_ID_INVALID;
    }
  if (!wrs_build_record (build, node, id))
    {
      *ok = 0;
      return EQX_ECLASS_ID_INVALID;
    }
  return id;
}

static void
wrs_build_destroy (WRSEGraphBuild *build)
{
  if (build == NULL)
    {
      return;
    }
  free (build->nodes);
  free (build->ids);
  memset (build, 0, sizeof (*build));
}

static eqx_pattern_t *
wrs_pattern_binop (uint32_t op, const char *lhs, const char *rhs)
{
  eqx_pattern_t *children[2];

  children[0] = eqx_pattern_var (lhs);
  children[1] = eqx_pattern_var (rhs);
  if (children[0] == NULL || children[1] == NULL)
    {
      eqx_pattern_destroy (children[0]);
      eqx_pattern_destroy (children[1]);
      return NULL;
    }
  return eqx_pattern_app (op, children, 2u);
}

static eqx_rewrite_rule_t *
wrs_make_comm_rule (uint32_t op, const char *name)
{
  eqx_pattern_t *lhs = wrs_pattern_binop (op, "x", "y");
  eqx_pattern_t *rhs = wrs_pattern_binop (op, "y", "x");
  if (lhs == NULL || rhs == NULL)
    {
      eqx_pattern_destroy (lhs);
      eqx_pattern_destroy (rhs);
      return NULL;
    }
  return eqx_rewrite_rule_create (name, lhs, rhs, NULL);
}

static eqx_rewrite_rule_t *
wrs_make_assoc_rule (uint32_t op, const char *name, int reverse)
{
  eqx_pattern_t *lhs_children[2];
  eqx_pattern_t *rhs_children[2];
  eqx_pattern_t *inner;
  eqx_pattern_t *lhs;
  eqx_pattern_t *rhs;

  if (!reverse)
    {
      lhs_children[0] = eqx_pattern_var ("x");
      lhs_children[1] = eqx_pattern_app (
          op, (eqx_pattern_t *[]){ eqx_pattern_var ("y"), eqx_pattern_var ("z") },
          2u);
      rhs_children[0] = eqx_pattern_app (
          op, (eqx_pattern_t *[]){ eqx_pattern_var ("x"), eqx_pattern_var ("y") },
          2u);
      rhs_children[1] = eqx_pattern_var ("z");
    }
  else
    {
      lhs_children[0] = eqx_pattern_app (
          op, (eqx_pattern_t *[]){ eqx_pattern_var ("x"), eqx_pattern_var ("y") },
          2u);
      lhs_children[1] = eqx_pattern_var ("z");
      rhs_children[0] = eqx_pattern_var ("x");
      rhs_children[1] = eqx_pattern_app (
          op, (eqx_pattern_t *[]){ eqx_pattern_var ("y"), eqx_pattern_var ("z") },
          2u);
    }

  if (lhs_children[0] == NULL || lhs_children[1] == NULL || rhs_children[0] == NULL
      || rhs_children[1] == NULL)
    {
      eqx_pattern_destroy (lhs_children[0]);
      eqx_pattern_destroy (lhs_children[1]);
      eqx_pattern_destroy (rhs_children[0]);
      eqx_pattern_destroy (rhs_children[1]);
      return NULL;
    }

  lhs = eqx_pattern_app (op, lhs_children, 2u);
  rhs = eqx_pattern_app (op, rhs_children, 2u);
  if (lhs == NULL || rhs == NULL)
    {
      eqx_pattern_destroy (lhs);
      eqx_pattern_destroy (rhs);
      return NULL;
    }
  inner = NULL;
  return eqx_rewrite_rule_create (name, lhs, rhs, NULL);
}

static size_t
wrs_saturate_eqx (eqx_egraph_t *graph, uint32_t max_iterations)
{
  eqx_rewrite_rule_t *rules[15];
  size_t rule_count = 0u;
  size_t total;
  size_t index;

  memset (rules, 0, sizeof (rules));

  rules[rule_count++] = wrs_make_comm_rule (wrs_op_of_kind (WIL_NODE_ADD), "add-comm");
  rules[rule_count++] = wrs_make_assoc_rule (wrs_op_of_kind (WIL_NODE_ADD), "add-assoc-lr", 0);
  rules[rule_count++] = wrs_make_assoc_rule (wrs_op_of_kind (WIL_NODE_ADD), "add-assoc-rl", 1);

  rules[rule_count++] = wrs_make_comm_rule (wrs_op_of_kind (WIL_NODE_MUL), "mul-comm");
  rules[rule_count++] = wrs_make_assoc_rule (wrs_op_of_kind (WIL_NODE_MUL), "mul-assoc-lr", 0);
  rules[rule_count++] = wrs_make_assoc_rule (wrs_op_of_kind (WIL_NODE_MUL), "mul-assoc-rl", 1);

  rules[rule_count++] = wrs_make_comm_rule (wrs_op_of_kind (WIL_NODE_AND), "and-comm");
  rules[rule_count++] = wrs_make_assoc_rule (wrs_op_of_kind (WIL_NODE_AND), "and-assoc-lr", 0);
  rules[rule_count++] = wrs_make_assoc_rule (wrs_op_of_kind (WIL_NODE_AND), "and-assoc-rl", 1);

  rules[rule_count++] = wrs_make_comm_rule (wrs_op_of_kind (WIL_NODE_OR), "or-comm");
  rules[rule_count++] = wrs_make_assoc_rule (wrs_op_of_kind (WIL_NODE_OR), "or-assoc-lr", 0);
  rules[rule_count++] = wrs_make_assoc_rule (wrs_op_of_kind (WIL_NODE_OR), "or-assoc-rl", 1);

  rules[rule_count++] = wrs_make_comm_rule (wrs_op_of_kind (WIL_NODE_XOR), "xor-comm");
  rules[rule_count++] = wrs_make_assoc_rule (wrs_op_of_kind (WIL_NODE_XOR), "xor-assoc-lr", 0);
  rules[rule_count++] = wrs_make_assoc_rule (wrs_op_of_kind (WIL_NODE_XOR), "xor-assoc-rl", 1);

  for (index = 0u; index < rule_count; ++index)
    {
      if (rules[index] == NULL)
        {
          size_t j;
          for (j = 0u; j < rule_count; ++j)
            {
              eqx_rewrite_rule_destroy (rules[j]);
            }
          return 0u;
        }
    }

  total = eqx_rewrite_apply_rules (rules, rule_count, graph,
                                   max_iterations == 0u ? 1u : (size_t) max_iterations);
  for (index = 0u; index < rule_count; ++index)
    {
      eqx_rewrite_rule_destroy (rules[index]);
    }
  return total;
}

static double
wrs_node_cost (WILNode *node)
{
  if (node == NULL)
    {
      return 1000000.0;
    }
  switch (wil_node_kind (node))
    {
    case WIL_NODE_CONST_INT:
    case WIL_NODE_CONST_FLOAT:
    case WIL_NODE_CONST_BOOL:
    case WIL_NODE_PARAM:
      return 1.0;
    default:
      return 2.0 + (double) wil_node_input_count (node);
    }
}

static WILNode *
wrs_pick_best (eqx_egraph_t *graph, const WRSEGraphBuild *build,
               eqx_eclass_id_t root_id)
{
  WRSEntry *entries;
  uint32_t index;
  WILNode *best = NULL;
  double best_cost = 1.0e18;
  eqx_egraph_iter_t iter;

  if (graph == NULL || build == NULL)
    {
      return NULL;
    }

  entries = (WRSEntry *) calloc (build->count, sizeof (*entries));
  if (entries == NULL)
    {
      return NULL;
    }

  for (index = 0u; index < build->count; ++index)
    {
      entries[index].node = build->nodes[index];
      entries[index].cost = wrs_node_cost (build->nodes[index]);
      entries[index].valid = 1;
    }

  iter = eqx_egraph_iter_begin (graph);
  while (eqx_egraph_iter_has_next (iter))
    {
      eqx_eclass_id_t class_id = eqx_egraph_iter_next (iter);
      if (eqx_egraph_find (graph, class_id) == eqx_egraph_find (graph, root_id))
        {
          for (index = 0u; index < build->count; ++index)
            {
              if (eqx_egraph_find (graph, build->ids[index])
                  == eqx_egraph_find (graph, root_id))
                {
                  if (entries[index].valid && entries[index].cost < best_cost)
                    {
                      best_cost = entries[index].cost;
                      best = entries[index].node;
                    }
                }
            }
          break;
        }
    }
  eqx_egraph_iter_end (iter);
  free (entries);
  return best;
}

WILNode *
wrs_normalize_with_egraph (WILRewriteSystem *sys, WILNode *node,
                           uint32_t maxIterations)
{
  eqx_egraph_config_t cfg = eqx_egraph_config_default ();
  eqx_egraph_t *graph;
  WRSEGraphBuild build;
  eqx_eclass_id_t root_id;
  int ok = 1;
  WILNode *best;

  if (sys == NULL || node == NULL)
    {
      return node;
    }

  memset (&build, 0, sizeof (build));
  cfg.max_iterations = maxIterations == 0u ? 1u : (size_t) maxIterations;
  graph = eqx_egraph_create (&cfg);
  if (graph == NULL)
    {
      return NULL;
    }

  root_id = wrs_add_node (graph, &build, node, &ok);
  if (!ok || root_id == EQX_ECLASS_ID_INVALID)
    {
      wrs_build_destroy (&build);
      eqx_egraph_destroy (graph);
      return NULL;
    }

  (void) eqx_egraph_rebuild (graph);
  (void) wrs_saturate_eqx (graph, maxIterations);
  (void) eqx_egraph_rebuild (graph);

  best = wrs_pick_best (graph, &build, root_id);
  wrs_build_destroy (&build);
  eqx_egraph_destroy (graph);
  return best;
}

int
wrs_nodes_equivalent (WILNode *a, WILNode *b)
{
  eqx_egraph_t *graph;
  eqx_egraph_config_t cfg = eqx_egraph_config_default ();
  WRSEGraphBuild build;
  eqx_eclass_id_t ia;
  eqx_eclass_id_t ib;
  int ok = 1;
  int equal;

  if (a == b)
    {
      return 1;
    }
  if (a == NULL || b == NULL)
    {
      return 0;
    }

  memset (&build, 0, sizeof (build));
  cfg.max_iterations = 8u;
  graph = eqx_egraph_create (&cfg);
  if (graph == NULL)
    {
      return 0;
    }

  ia = wrs_add_node (graph, &build, a, &ok);
  ib = wrs_add_node (graph, &build, b, &ok);
  if (!ok || ia == EQX_ECLASS_ID_INVALID || ib == EQX_ECLASS_ID_INVALID)
    {
      wrs_build_destroy (&build);
      eqx_egraph_destroy (graph);
      return 0;
    }

  (void) eqx_egraph_rebuild (graph);
  if (wrs_is_eqx_rewritable_kind (wil_node_kind (a))
      || wrs_is_eqx_rewritable_kind (wil_node_kind (b)))
    {
      (void) wrs_saturate_eqx (graph, 8u);
      (void) eqx_egraph_rebuild (graph);
    }
  equal = eqx_egraph_equiv (graph, ia, ib) ? 1 : 0;

  wrs_build_destroy (&build);
  eqx_egraph_destroy (graph);
  return equal;
}
