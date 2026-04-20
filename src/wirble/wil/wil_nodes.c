#include "wil_nodes.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WIL_NODE_FLAG_REMOVED 0x1u

static void *
wil_xrealloc (void *ptr, size_t size)
{
  if (size == 0u)
    {
      free (ptr);
      return NULL;
    }

  return realloc (ptr, size);
}

void *
wil_context_alloc (WILContext *ctx, size_t size, size_t alignment)
{
  if (ctx == NULL || ctx->attached_arena == NULL)
    {
      return calloc (1u, size);
    }

  return wirble_arena_calloc_from (&ctx->attached_arena->arena, 1u, size,
                                   alignment);
}

const char *
wil_context_strdup (WILContext *ctx, const char *text)
{
  size_t size;
  char *copy;

  if (text == NULL)
    {
      return NULL;
    }

  size = strlen (text) + 1u;
  if (ctx != NULL && ctx->attached_arena != NULL)
    {
      return wirble_arena_strdup (&ctx->attached_arena->arena, text);
    }

  copy = (char *) malloc (size);
  if (copy == NULL)
    {
      return NULL;
    }

  memcpy (copy, text, size);
  return copy;
}

static void
wil_graph_destroy (WILGraph *graph, int free_nodes)
{
  WILIndex index;

  if (graph == NULL)
    {
      return;
    }

  if (free_nodes)
    {
      for (index = 0u; index < graph->node_count; ++index)
        {
          WILNode *node = graph->nodes[index];
          if (node == NULL)
            {
              continue;
            }
          free (node->inputs);
          free (node->uses);
          if (node->context != NULL && node->context->attached_arena == NULL)
            {
              free ((char *) node->name);
              free (node);
            }
        }
    }

  free (graph->nodes);
  graph->nodes = NULL;
  graph->node_count = 0u;
  graph->node_capacity = 0u;
}

static void
wil_context_init (WILContext *ctx)
{
  memset (ctx, 0, sizeof (*ctx));
}

WILContext *
wil_context_create (void)
{
  WILContext *ctx;

  ctx = (WILContext *) calloc (1u, sizeof (*ctx));
  if (ctx == NULL)
    {
      return NULL;
    }

  wil_context_init (ctx);
  ctx->attached_arena = wil_arena_create ();
  ctx->owns_attached_arena = ctx->attached_arena != NULL;
  return ctx;
}

void
wil_context_destroy (WILContext *ctx)
{
  if (ctx == NULL)
    {
      return;
    }

  wil_graph_destroy (&ctx->graph, 1);
  if (ctx->owns_attached_arena)
    {
      wil_arena_destroy (ctx->attached_arena);
    }
  free (ctx);
}

WILGraph *
wil_context_graph (WILContext *ctx)
{
  return ctx == NULL ? NULL : &ctx->graph;
}

WILArena *
wil_arena_create (void)
{
  WILArena *arena;

  arena = (WILArena *) calloc (1u, sizeof (*arena));
  if (arena == NULL)
    {
      return NULL;
    }

  wirble_arena_init (&arena->arena, 4096u);
  return arena;
}

void
wil_arena_destroy (WILArena *arena)
{
  if (arena == NULL)
    {
      return;
    }

  wirble_arena_destroy (&arena->arena);
  free (arena);
}

void *
wil_arena_alloc (WILArena *arena, unsigned long size)
{
  if (arena == NULL || size == 0ul)
    {
      return NULL;
    }

  return wirble_arena_calloc_from (&arena->arena, 1u, (size_t) size,
                                   sizeof (void *));
}

void
wil_arena_reset (WILArena *arena)
{
  if (arena != NULL)
    {
      wirble_arena_reset (&arena->arena);
    }
}

void
wil_context_set_arena (WILContext *ctx, WILArena *arena)
{
  if (ctx == NULL)
    {
      return;
    }

  if (ctx->owns_attached_arena)
    {
      wil_arena_destroy (ctx->attached_arena);
    }

  ctx->attached_arena = arena;
  ctx->owns_attached_arena = 0;
}

WILIndex
wil_graph_node_count (const WILGraph *graph)
{
  return graph == NULL ? 0u : graph->node_count;
}

WILNode *
wil_graph_get_node (const WILGraph *graph, WILIndex index)
{
  if (graph == NULL || index >= graph->node_count)
    {
      return NULL;
    }

  return graph->nodes[index];
}

