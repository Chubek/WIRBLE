#include "wil_builder.h"

#include <stdlib.h>
#include <string.h>

static WILNode *
wil_make_typed_node (WILContext *ctx, WILNodeKind kind, WILType type,
                     const char *name)
{
  return wil_node_create (ctx, kind, type, name);
}

WILNode *
wil_build_unary (WILContext *ctx, WILNodeKind kind, WILNode *operand,
                 WILType type_hint)
{
  WILNode *node;
  WILType type;

  type = wil_node_kind_default_type (kind, type_hint, operand, NULL);
  node = wil_make_typed_node (ctx, kind, type, NULL);
  if (node != NULL)
    {
      wil_node_add_input (node, operand,
                          kind == WIL_NODE_RETURN ? WIL_EDGE_CONTROL
                                                  : WIL_EDGE_DATA);
    }
  return node;
}

WILNode *
wil_build_binary (WILContext *ctx, WILNodeKind kind, WILNode *lhs,
                  WILNode *rhs, WILType type_hint)
{
  WILNode *node;
  WILType type;

  type = wil_node_kind_default_type (kind, type_hint, lhs, rhs);
  node = wil_make_typed_node (ctx, kind, type, NULL);
  if (node == NULL)
    {
      return NULL;
    }

  wil_node_add_input (node, lhs, WIL_EDGE_DATA);
  wil_node_add_input (node, rhs, WIL_EDGE_DATA);
  return node;
}

WILNode *
wil_build_nary (WILContext *ctx, WILNodeKind kind, WILNode **inputs,
                const WILEdgeKind *edge_kinds, WILIndex count,
                WILType type_hint, const char *name)
{
  WILNode *node;
  WILIndex index;
  WILType type;

  type = wil_node_kind_default_type (kind, type_hint,
                                     count == 0u ? NULL : inputs[0],
                                     count > 1u ? inputs[1] : NULL);
  node = wil_make_typed_node (ctx, kind, type, name);
  if (node == NULL)
    {
      return NULL;
    }

  for (index = 0u; index < count; ++index)
    {
      wil_node_add_input (node, inputs[index],
                          edge_kinds == NULL ? WIL_EDGE_DATA : edge_kinds[index]);
    }

  return node;
}

WILNode *
wil_make_const_int (WILContext *ctx, long long value, WILType type)
{
  WILNode *node = wil_node_create (ctx, WIL_NODE_CONST_INT, type, NULL);
  if (node != NULL)
    {
      node->payload.int_value = value;
    }
  return node;
}

WILNode *
wil_make_const_float (WILContext *ctx, double value, WILType type)
{
  WILNode *node = wil_node_create (ctx, WIL_NODE_CONST_FLOAT, type, NULL);
  if (node != NULL)
    {
      node->payload.float_value = value;
    }
  return node;
}

WILNode *
wil_make_const_bool (WILContext *ctx, int value)
{
  WILNode *node = wil_node_create (ctx, WIL_NODE_CONST_BOOL, WIL_TYPE_I1, NULL);
  if (node != NULL)
    {
      node->payload.bool_value = value != 0;
    }
  return node;
}

WILNode *wil_make_undef (WILContext *ctx, WILType type)
{ return wil_node_create (ctx, WIL_NODE_UNDEF, type, NULL); }

WILNode *
wil_make_param (WILContext *ctx, WILIndex index, WILType type, const char *name)
{
  WILNode *node = wil_node_create (ctx, WIL_NODE_PARAM, type, name);
  if (node != NULL)
    {
      node->payload.index_value = index;
    }
  return node;
}

#define WIL_DEFINE_BINARY_BUILDER(fn_name, kind_name) \
  WILNode *fn_name (WILContext *ctx, WILNode *lhs, WILNode *rhs) \
  { return wil_build_binary (ctx, kind_name, lhs, rhs, WIL_TYPE_VOID); }

#define WIL_DEFINE_UNARY_BUILDER(fn_name, kind_name) \
  WILNode *fn_name (WILContext *ctx, WILNode *operand) \
  { return wil_build_unary (ctx, kind_name, operand, WIL_TYPE_VOID); }

