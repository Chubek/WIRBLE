#ifndef WIRBLE_WIL_H
#define WIRBLE_WIL_H

#include "api-boilerplate.h"
#include <stdio.h>

WIRBLE_BEGIN_DECLS

/* ============================================================
   WIRBLE Intermediate Language (WIL)
   
   A minimal, fully-featured graph-based IR designed for:
   - Direct AST-to-WIL lowering (no CFG required)
   - Sea-of-Nodes style data/control flow representation
   - Pattern-based rewriting via WRS
   - Optional CFG extraction for advanced analyses
   
   Philosophy:
   - Nodes represent computations and control flow
   - Edges represent dependencies (data or control)
   - No SSA required for basic usage
   - CFG/dominance/scheduling are opt-in features
   ============================================================ */

/* ------------------------------------------------------------
   Basic Types
   ------------------------------------------------------------ */

typedef unsigned long WILNodeId;
typedef unsigned long WILIndex;

/* Node categories */
typedef enum {
    WIL_CAT_VALUE,      /* Produces a value (arithmetic, memory ops, etc.) */
    WIL_CAT_CONTROL,    /* Control flow (if, loop, region, etc.) */
    WIL_CAT_EFFECT,     /* Side effects (store, call, etc.) */
    WIL_CAT_CONSTANT,   /* Compile-time constants */
    WIL_CAT_PARAMETER   /* Function parameters */
} WILNodeCategory;

/* Specific node kinds */
typedef enum {
    /* Constants */
    WIL_NODE_CONST_INT,
    WIL_NODE_CONST_FLOAT,
    WIL_NODE_CONST_BOOL,
    WIL_NODE_UNDEF,
    
    /* Parameters */
    WIL_NODE_PARAM,
    
    /* Arithmetic (binary) */
    WIL_NODE_ADD,
    WIL_NODE_SUB,
    WIL_NODE_MUL,
    WIL_NODE_DIV,
    WIL_NODE_MOD,
    WIL_NODE_AND,
    WIL_NODE_OR,
    WIL_NODE_XOR,
    WIL_NODE_SHL,
    WIL_NODE_SHR,
    
    /* Arithmetic (unary) */
    WIL_NODE_NEG,
    WIL_NODE_NOT,
    WIL_NODE_ABS,
    
    /* Comparison */
    WIL_NODE_EQ,
    WIL_NODE_NE,
    WIL_NODE_LT,
    WIL_NODE_LE,
    WIL_NODE_GT,
    WIL_NODE_GE,
    
    /* Memory */
    WIL_NODE_LOAD,
    WIL_NODE_STORE,
    WIL_NODE_ALLOCA,
    
    /* Control flow */
    WIL_NODE_START,     /* Entry point of function */
    WIL_NODE_REGION,    /* Merge point (like basic block header) */
    WIL_NODE_IF,        /* Conditional branch */
    WIL_NODE_LOOP,      /* Loop header */
    WIL_NODE_RETURN,    /* Function return */
    WIL_NODE_JUMP,      /* Unconditional jump */
    WIL_NODE_PROJ,      /* Projection (extract control path from If/Loop) */
    
    /* Data flow */
    WIL_NODE_PHI,       /* Merge values from multiple control paths */
    
    /* Function calls */
    WIL_NODE_CALL,
    WIL_NODE_CALL_INDIRECT,
    
    /* Type operations */
    WIL_NODE_CAST,
    WIL_NODE_BITCAST,
    
    /* Aggregate operations */
    WIL_NODE_EXTRACT,   /* Extract element from aggregate */
    WIL_NODE_INSERT,    /* Insert element into aggregate */
    
    /* Special */
    WIL_NODE_SELECT     /* Ternary select: select(cond, true_val, false_val) */
} WILNodeKind;

/* Value types */
typedef enum {
    WIL_TYPE_VOID,
    WIL_TYPE_I1,        /* Boolean */
    WIL_TYPE_I8,
    WIL_TYPE_I16,
    WIL_TYPE_I32,
    WIL_TYPE_I64,
    WIL_TYPE_F32,
    WIL_TYPE_F64,
    WIL_TYPE_PTR,       /* Pointer */
    WIL_TYPE_CONTROL    /* Control token */
} WILType;

/* Edge types (for explicit dependency tracking) */
typedef enum {
    WIL_EDGE_DATA,      /* Data dependency */
    WIL_EDGE_CONTROL,   /* Control dependency */
    WIL_EDGE_EFFECT     /* Memory/side-effect dependency */
} WILEdgeKind;

/* ------------------------------------------------------------
   Opaque Handles
   ------------------------------------------------------------ */

typedef struct WILContext WILContext;
typedef struct WILNode WILNode;
typedef struct WILGraph WILGraph;
typedef struct WILArena WILArena;
typedef struct WILEdge WILEdge;