WILNode *
wil_graph_find_node (const WILGraph *graph, WILNodeId id)
{
  WILIndex index;

  if (graph == NULL)
    {
      return NULL;
    }

  for (index = 0u; index < graph->node_count; ++index)
    {
      if (graph->nodes[index] != NULL && graph->nodes[index]->id == id)
        {
          return graph->nodes[index];
        }
    }

  return NULL;
}

WILNode *
wil_graph_first_node (const WILGraph *graph)
{
  return wil_graph_get_node (graph, 0u);
}

WILNode *
wil_graph_next_node (const WILGraph *graph, const WILNode *node)
{
  WILIndex index;

  if (graph == NULL || node == NULL)
    {
      return NULL;
    }

  for (index = 0u; index + 1u < graph->node_count; ++index)
    {
      if (graph->nodes[index] == node)
        {
          return graph->nodes[index + 1u];
        }
    }

  return NULL;
}

int
wil_graph_append_node (WILGraph *graph, WILNode *node)
{
  WILNode **new_nodes;
  WILIndex new_capacity;

  if (graph == NULL || node == NULL)
    {
      return 0;
    }

  if (graph->node_count == graph->node_capacity)
    {
      new_capacity = graph->node_capacity == 0u ? 16u : graph->node_capacity * 2u;
      new_nodes = (WILNode **) wil_xrealloc (graph->nodes,
                                             (size_t) new_capacity
                                                 * sizeof (*new_nodes));
      if (new_nodes == NULL)
        {
          return 0;
        }
      graph->nodes = new_nodes;
      graph->node_capacity = new_capacity;
    }

  graph->nodes[graph->node_count++] = node;
  return 1;
}

WILNodeCategory
wil_node_kind_category (WILNodeKind kind)
{
  switch (kind)
    {
    case WIL_NODE_CONST_INT:
    case WIL_NODE_CONST_FLOAT:
    case WIL_NODE_CONST_BOOL:
    case WIL_NODE_UNDEF:
      return WIL_CAT_CONSTANT;
    case WIL_NODE_PARAM:
      return WIL_CAT_PARAMETER;
    case WIL_NODE_START:
    case WIL_NODE_REGION:
    case WIL_NODE_IF:
    case WIL_NODE_LOOP:
    case WIL_NODE_RETURN:
    case WIL_NODE_JUMP:
    case WIL_NODE_PROJ:
      return WIL_CAT_CONTROL;
    case WIL_NODE_STORE:
    case WIL_NODE_CALL:
    case WIL_NODE_CALL_INDIRECT:
      return WIL_CAT_EFFECT;
    default:
      return WIL_CAT_VALUE;
    }
}

WILType
wil_node_kind_default_type (WILNodeKind kind, WILType hint, WILNode *lhs,
                            WILNode *rhs)
{
  (void) rhs;
  if (hint != WIL_TYPE_VOID)
    {
      return hint;
    }

  switch (kind)
    {
    case WIL_NODE_CONST_BOOL:
    case WIL_NODE_EQ:
    case WIL_NODE_NE:
    case WIL_NODE_LT:
    case WIL_NODE_LE:
    case WIL_NODE_GT:
    case WIL_NODE_GE:
      return WIL_TYPE_I1;
    case WIL_NODE_START:
    case WIL_NODE_REGION:
    case WIL_NODE_IF:
    case WIL_NODE_LOOP:
    case WIL_NODE_RETURN:
    case WIL_NODE_JUMP:
    case WIL_NODE_PROJ:
      return WIL_TYPE_CONTROL;
    case WIL_NODE_STORE:
    case WIL_NODE_CALL:
    case WIL_NODE_CALL_INDIRECT:
      return WIL_TYPE_VOID;
    case WIL_NODE_ALLOCA:
    case WIL_NODE_LOAD:
    case WIL_NODE_EXTRACT:
    case WIL_NODE_INSERT:
    case WIL_NODE_CAST:
    case WIL_NODE_BITCAST:
    case WIL_NODE_SELECT:
    case WIL_NODE_PHI:
      return lhs == NULL ? WIL_TYPE_PTR : lhs->type;
    default:
      return lhs == NULL ? WIL_TYPE_I32 : lhs->type;
    }
}