WIL_DEFINE_BINARY_BUILDER (wil_make_add, WIL_NODE_ADD)
WIL_DEFINE_BINARY_BUILDER (wil_make_sub, WIL_NODE_SUB)
WIL_DEFINE_BINARY_BUILDER (wil_make_mul, WIL_NODE_MUL)
WIL_DEFINE_BINARY_BUILDER (wil_make_div, WIL_NODE_DIV)
WIL_DEFINE_BINARY_BUILDER (wil_make_mod, WIL_NODE_MOD)
WIL_DEFINE_BINARY_BUILDER (wil_make_and, WIL_NODE_AND)
WIL_DEFINE_BINARY_BUILDER (wil_make_or, WIL_NODE_OR)
WIL_DEFINE_BINARY_BUILDER (wil_make_xor, WIL_NODE_XOR)
WIL_DEFINE_BINARY_BUILDER (wil_make_shl, WIL_NODE_SHL)
WIL_DEFINE_BINARY_BUILDER (wil_make_shr, WIL_NODE_SHR)
WIL_DEFINE_UNARY_BUILDER (wil_make_neg, WIL_NODE_NEG)
WIL_DEFINE_UNARY_BUILDER (wil_make_not, WIL_NODE_NOT)
WIL_DEFINE_UNARY_BUILDER (wil_make_abs, WIL_NODE_ABS)
WIL_DEFINE_BINARY_BUILDER (wil_make_eq, WIL_NODE_EQ)
WIL_DEFINE_BINARY_BUILDER (wil_make_ne, WIL_NODE_NE)
WIL_DEFINE_BINARY_BUILDER (wil_make_lt, WIL_NODE_LT)
WIL_DEFINE_BINARY_BUILDER (wil_make_le, WIL_NODE_LE)
WIL_DEFINE_BINARY_BUILDER (wil_make_gt, WIL_NODE_GT)
WIL_DEFINE_BINARY_BUILDER (wil_make_ge, WIL_NODE_GE)

WILNode *
wil_make_load (WILContext *ctx, WILNode *address, WILNode *effect, WILType type)
{
  WILNode *inputs[2] = { address, effect };
  WILEdgeKind edges[2] = { WIL_EDGE_DATA, WIL_EDGE_EFFECT };
  return wil_build_nary (ctx, WIL_NODE_LOAD, inputs, edges, 2u, type, NULL);
}

WILNode *
wil_make_store (WILContext *ctx, WILNode *address, WILNode *value, WILNode *effect)
{
  WILNode *inputs[3] = { address, value, effect };
  WILEdgeKind edges[3] = { WIL_EDGE_DATA, WIL_EDGE_DATA, WIL_EDGE_EFFECT };
  return wil_build_nary (ctx, WIL_NODE_STORE, inputs, edges, 3u, WIL_TYPE_VOID,
                         NULL);
}

WILNode *
wil_make_alloca (WILContext *ctx, WILType type, WILNode *size)
{
  WILNode *node = wil_node_create (ctx, WIL_NODE_ALLOCA, WIL_TYPE_PTR, NULL);
  if (node != NULL)
    {
      node->payload.index_value = (WILIndex) type;
      if (size != NULL)
        {
          wil_node_add_input (node, size, WIL_EDGE_DATA);
        }
    }
  return node;
}

WILNode *wil_make_start (WILContext *ctx)
{ return wil_node_create (ctx, WIL_NODE_START, WIL_TYPE_CONTROL, "start"); }

WILNode *
wil_make_return (WILContext *ctx, WILNode *control, WILNode *value)
{
  WILNode *inputs[2] = { control, value };
  WILEdgeKind edges[2] = { WIL_EDGE_CONTROL, WIL_EDGE_DATA };
  return wil_build_nary (ctx, WIL_NODE_RETURN, inputs, edges, 2u,
                         WIL_TYPE_CONTROL, NULL);
}

WILNode *
wil_make_jump (WILContext *ctx, WILNode *control, WILNode *target)
{
  WILNode *inputs[2] = { control, target };
  WILEdgeKind edges[2] = { WIL_EDGE_CONTROL, WIL_EDGE_CONTROL };
  return wil_build_nary (ctx, WIL_NODE_JUMP, inputs, edges, 2u,
                         WIL_TYPE_CONTROL, NULL);
}

WILNode *
wil_make_region (WILContext *ctx, WILNode **controls, WILIndex count)
{
  WILEdgeKind stack_edges[16];
  WILEdgeKind *edges;
  WILIndex index;

  edges = count <= 16u ? stack_edges : (WILEdgeKind *) malloc (
                                           (size_t) count * sizeof (*edges));
  if (edges == NULL)
    {
      return NULL;
    }

  for (index = 0u; index < count; ++index)
    {
      edges[index] = WIL_EDGE_CONTROL;
    }

  {
    WILNode *node = wil_build_nary (ctx, WIL_NODE_REGION, controls, edges, count,
                                    WIL_TYPE_CONTROL, NULL);
    if (edges != stack_edges)
      {
        free (edges);
      }
    return node;
  }
}

