#include "wil_nodes.h"

#include <stdio.h>

const char *
wil_node_kind_str (WILNodeKind kind)
{
  switch (kind)
    {
    case WIL_NODE_CONST_INT: return "const_int";
    case WIL_NODE_CONST_FLOAT: return "const_float";
    case WIL_NODE_CONST_BOOL: return "const_bool";
    case WIL_NODE_UNDEF: return "undef";
    case WIL_NODE_PARAM: return "param";
    case WIL_NODE_ADD: return "add";
    case WIL_NODE_SUB: return "sub";
    case WIL_NODE_MUL: return "mul";
    case WIL_NODE_DIV: return "div";
    case WIL_NODE_MOD: return "mod";
    case WIL_NODE_AND: return "and";
    case WIL_NODE_OR: return "or";
    case WIL_NODE_XOR: return "xor";
    case WIL_NODE_SHL: return "shl";
    case WIL_NODE_SHR: return "shr";
    case WIL_NODE_NEG: return "neg";
    case WIL_NODE_NOT: return "not";
    case WIL_NODE_ABS: return "abs";
    case WIL_NODE_EQ: return "eq";
    case WIL_NODE_NE: return "ne";
    case WIL_NODE_LT: return "lt";
    case WIL_NODE_LE: return "le";
    case WIL_NODE_GT: return "gt";
    case WIL_NODE_GE: return "ge";
    case WIL_NODE_LOAD: return "load";
    case WIL_NODE_STORE: return "store";
    case WIL_NODE_ALLOCA: return "alloca";
    case WIL_NODE_START: return "start";
    case WIL_NODE_REGION: return "region";
    case WIL_NODE_IF: return "if";
    case WIL_NODE_LOOP: return "loop";
    case WIL_NODE_RETURN: return "return";
    case WIL_NODE_JUMP: return "jump";
    case WIL_NODE_PROJ: return "proj";
    case WIL_NODE_PHI: return "phi";
    case WIL_NODE_CALL: return "call";
    case WIL_NODE_CALL_INDIRECT: return "call_indirect";
    case WIL_NODE_CAST: return "cast";
    case WIL_NODE_BITCAST: return "bitcast";
    case WIL_NODE_EXTRACT: return "extract";
    case WIL_NODE_INSERT: return "insert";
    case WIL_NODE_SELECT: return "select";
    default: return "unknown";
    }
}

const char *
wil_type_str (WILType type)
{
  switch (type)
    {
    case WIL_TYPE_VOID: return "void";
    case WIL_TYPE_I1: return "i1";
    case WIL_TYPE_I8: return "i8";
    case WIL_TYPE_I16: return "i16";
    case WIL_TYPE_I32: return "i32";
    case WIL_TYPE_I64: return "i64";
    case WIL_TYPE_F32: return "f32";
    case WIL_TYPE_F64: return "f64";
    case WIL_TYPE_PTR: return "ptr";
    case WIL_TYPE_CONTROL: return "control";
    default: return "?";
    }
}

const char *
wil_edge_kind_str (WILEdgeKind kind)
{
  switch (kind)
    {
    case WIL_EDGE_DATA: return "data";
    case WIL_EDGE_CONTROL: return "control";
    case WIL_EDGE_EFFECT: return "effect";
    default: return "unknown";
    }
}

static void
wil_print_payload (const WILNode *node, FILE *out)
{
  switch (node->kind)
    {
    case WIL_NODE_CONST_INT:
      fprintf (out, " %lld", node->payload.int_value);
      break;
    case WIL_NODE_CONST_FLOAT:
      fprintf (out, " %g", node->payload.float_value);
      break;
    case WIL_NODE_CONST_BOOL:
      fprintf (out, " %s", node->payload.bool_value ? "true" : "false");
      break;
    case WIL_NODE_PARAM:
    case WIL_NODE_PROJ:
    case WIL_NODE_EXTRACT:
    case WIL_NODE_INSERT:
      fprintf (out, " #%lu", (unsigned long) node->payload.index_value);
      break;
    case WIL_NODE_CALL:
      if (node->name != NULL)
        {
          fprintf (out, " %s", node->name);
        }
      break;
    default:
      break;
    }
}

void
wil_print_node (const WILNode *node, FILE *out)
{
  WILIndex index;

  if (node == NULL)
    {
      return;
    }

  if (out == NULL)
    {
      out = stdout;
    }

  fprintf (out, "n%lu = %s : %s", (unsigned long) node->id,
           wil_node_kind_str (node->kind), wil_type_str (node->type));
  if (node->name != NULL && node->kind != WIL_NODE_CALL)
    {
      fprintf (out, " [%s]", node->name);
    }
  wil_print_payload (node, out);
  if (node->input_count != 0u)
    {
      fprintf (out, " <-");
      for (index = 0u; index < node->input_count; ++index)
        {
          fprintf (out, " %s:n%lu", wil_edge_kind_str (node->inputs[index].edge_kind),
                   (unsigned long) wil_node_id (node->inputs[index].node));
        }
    }
  fprintf (out, "\n");
}

void
wil_print_graph (const WILGraph *graph, FILE *out)
{
  WILIndex index;

  if (graph == NULL)
    {
      return;
    }

  if (out == NULL)
    {
      out = stdout;
    }

  fprintf (out, "WIL graph: %lu nodes\n", (unsigned long) graph->node_count);
  for (index = 0u; index < graph->node_count; ++index)
    {
      wil_print_node (graph->nodes[index], out);
    }
}

void wil_print_cfg (const WILCFG *cfg, FILE *out)
{ (void) cfg; if (out != NULL) fprintf (out, "CFG extraction not implemented\n"); }
void wil_print_schedule (const WILSchedule *schedule, FILE *out)
{ (void) schedule; if (out != NULL) fprintf (out, "Scheduling not implemented\n"); }
void wil_print_domtree (const WILDomTree *tree, FILE *out)
{ (void) tree; if (out != NULL) fprintf (out, "Dominance not implemented\n"); }