/* Optional CFG structures (only needed for advanced analyses) */
typedef struct WILCFG WILCFG;
typedef struct WILBlock WILBlock;
typedef struct WILDomTree WILDomTree;
typedef struct WILSchedule WILSchedule;

/* Visualization */
typedef struct WILGraphViz WILGraphViz;

/* ------------------------------------------------------------
   Context and Graph Management
   ------------------------------------------------------------ */

WILContext* wil_context_create(void);
void        wil_context_destroy(WILContext* ctx);

/* Get the main graph from context */
WILGraph* wil_context_graph(WILContext* ctx);

/* Graph operations */
WILIndex wil_graph_node_count(const WILGraph* graph);
WILNode* wil_graph_get_node(const WILGraph* graph, WILIndex index);
WILNode* wil_graph_find_node(const WILGraph* graph, WILNodeId id);

/* Iterate all nodes */
WILNode* wil_graph_first_node(const WILGraph* graph);
WILNode* wil_graph_next_node(const WILGraph* graph, const WILNode* node);

/* ------------------------------------------------------------
   Memory Management: Arena Allocator
   ------------------------------------------------------------ */

WILArena* wil_arena_create(void);
void      wil_arena_destroy(WILArena* arena);
void*     wil_arena_alloc(WILArena* arena, unsigned long size);
void      wil_arena_reset(WILArena* arena);

/* Attach arena to context for automatic node allocation */
void wil_context_set_arena(WILContext* ctx, WILArena* arena);

/* ------------------------------------------------------------
   Node Construction (Direct AST Lowering)
   ------------------------------------------------------------ */

/* Constants */
WILNode* wil_make_const_int(WILContext* ctx, long long value, WILType type);
WILNode* wil_make_const_float(WILContext* ctx, double value, WILType type);
WILNode* wil_make_const_bool(WILContext* ctx, int value);
WILNode* wil_make_undef(WILContext* ctx, WILType type);

/* Parameters */
WILNode* wil_make_param(WILContext* ctx, WILIndex index, WILType type, const char* name);

/* Binary arithmetic */
WILNode* wil_make_add(WILContext* ctx, WILNode* lhs, WILNode* rhs);
WILNode* wil_make_sub(WILContext* ctx, WILNode* lhs, WILNode* rhs);
WILNode* wil_make_mul(WILContext* ctx, WILNode* lhs, WILNode* rhs);
WILNode* wil_make_div(WILContext* ctx, WILNode* lhs, WILNode* rhs);
WILNode* wil_make_mod(WILContext* ctx, WILNode* lhs, WILNode* rhs);
WILNode* wil_make_and(WILContext* ctx, WILNode* lhs, WILNode* rhs);
WILNode* wil_make_or(WILContext* ctx, WILNode* lhs, WILNode* rhs);
WILNode* wil_make_xor(WILContext* ctx, WILNode* lhs, WILNode* rhs);
WILNode* wil_make_shl(WILContext* ctx, WILNode* lhs, WILNode* rhs);
WILNode* wil_make_shr(WILContext* ctx, WILNode* lhs, WILNode* rhs);

/* Unary arithmetic */
WILNode* wil_make_neg(WILContext* ctx, WILNode* operand);
WILNode* wil_make_not(WILContext* ctx, WILNode* operand);
WILNode* wil_make_abs(WILContext* ctx, WILNode* operand);

/* Comparison */
WILNode* wil_make_eq(WILContext* ctx, WILNode* lhs, WILNode* rhs);
WILNode* wil_make_ne(WILContext* ctx, WILNode* lhs, WILNode* rhs);
WILNode* wil_make_lt(WILContext* ctx, WILNode* lhs, WILNode* rhs);
WILNode* wil_make_le(WILContext* ctx, WILNode* lhs, WILNode* rhs);
WILNode* wil_make_gt(WILContext* ctx, WILNode* lhs, WILNode* rhs);
WILNode* wil_make_ge(WILContext* ctx, WILNode* lhs, WILNode* rhs);

/* Memory operations */
WILNode* wil_make_load(WILContext* ctx, WILNode* address, WILNode* effect, WILType type);
WILNode* wil_make_store(WILContext* ctx, WILNode* address, WILNode* value, WILNode* effect);
WILNode* wil_make_alloca(WILContext* ctx, WILType type, WILNode* size);

/* Control flow (simple, no CFG required) */
WILNode* wil_make_start(WILContext* ctx);
WILNode* wil_make_return(WILContext* ctx, WILNode* control, WILNode* value);
WILNode* wil_make_jump(WILContext* ctx, WILNode* control, WILNode* target);