WILNode *
wil_make_if (WILContext *ctx, WILNode *control, WILNode *condition)
{
  WILNode *inputs[2] = { control, condition };
  WILEdgeKind edges[2] = { WIL_EDGE_CONTROL, WIL_EDGE_DATA };
  return wil_build_nary (ctx, WIL_NODE_IF, inputs, edges, 2u,
                         WIL_TYPE_CONTROL, NULL);
}

WILNode *
wil_make_loop (WILContext *ctx, WILNode *entry_control)
{
  WILNode *inputs[1] = { entry_control };
  WILEdgeKind edges[1] = { WIL_EDGE_CONTROL };
  return wil_build_nary (ctx, WIL_NODE_LOOP, inputs, edges, 1u,
                         WIL_TYPE_CONTROL, NULL);
}

WILNode *
wil_make_proj (WILContext *ctx, WILNode *control, WILIndex index)
{
  WILNode *node = wil_build_unary (ctx, WIL_NODE_PROJ, control, WIL_TYPE_CONTROL);
  if (node != NULL)
    {
      node->payload.index_value = index;
    }
  return node;
}

WILNode *
wil_make_phi (WILContext *ctx, WILNode *region, WILNode **values, WILIndex count)
{
  WILNode *node;
  WILIndex index;
  WILType type = count == 0u || values[0] == NULL ? WIL_TYPE_VOID : values[0]->type;

  node = wil_node_create (ctx, WIL_NODE_PHI, type, NULL);
  if (node == NULL)
    {
      return NULL;
    }

  wil_node_add_input (node, region, WIL_EDGE_CONTROL);
  for (index = 0u; index < count; ++index)
    {
      wil_node_add_input (node, values[index], WIL_EDGE_DATA);
    }
  return node;
}

WILNode *
wil_make_call (WILContext *ctx, WILNode *control, WILNode *effect,
               const char *callee, WILNode **args, WILIndex arg_count)
{
  WILNode *node = wil_node_create (ctx, WIL_NODE_CALL, WIL_TYPE_VOID, callee);
  WILIndex index;
  if (node == NULL)
    {
      return NULL;
    }

  wil_node_add_input (node, control, WIL_EDGE_CONTROL);
  wil_node_add_input (node, effect, WIL_EDGE_EFFECT);
  for (index = 0u; index < arg_count; ++index)
    {
      wil_node_add_input (node, args[index], WIL_EDGE_DATA);
    }
  return node;
}

WILNode *
wil_make_call_indirect (WILContext *ctx, WILNode *control, WILNode *effect,
                        WILNode *callee, WILNode **args, WILIndex arg_count)
{
  WILNode *node = wil_node_create (ctx, WIL_NODE_CALL_INDIRECT, WIL_TYPE_VOID, NULL);
  WILIndex index;
  if (node == NULL)
    {
      return NULL;
    }

  wil_node_add_input (node, control, WIL_EDGE_CONTROL);
  wil_node_add_input (node, effect, WIL_EDGE_EFFECT);
  wil_node_add_input (node, callee, WIL_EDGE_DATA);
  for (index = 0u; index < arg_count; ++index)
    {
      wil_node_add_input (node, args[index], WIL_EDGE_DATA);
    }
  return node;
}

WILNode *
wil_make_cast (WILContext *ctx, WILNode *value, WILType target_type)
{ return wil_build_unary (ctx, WIL_NODE_CAST, value, target_type); }

WILNode *
wil_make_bitcast (WILContext *ctx, WILNode *value, WILType target_type)
{ return wil_build_unary (ctx, WIL_NODE_BITCAST, value, target_type); }

WILNode *
wil_make_extract (WILContext *ctx, WILNode *aggregate, WILIndex index)
{
  WILNode *node = wil_build_unary (ctx, WIL_NODE_EXTRACT, aggregate, WIL_TYPE_VOID);
  if (node != NULL)
    {
      node->payload.index_value = index;
    }
  return node;
}

WILNode *
wil_make_insert (WILContext *ctx, WILNode *aggregate, WILNode *value, WILIndex index)
{
  WILNode *node = wil_build_binary (ctx, WIL_NODE_INSERT, aggregate, value,
                                    aggregate == NULL ? WIL_TYPE_VOID : aggregate->type);
  if (node != NULL)
    {
      node->payload.index_value = index;
    }
  return node;
}

WILNode *
wil_make_select (WILContext *ctx, WILNode *condition, WILNode *true_val,
                 WILNode *false_val)
{
  WILNode *inputs[3] = { condition, true_val, false_val };
  WILEdgeKind edges[3] = { WIL_EDGE_DATA, WIL_EDGE_DATA, WIL_EDGE_DATA };
  WILType type = true_val == NULL ? WIL_TYPE_VOID : true_val->type;
  return wil_build_nary (ctx, WIL_NODE_SELECT, inputs, edges, 3u, type, NULL);
}