WILNode *
wil_node_create (WILContext *ctx, WILNodeKind kind, WILType type,
                 const char *name)
{
  WILNode *node;

  if (ctx == NULL)
    {
      return NULL;
    }

  node = (WILNode *) wil_context_alloc (ctx, sizeof (*node), _Alignof (WILNode));
  if (node == NULL)
    {
      return NULL;
    }

  memset (node, 0, sizeof (*node));
  node->context = ctx;
  node->id = (WILNodeId) (ctx->graph.node_count + 1u);
  node->kind = kind;
  node->category = wil_node_kind_category (kind);
  node->type = type;
  node->name = wil_context_strdup (ctx, name);
  if (!wil_graph_append_node (&ctx->graph, node))
    {
      return NULL;
    }

  return node;
}

WILNodeId
wil_node_id (const WILNode *node)
{
  return node == NULL ? 0ul : node->id;
}

WILNodeKind
wil_node_kind (const WILNode *node)
{
  return node == NULL ? WIL_NODE_CONST_INT : node->kind;
}

WILNodeCategory
wil_node_category (const WILNode *node)
{
  return node == NULL ? WIL_CAT_VALUE : node->category;
}

WILType
wil_node_type (const WILNode *node)
{
  return node == NULL ? WIL_TYPE_VOID : node->type;
}

const char *
wil_node_name (const WILNode *node)
{
  return node == NULL ? NULL : node->name;
}

WILIndex
wil_node_input_count (const WILNode *node)
{
  return node == NULL ? 0u : node->input_count;
}

WILNode *
wil_node_input (const WILNode *node, WILIndex index)
{
  if (node == NULL || index >= node->input_count)
    {
      return NULL;
    }

  return node->inputs[index].node;
}

WILEdgeKind
wil_node_input_edge_kind (const WILNode *node, WILIndex index)
{
  if (node == NULL || index >= node->input_count)
    {
      return WIL_EDGE_DATA;
    }

  return node->inputs[index].edge_kind;
}

WILIndex
wil_node_use_count (const WILNode *node)
{
  return node == NULL ? 0u : node->use_count;
}

WILNode *
wil_node_use (const WILNode *node, WILIndex index)
{
  if (node == NULL || index >= node->use_count)
    {
      return NULL;
    }

  return node->uses[index];
}

int
wil_node_is_const (const WILNode *node)
{
  return node != NULL && node->category == WIL_CAT_CONSTANT;
}

long long
wil_node_const_int (const WILNode *node)
{
  return (node != NULL && node->kind == WIL_NODE_CONST_INT) ? node->payload.int_value : 0ll;
}

double
wil_node_const_float (const WILNode *node)
{
  return (node != NULL && node->kind == WIL_NODE_CONST_FLOAT) ? node->payload.float_value : 0.0;
}

int
wil_node_const_bool (const WILNode *node)
{
  return (node != NULL && node->kind == WIL_NODE_CONST_BOOL) ? node->payload.bool_value : 0;
}

int
wil_node_ensure_input_capacity (WILNode *node, WILIndex capacity)
{
  WILInputSlot *new_inputs;
  WILIndex new_capacity;

  if (node == NULL)
    {
      return 0;
    }

  if (capacity <= node->input_capacity)
    {
      return 1;
    }

  new_capacity = node->input_capacity == 0u ? 4u : node->input_capacity;
  while (new_capacity < capacity)
    {
      new_capacity *= 2u;
    }

  new_inputs = (WILInputSlot *) wil_xrealloc (node->inputs,
                                              (size_t) new_capacity
                                                  * sizeof (*new_inputs));
  if (new_inputs == NULL)
    {
      return 0;
    }

  node->inputs = new_inputs;
  node->input_capacity = new_capacity;
  return 1;
}

int
wil_node_ensure_use_capacity (WILNode *node, WILIndex capacity)
{
  WILNode **new_uses;
  WILIndex new_capacity;

  if (node == NULL)
    {
      return 0;
    }

  if (capacity <= node->use_capacity)
    {
      return 1;
    }

  new_capacity = node->use_capacity == 0u ? 4u : node->use_capacity;
  while (new_capacity < capacity)
    {
      new_capacity *= 2u;
    }

  new_uses = (WILNode **) wil_xrealloc (node->uses,
                                        (size_t) new_capacity * sizeof (*new_uses));
  if (new_uses == NULL)
    {
      return 0;
    }

  node->uses = new_uses;
  node->use_capacity = new_capacity;
  return 1;
}