/* Control flow (advanced, for CFG-based lowering) */
WILNode* wil_make_region(WILContext* ctx, WILNode** controls, WILIndex count);
WILNode* wil_make_if(WILContext* ctx, WILNode* control, WILNode* condition);
WILNode* wil_make_loop(WILContext* ctx, WILNode* entry_control);
WILNode* wil_make_proj(WILContext* ctx, WILNode* control, WILIndex index);
WILNode* wil_make_phi(WILContext* ctx, WILNode* region, WILNode** values, WILIndex count);

/* Function calls */
WILNode* wil_make_call(WILContext* ctx, WILNode* control, WILNode* effect, 
                       const char* callee, WILNode** args, WILIndex arg_count);
WILNode* wil_make_call_indirect(WILContext* ctx, WILNode* control, WILNode* effect,
                                WILNode* callee, WILNode** args, WILIndex arg_count);

/* Type operations */
WILNode* wil_make_cast(WILContext* ctx, WILNode* value, WILType target_type);
WILNode* wil_make_bitcast(WILContext* ctx, WILNode* value, WILType target_type);

/* Aggregate operations */
WILNode* wil_make_extract(WILContext* ctx, WILNode* aggregate, WILIndex index);
WILNode* wil_make_insert(WILContext* ctx, WILNode* aggregate, WILNode* value, WILIndex index);

/* Special */
WILNode* wil_make_select(WILContext* ctx, WILNode* condition, WILNode* true_val, WILNode* false_val);

/* ------------------------------------------------------------
   Node Inspection
   ------------------------------------------------------------ */

WILNodeId       wil_node_id(const WILNode* node);
WILNodeKind     wil_node_kind(const WILNode* node);
WILNodeCategory wil_node_category(const WILNode* node);
WILType         wil_node_type(const WILNode* node);
const char*     wil_node_name(const WILNode* node);

/* Get inputs (dependencies) */
WILIndex wil_node_input_count(const WILNode* node);
WILNode* wil_node_input(const WILNode* node, WILIndex index);
WILEdgeKind wil_node_input_edge_kind(const WILNode* node, WILIndex index);

/* Get uses (nodes that depend on this node) */
WILIndex wil_node_use_count(const WILNode* node);
WILNode* wil_node_use(const WILNode* node, WILIndex index);

/* Constant value extraction */
int wil_node_is_const(const WILNode* node);
long long wil_node_const_int(const WILNode* node);
double wil_node_const_float(const WILNode* node);
int wil_node_const_bool(const WILNode* node);

/* ------------------------------------------------------------
   Node Mutation (for optimization passes)
   ------------------------------------------------------------ */

void wil_node_add_input(WILNode* node, WILNode* input, WILEdgeKind edge_kind);
void wil_node_replace_input(WILNode* node, WILIndex index, WILNode* new_input);
void wil_node_remove_input(WILNode* node, WILIndex index);

/* Replace all uses of old_node with new_node */
void wil_node_replace_all_uses(WILNode* old_node, WILNode* new_node);

/* Remove node from graph (must have no uses) */
void wil_node_remove(WILNode* node);

/* ------------------------------------------------------------
   Graph Traversal Utilities
   ------------------------------------------------------------ */

/* Depth-first traversal */
typedef void (*WILNodeVisitor)(WILNode* node, void* user_data);
void wil_graph_traverse_dfs(WILGraph* graph, WILNode* start, WILNodeVisitor visitor, void* user_data);

/* Breadth-first traversal */
void wil_graph_traverse_bfs(WILGraph* graph, WILNode* start, WILNodeVisitor visitor, void* user_data);

/* Post-order traversal (children before parent) */
void wil_graph_traverse_postorder(WILGraph* graph, WILNode* start, WILNodeVisitor visitor, void* user_data);

/* Reverse post-order traversal */
void wil_graph_traverse_rpo(WILGraph* graph, WILNode* start, WILNodeVisitor visitor, void* user_data);

/* ------------------------------------------------------------
   Optional: CFG Extraction (for advanced analyses)
   ------------------------------------------------------------ */

/* Extract CFG from control-flow nodes (Region, If, Loop, etc.) */
WILCFG* wil_extract_cfg(WILContext* ctx);
void    wil_cfg_destroy(WILCFG* cfg);

/* Query CFG */
WILIndex wil_cfg_block_count(const WILCFG* cfg);
WILBlock* wil_cfg_get_block(const WILCFG* cfg, WILIndex index);
WILNode* wil_cfg_block_region(const WILBlock* block);

/* Block successors/predecessors */
WILIndex wil_cfg_block_succ_count(const WILBlock* block);
WILBlock* wil_cfg_block_succ(const WILBlock* block, WILIndex index);
WILIndex wil_cfg_block_pred_count(const WILBlock* block);
WILBlock* wil_cfg_block_pred(const WILBlock* block, WILIndex index);

