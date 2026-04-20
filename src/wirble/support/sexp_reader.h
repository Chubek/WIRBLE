#ifndef WIRBLE_SUPPORT_SEXP_READER_H
#define WIRBLE_SUPPORT_SEXP_READER_H

#include <stddef.h>

#include "common/arena.h"
#include "common/bytebuf.h"
#include "common/diagnostics.h"

typedef enum WirbleSexpNodeKind
{
  WIRBLE_SEXP_NODE_ATOM = 0,
  WIRBLE_SEXP_NODE_LIST
} WirbleSexpNodeKind;

typedef enum WirbleSexpAtomKind
{
  WIRBLE_SEXP_ATOM_BARE = 0,
  WIRBLE_SEXP_ATOM_SINGLE_QUOTED,
  WIRBLE_SEXP_ATOM_DOUBLE_QUOTED,
  WIRBLE_SEXP_ATOM_BINARY
} WirbleSexpAtomKind;

typedef struct WirbleSexpNode
{
  WirbleSexpNodeKind kind;
  WirbleSexpAtomKind atom_kind;
  const char *text;
  size_t text_length;
  size_t child_count;
  struct WirbleSexpNode *parent;
  struct WirbleSexpNode *first_child;
  struct WirbleSexpNode *next_sibling;
} WirbleSexpNode;

typedef struct WirbleSexpTree
{
  WirbleArena arena;
  WirbleByteBuf storage;
  WirbleDiagnostics *diag;
  const char *source_name;
  WirbleSexpNode *root;
  size_t form_count;
} WirbleSexpTree;

typedef struct WirbleSexpRuleView
{
  const WirbleSexpNode *form;
  const WirbleSexpNode *name;
  const WirbleSexpNode *match;
  const WirbleSexpNode *replace;
  const WirbleSexpNode *condition;
} WirbleSexpRuleView;

void wirble_sexp_tree_init (WirbleSexpTree *tree, WirbleDiagnostics *diag);
void wirble_sexp_tree_reset (WirbleSexpTree *tree);
void wirble_sexp_tree_destroy (WirbleSexpTree *tree);

int wirble_sexp_tree_parse_file (WirbleSexpTree *tree, const char *path);
int wirble_sexp_tree_parse_cstr (WirbleSexpTree *tree, const char *source_name,
                                 const char *text);

const WirbleSexpNode *wirble_sexp_tree_root (const WirbleSexpTree *tree);
size_t wirble_sexp_tree_form_count (const WirbleSexpTree *tree);

int wirble_sexp_node_is_atom (const WirbleSexpNode *node);
int wirble_sexp_node_is_list (const WirbleSexpNode *node);
int wirble_sexp_atom_equals (const WirbleSexpNode *node, const char *text);
const WirbleSexpNode *wirble_sexp_node_child (const WirbleSexpNode *node,
                                              size_t index);
const WirbleSexpNode *wirble_sexp_node_next (const WirbleSexpNode *node);
size_t wirble_sexp_node_child_count (const WirbleSexpNode *node);
const WirbleSexpNode *wirble_sexp_list_find_keyword (const WirbleSexpNode *node,
                                                     const char *keyword);
int wirble_sexp_extract_rule (const WirbleSexpNode *form,
                              WirbleSexpRuleView *view);

#endif /* WIRBLE_SUPPORT_SEXP_READER_H */