void
wil_node_register_use (WILNode *input, WILNode *user)
{
  if (input == NULL || user == NULL)
    {
      return;
    }

  if (!wil_node_ensure_use_capacity (input, input->use_count + 1u))
    {
      return;
    }

  input->uses[input->use_count++] = user;
}

void
wil_node_unregister_use (WILNode *input, WILNode *user)
{
  WILIndex index;

  if (input == NULL || user == NULL)
    {
      return;
    }

  for (index = 0u; index < input->use_count; ++index)
    {
      if (input->uses[index] == user)
        {
          memmove (&input->uses[index], &input->uses[index + 1u],
                   (size_t) (input->use_count - index - 1u)
                       * sizeof (*input->uses));
          --input->use_count;
          return;
        }
    }
}

void
wil_node_add_input (WILNode *node, WILNode *input, WILEdgeKind edge_kind)
{
  if (node == NULL)
    {
      return;
    }

  if (!wil_node_ensure_input_capacity (node, node->input_count + 1u))
    {
      return;
    }

  node->inputs[node->input_count].node = input;
  node->inputs[node->input_count].edge_kind = edge_kind;
  ++node->input_count;
  wil_node_register_use (input, node);
}

void
wil_node_replace_input (WILNode *node, WILIndex index, WILNode *new_input)
{
  if (node == NULL || index >= node->input_count)
    {
      return;
    }

  wil_node_unregister_use (node->inputs[index].node, node);
  node->inputs[index].node = new_input;
  wil_node_register_use (new_input, node);
}

void
wil_node_remove_input (WILNode *node, WILIndex index)
{
  if (node == NULL || index >= node->input_count)
    {
      return;
    }

  wil_node_unregister_use (node->inputs[index].node, node);
  memmove (&node->inputs[index], &node->inputs[index + 1u],
           (size_t) (node->input_count - index - 1u) * sizeof (*node->inputs));
  --node->input_count;
}

void
wil_node_replace_all_uses (WILNode *old_node, WILNode *new_node)
{
  WILIndex index;
  WILIndex use_count;
  WILNode **uses_copy;

  if (old_node == NULL || new_node == NULL || old_node == new_node)
    {
      return;
    }

  use_count = old_node->use_count;
  uses_copy = (WILNode **) malloc ((size_t) use_count * sizeof (*uses_copy));
  if (uses_copy == NULL)
    {
      return;
    }

  memcpy (uses_copy, old_node->uses, (size_t) use_count * sizeof (*uses_copy));
  for (index = 0u; index < use_count; ++index)
    {
      WILNode *user = uses_copy[index];
      WILIndex input_index;
      for (input_index = 0u; input_index < user->input_count; ++input_index)
        {
          if (user->inputs[input_index].node == old_node)
            {
              wil_node_replace_input (user, input_index, new_node);
            }
        }
    }

  free (uses_copy);
}

void
wil_node_remove (WILNode *node)
{
  WILIndex index;

  if (node == NULL || node->use_count != 0u)
    {
      return;
    }

  for (index = node->input_count; index > 0u; --index)
    {
      wil_node_remove_input (node, index - 1u);
    }

  node->flags |= WIL_NODE_FLAG_REMOVED;
}

static void
wil_visit_depth_first (WILNode *node, unsigned char *visited,
                       WILNodeVisitor visitor, void *user_data,
                       int postorder)
{
  WILIndex index;

  if (node == NULL || visited[node->id] != 0u)
    {
      return;
    }

  visited[node->id] = 1u;
  if (!postorder && visitor != NULL)
    {
      visitor (node, user_data);
    }

  for (index = 0u; index < node->input_count; ++index)
    {
      wil_visit_depth_first (node->inputs[index].node, visited, visitor,
                             user_data, postorder);
    }

  if (postorder && visitor != NULL)
    {
      visitor (node, user_data);
    }
}

typedef struct WILPostorderCollector
{
  WILNode **nodes;
  WILIndex count;
} WILPostorderCollector;

static void
wil_collect_postorder (WILNode *node, void *user_data)
{
  WILPostorderCollector *collector;

  collector = (WILPostorderCollector *) user_data;
  collector->nodes[collector->count++] = node;
}