/* ------------------------------------------------------------
   Optional: Dominance Analysis (requires CFG)
   ------------------------------------------------------------ */

WILDomTree* wil_compute_dominance(const WILCFG* cfg);
void        wil_domtree_destroy(WILDomTree* tree);

/* Query dominance */
int wil_dominates(const WILDomTree* tree, const WILBlock* a, const WILBlock* b);
WILBlock* wil_immediate_dominator(const WILDomTree* tree, const WILBlock* block);

/* Dominator tree children */
WILIndex wil_domtree_child_count(const WILDomTree* tree, const WILBlock* block);
WILBlock* wil_domtree_child(const WILDomTree* tree, const WILBlock* block, WILIndex index);

/* Dominance frontier */
WILIndex wil_dominance_frontier_size(const WILDomTree* tree, const WILBlock* block);
WILBlock* wil_dominance_frontier_block(const WILDomTree* tree, const WILBlock* block, WILIndex index);

/* ------------------------------------------------------------
   Optional: Scheduling (requires CFG)
   ------------------------------------------------------------ */

/* Compute schedule: assigns nodes to blocks and orders them */
WILSchedule* wil_compute_schedule(WILContext* ctx, const WILCFG* cfg);
void         wil_schedule_destroy(WILSchedule* schedule);

/* Query schedule */
WILIndex wil_schedule_block_count(const WILSchedule* schedule);
WILIndex wil_schedule_node_count(const WILSchedule* schedule, WILIndex block_index);
WILNode* wil_schedule_node(const WILSchedule* schedule, WILIndex block_index, WILIndex node_index);

/* Get block assignment for a node */
WILBlock* wil_node_scheduled_block(const WILSchedule* schedule, const WILNode* node);

/* ------------------------------------------------------------
   Optimization Passes
   ------------------------------------------------------------ */

/* Dead code elimination */
void wil_run_dce(WILContext* ctx);

/* Common subexpression elimination */
void wil_run_cse(WILContext* ctx);

/* Constant folding */
void wil_run_constant_folding(WILContext* ctx);

/* Sparse conditional constant propagation */
void wil_run_sccp(WILContext* ctx);

/* Global value numbering */
void wil_run_gvn(WILContext* ctx);

/* ------------------------------------------------------------
   Verification
   ------------------------------------------------------------ */

/* Verify graph integrity (returns 1 if valid, 0 otherwise) */
int wil_verify_graph(const WILContext* ctx);

/* Get verification error message (if any) */
const char* wil_verify_error(const WILContext* ctx);

/* ------------------------------------------------------------
   Visualization
   ------------------------------------------------------------ */

/* GraphViz DOT output */
WILGraphViz* wil_graphviz_create(const WILGraph* graph);
void         wil_graphviz_destroy(WILGraphViz* viz);

/* Configure visualization */
void wil_graphviz_set_title(WILGraphViz* viz, const char* title);
void wil_graphviz_show_types(WILGraphViz* viz, int show);
void wil_graphviz_show_ids(WILGraphViz* viz, int show);
void wil_graphviz_show_edge_kinds(WILGraphViz* viz, int show);
void wil_graphviz_highlight_node(WILGraphViz* viz, WILNode* node, const char* color);

/* Export to DOT format */
void wil_graphviz_write_dot(WILGraphViz* viz, FILE* out);
void wil_graphviz_write_dot_file(WILGraphViz* viz, const char* filename);

/* TikZ output (for LaTeX) */
void wil_graphviz_write_tikz(WILGraphViz* viz, FILE* out);
void wil_graphviz_write_tikz_file(WILGraphViz* viz, const char* filename);

/* ------------------------------------------------------------
   Serialization (for .wil files)
   ------------------------------------------------------------ */

/* Save graph to file */
int wil_graph_save(const WILGraph* graph, const char* filename);

/* Load graph from file */
WILGraph* wil_graph_load(WILContext* ctx, const char* filename);

/* ------------------------------------------------------------
   Debug Printing
   ------------------------------------------------------------ */

void wil_print_node(const WILNode* node, FILE* out);
void wil_print_graph(const WILGraph* graph, FILE* out);
void wil_print_cfg(const WILCFG* cfg, FILE* out);
void wil_print_schedule(const WILSchedule* schedule, FILE* out);
void wil_print_domtree(const WILDomTree* tree, FILE* out);

/* Print node kind as string */
const char* wil_node_kind_str(WILNodeKind kind);
const char* wil_type_str(WILType type);
const char* wil_edge_kind_str(WILEdgeKind kind);

WIRBLE_END_DECLS

#endif /* WIRBLE_WIL_H */
