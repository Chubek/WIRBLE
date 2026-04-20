#ifndef WIRBLE_WIL_NODES_H
#define WIRBLE_WIL_NODES_H

#include <stddef.h>

#include "common/arena.h"
#include "common/diagnostics.h"
#include "wirble/wirble-wil.h"

typedef struct WILInputSlot
{
  WILNode *node;
  WILEdgeKind edge_kind;
} WILInputSlot;

typedef union WILNodePayload
{
  long long int_value;
  double float_value;
  int bool_value;
  WILIndex index_value;
  const char *string_value;
} WILNodePayload;

struct WILNode
{
  WILContext *context;
  WILNodeId id;
  WILNodeKind kind;
  WILNodeCategory category;
  WILType type;
  const char *name;
  WILInputSlot *inputs;
  WILIndex input_count;
  WILIndex input_capacity;
  WILNode **uses;
  WILIndex use_count;
  WILIndex use_capacity;
  WILNodePayload payload;
  unsigned int flags;
};

struct WILGraph
{
  WILNode **nodes;
  WILIndex node_count;
  WILIndex node_capacity;
};

struct WILArena
{
  WirbleArena arena;
};

struct WILContext
{
  WILGraph graph;
  WILArena *attached_arena;
  int owns_attached_arena;
  char verify_error[256];
  const char *label;
};

void *wil_context_alloc (WILContext *ctx, size_t size, size_t alignment);
const char *wil_context_strdup (WILContext *ctx, const char *text);
WILNodeCategory wil_node_kind_category (WILNodeKind kind);
WILType wil_node_kind_default_type (WILNodeKind kind, WILType hint,
                                    WILNode *lhs, WILNode *rhs);
WILNode *wil_node_create (WILContext *ctx, WILNodeKind kind, WILType type,
                          const char *name);
int wil_graph_append_node (WILGraph *graph, WILNode *node);
int wil_node_ensure_input_capacity (WILNode *node, WILIndex capacity);
int wil_node_ensure_use_capacity (WILNode *node, WILIndex capacity);
void wil_node_register_use (WILNode *input, WILNode *user);
void wil_node_unregister_use (WILNode *input, WILNode *user);
void wil_context_set_verify_errorf (WILContext *ctx, const char *format, ...);
void wil_context_clear_verify_error (WILContext *ctx);

#endif /* WIRBLE_WIL_NODES_H */