void
wil_graph_traverse_dfs (WILGraph *graph, WILNode *start, WILNodeVisitor visitor,
                        void *user_data)
{
  unsigned char *visited;
  size_t visited_size;

  (void) graph;
  if (start == NULL)
    {
      return;
    }

  visited_size = (size_t) start->context->graph.node_count + 1u;
  visited = (unsigned char *) calloc (visited_size, sizeof (*visited));
  if (visited == NULL)
    {
      return;
    }

  wil_visit_depth_first (start, visited, visitor, user_data, 0);
  free (visited);
}

void
wil_graph_traverse_postorder (WILGraph *graph, WILNode *start,
                              WILNodeVisitor visitor, void *user_data)
{
  unsigned char *visited;
  size_t visited_size;

  (void) graph;
  if (start == NULL)
    {
      return;
    }

  visited_size = (size_t) start->context->graph.node_count + 1u;
  visited = (unsigned char *) calloc (visited_size, sizeof (*visited));
  if (visited == NULL)
    {
      return;
    }

  wil_visit_depth_first (start, visited, visitor, user_data, 1);
  free (visited);
}

void
wil_graph_traverse_bfs (WILGraph *graph, WILNode *start, WILNodeVisitor visitor,
                        void *user_data)
{
  WILNode **queue;
  unsigned char *visited;
  size_t visited_size;
  size_t head;
  size_t tail;

  (void) graph;
  if (start == NULL)
    {
      return;
    }

  visited_size = (size_t) start->context->graph.node_count + 1u;
  queue = (WILNode **) calloc (visited_size, sizeof (*queue));
  visited = (unsigned char *) calloc (visited_size, sizeof (*visited));
  if (queue == NULL || visited == NULL)
    {
      free (queue);
      free (visited);
      return;
    }

  head = 0u;
  tail = 0u;
  queue[tail++] = start;
  visited[start->id] = 1u;

  while (head < tail)
    {
      WILNode *node = queue[head++];
      WILIndex index;

      if (visitor != NULL)
        {
          visitor (node, user_data);
        }

      for (index = 0u; index < node->input_count; ++index)
        {
          WILNode *input = node->inputs[index].node;
          if (input != NULL && visited[input->id] == 0u)
            {
              visited[input->id] = 1u;
              queue[tail++] = input;
            }
        }
    }

  free (queue);
  free (visited);
}

void
wil_graph_traverse_rpo (WILGraph *graph, WILNode *start, WILNodeVisitor visitor,
                        void *user_data)
{
  WILNode **nodes;
  WILIndex count;
  WILIndex index;

  (void) graph;
  if (start == NULL)
    {
      return;
    }

  count = start->context->graph.node_count;
  nodes = (WILNode **) calloc ((size_t) count, sizeof (*nodes));
  if (nodes == NULL)
    {
      free (nodes);
      return;
    }

  /* Collect postorder then replay in reverse order. */
  {
    WILPostorderCollector collector;

    collector.nodes = nodes;
    collector.count = 0u;
    wil_graph_traverse_postorder (graph, start, wil_collect_postorder, &collector);
    index = collector.count;
  }

  while (index > 0u)
    {
      --index;
      if (visitor != NULL)
        {
          visitor (nodes[index], user_data);
        }
    }

  free (nodes);
}

void
wil_context_set_verify_errorf (WILContext *ctx, const char *format, ...)
{
  va_list args;

  if (ctx == NULL || format == NULL)
    {
      return;
    }

  va_start (args, format);
  vsnprintf (ctx->verify_error, sizeof (ctx->verify_error), format, args);
  va_end (args);
}

void
wil_context_clear_verify_error (WILContext *ctx)
{
  if (ctx != NULL)
    {
      ctx->verify_error[0] = '\0';
    }
}

const char *
wil_verify_error (const WILContext *ctx)
{
  return (ctx == NULL || ctx->verify_error[0] == '\0') ? NULL
                                                         : ctx->verify_error;
}

WILCFG *wil_extract_cfg (WILContext *ctx) { (void) ctx; return NULL; }
void wil_cfg_destroy (WILCFG *cfg) { (void) cfg; }
WILIndex wil_cfg_block_count (const WILCFG *cfg) { (void) cfg; return 0u; }
WILBlock *wil_cfg_get_block (const WILCFG *cfg, WILIndex index)
{ (void) cfg; (void) index; return NULL; }
WILNode *wil_cfg_block_region (const WILBlock *block)
{ (void) block; return NULL; }
WILIndex wil_cfg_block_succ_count (const WILBlock *block)
{ (void) block; return 0u; }
WILBlock *wil_cfg_block_succ (const WILBlock *block, WILIndex index)
{ (void) block; (void) index; return NULL; }
WILIndex wil_cfg_block_pred_count (const WILBlock *block)
{ (void) block; return 0u; }
WILBlock *wil_cfg_block_pred (const WILBlock *block, WILIndex index)
{ (void) block; (void) index; return NULL; }
WILDomTree *wil_compute_dominance (const WILCFG *cfg)
{ (void) cfg; return NULL; }
void wil_domtree_destroy (WILDomTree *tree) { (void) tree; }
int wil_dominates (const WILDomTree *tree, const WILBlock *a, const WILBlock *b)
{ (void) tree; (void) a; (void) b; return 0; }
WILBlock *wil_immediate_dominator (const WILDomTree *tree, const WILBlock *block)
{ (void) tree; (void) block; return NULL; }
WILIndex wil_domtree_child_count (const WILDomTree *tree, const WILBlock *block)
{ (void) tree; (void) block; return 0u; }
WILBlock *wil_domtree_child (const WILDomTree *tree, const WILBlock *block,
                             WILIndex index)
{ (void) tree; (void) block; (void) index; return NULL; }
WILIndex wil_dominance_frontier_size (const WILDomTree *tree,
                                      const WILBlock *block)
{ (void) tree; (void) block; return 0u; }
WILBlock *wil_dominance_frontier_block (const WILDomTree *tree,
                                        const WILBlock *block, WILIndex index)
{ (void) tree; (void) block; (void) index; return NULL; }
WILSchedule *wil_compute_schedule (WILContext *ctx, const WILCFG *cfg)
{ (void) ctx; (void) cfg; return NULL; }
void wil_schedule_destroy (WILSchedule *schedule) { (void) schedule; }
WILIndex wil_schedule_block_count (const WILSchedule *schedule)
{ (void) schedule; return 0u; }
WILIndex wil_schedule_node_count (const WILSchedule *schedule,
                                  WILIndex block_index)
{ (void) schedule; (void) block_index; return 0u; }
WILNode *wil_schedule_node (const WILSchedule *schedule, WILIndex block_index,
                            WILIndex node_index)
{ (void) schedule; (void) block_index; (void) node_index; return NULL; }
WILBlock *wil_node_scheduled_block (const WILSchedule *schedule,
                                    const WILNode *node)
{ (void) schedule; (void) node; return NULL; }
void wil_run_dce (WILContext *ctx) { (void) ctx; }
void wil_run_cse (WILContext *ctx) { (void) ctx; }
void wil_run_constant_folding (WILContext *ctx) { (void) ctx; }
void wil_run_sccp (WILContext *ctx) { (void) ctx; }
void wil_run_gvn (WILContext *ctx) { (void) ctx; }
WILGraphViz *wil_graphviz_create (const WILGraph *graph)
{ (void) graph; return NULL; }
void wil_graphviz_destroy (WILGraphViz *viz) { (void) viz; }
void wil_graphviz_set_title (WILGraphViz *viz, const char *title)
{ (void) viz; (void) title; }
void wil_graphviz_show_types (WILGraphViz *viz, int show)
{ (void) viz; (void) show; }
void wil_graphviz_show_ids (WILGraphViz *viz, int show)
{ (void) viz; (void) show; }
void wil_graphviz_show_edge_kinds (WILGraphViz *viz, int show)
{ (void) viz; (void) show; }
void wil_graphviz_highlight_node (WILGraphViz *viz, WILNode *node,
                                  const char *color)
{ (void) viz; (void) node; (void) color; }
void wil_graphviz_write_dot (WILGraphViz *viz, FILE *out)
{ (void) viz; (void) out; }
void wil_graphviz_write_dot_file (WILGraphViz *viz, const char *filename)
{ (void) viz; (void) filename; }
void wil_graphviz_write_tikz (WILGraphViz *viz, FILE *out)
{ (void) viz; (void) out; }
void wil_graphviz_write_tikz_file (WILGraphViz *viz, const char *filename)
{ (void) viz; (void) filename; }
int wil_graph_save (const WILGraph *graph, const char *filename)
{ (void) graph; (void) filename; return 0; }
WILGraph *wil_graph_load (WILContext *ctx, const char *filename)
{ (void) filename; return wil_context_graph (ctx); }
