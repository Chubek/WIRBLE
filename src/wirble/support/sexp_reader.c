#include "support/sexp_reader.h"

#include <stdarg.h>
#include <stddef.h>
#include <string.h>

#include "sexp.h"

static void
wirble_sexp_report (WirbleSexpTree *tree, WirbleDiagSeverity severity,
                    const char *format, ...)
{
  va_list args;

  if (tree == NULL || tree->diag == NULL)
    {
      return;
    }

  va_start (args, format);
  wirble_diag_vreportf (tree->diag, severity, NULL, format, args);
  va_end (args);
}

static const char *
wirble_sexp_safe_source (const char *source_name)
{
  if (source_name == NULL || source_name[0] == '\0')
    {
      return "<sexp>";
    }

  return source_name;
}

static WirbleSexpNode *
wirble_sexp_alloc_node (WirbleSexpTree *tree)
{
  return (WirbleSexpNode *) wirble_arena_calloc_from (
      &tree->arena, 1u, sizeof (WirbleSexpNode), _Alignof (WirbleSexpNode));
}

static char *
wirble_sexp_copy_bytes (WirbleSexpTree *tree, const char *data, size_t size)
{
  char *copy;

  copy = (char *) wirble_arena_alloc_from (&tree->arena, size + 1u,
                                           sizeof (char));
  if (copy == NULL)
    {
      return NULL;
    }

  if (size != 0u && data != NULL)
    {
      memcpy (copy, data, size);
    }
  copy[size] = '\0';
  return copy;
}

static WirbleSexpAtomKind
wirble_sexp_atom_kind_from_sfsexp (atom_t atom_kind)
{
  switch (atom_kind)
    {
    case SEXP_SQUOTE:
      return WIRBLE_SEXP_ATOM_SINGLE_QUOTED;
    case SEXP_DQUOTE:
      return WIRBLE_SEXP_ATOM_DOUBLE_QUOTED;
    case SEXP_BINARY:
      return WIRBLE_SEXP_ATOM_BINARY;
    case SEXP_BASIC:
    default:
      return WIRBLE_SEXP_ATOM_BARE;
    }
}

static WirbleSexpNode *
wirble_sexp_build_from_sfsexp (WirbleSexpTree *tree, const sexp_t *input,
                               WirbleSexpNode *parent)
{
  WirbleSexpNode *head;
  WirbleSexpNode *tail;
  const sexp_t *cursor;

  head = NULL;
  tail = NULL;
  for (cursor = input; cursor != NULL; cursor = cursor->next)
    {
      WirbleSexpNode *node;

      node = wirble_sexp_alloc_node (tree);
      if (node == NULL)
        {
          return NULL;
        }

      node->parent = parent;
      if (cursor->ty == SEXP_LIST)
        {
          node->kind = WIRBLE_SEXP_NODE_LIST;
          node->first_child
              = wirble_sexp_build_from_sfsexp (tree, cursor->list, node);
          if (cursor->list != NULL && node->first_child == NULL)
            {
              return NULL;
            }
        }
      else
        {
          size_t text_length;
          const char *text_data;

          node->kind = WIRBLE_SEXP_NODE_ATOM;
          node->atom_kind = wirble_sexp_atom_kind_from_sfsexp (cursor->aty);
          if (cursor->aty == SEXP_BINARY)
            {
              text_data = cursor->bindata;
              text_length = cursor->binlength;
            }
          else
            {
              text_data = cursor->val;
              text_length = cursor->val == NULL ? 0u : strlen (cursor->val);
            }

          node->text = wirble_sexp_copy_bytes (tree, text_data, text_length);
          if (node->text == NULL)
            {
              return NULL;
            }
          node->text_length = text_length;
        }

      if (head == NULL)
        {
          head = node;
        }
      else
        {
          tail->next_sibling = node;
        }
      tail = node;

      if (parent != NULL)
        {
          ++parent->child_count;
        }
    }

  return head;
}

static int
wirble_sexp_wrap_input (WirbleSexpTree *tree, const char *text, size_t text_size)
{
  static const char prefix[] = "(\n";
  static const char suffix[] = "\n)\n";

  wirble_bytebuf_clear (&tree->storage);

  if (!wirble_bytebuf_append (&tree->storage, prefix, sizeof (prefix) - 1u))
    {
      return 0;
    }

  if (text != NULL && text_size != 0u
      && !wirble_bytebuf_append (&tree->storage, text, text_size))
    {
      return 0;
    }

  if (!wirble_bytebuf_append (&tree->storage, suffix, sizeof (suffix) - 1u))
    {
      return 0;
    }

  return wirble_bytebuf_append_byte (&tree->storage, 0u);
}

static int
wirble_sexp_parse_buffer (WirbleSexpTree *tree)
{
  sexp_t *parsed;
  WirbleSexpNode *root;

  if (tree == NULL)
    {
      return 0;
    }

  parsed = parse_sexp ((char *) tree->storage.data,
                       tree->storage.size == 0u ? 0u : tree->storage.size - 1u);
  if (parsed == NULL)
    {
      wirble_sexp_report (tree, WIRBLE_DIAG_ERROR,
                          "failed to parse s-expression input from %s "
                          "(sfsexp err=%d)",
                          wirble_sexp_safe_source (tree->source_name),
                          (int) sexp_errno);
      sexp_cleanup ();
      return 0;
    }

  root = wirble_sexp_alloc_node (tree);
  if (root == NULL)
    {
      destroy_sexp (parsed);
      sexp_cleanup ();
      return 0;
    }

  root->kind = WIRBLE_SEXP_NODE_LIST;
  root->first_child = wirble_sexp_build_from_sfsexp (tree, parsed->list, root);
  if (parsed->list != NULL && root->first_child == NULL)
    {
      destroy_sexp (parsed);
      sexp_cleanup ();
      wirble_sexp_report (tree, WIRBLE_DIAG_ERROR,
                          "out of memory while materializing %s",
                          wirble_sexp_safe_source (tree->source_name));
      return 0;
    }

  tree->root = root;
  tree->form_count = root->child_count;

  destroy_sexp (parsed);
  sexp_cleanup ();
  return 1;
}

void
wirble_sexp_tree_init (WirbleSexpTree *tree, WirbleDiagnostics *diag)
{
  if (tree == NULL)
    {
      return;
    }

  wirble_arena_init (&tree->arena, 4096u);
  wirble_bytebuf_init (&tree->storage);
  tree->diag = diag;
  tree->source_name = NULL;
  tree->root = NULL;
  tree->form_count = 0u;
}

void
wirble_sexp_tree_reset (WirbleSexpTree *tree)
{
  if (tree == NULL)
    {
      return;
    }

  wirble_arena_reset (&tree->arena);
  wirble_bytebuf_clear (&tree->storage);
  tree->source_name = NULL;
  tree->root = NULL;
  tree->form_count = 0u;
}

void
wirble_sexp_tree_destroy (WirbleSexpTree *tree)
{
  if (tree == NULL)
    {
      return;
    }

  wirble_bytebuf_destroy (&tree->storage);
  wirble_arena_destroy (&tree->arena);
  tree->source_name = NULL;
  tree->root = NULL;
  tree->form_count = 0u;
}

int
wirble_sexp_tree_parse_file (WirbleSexpTree *tree, const char *path)
{
  WirbleByteBuf file_data;
  int ok;

  if (tree == NULL || path == NULL)
    {
      return 0;
    }

  wirble_sexp_tree_reset (tree);
  tree->source_name = path;

  wirble_bytebuf_init (&file_data);
  ok = wirble_bytebuf_read_file (&file_data, path);
  if (!ok)
    {
      wirble_bytebuf_destroy (&file_data);
      wirble_sexp_report (tree, WIRBLE_DIAG_ERROR,
                          "failed to read rewrite rules from %s",
                          wirble_sexp_safe_source (path));
      return 0;
    }

  ok = wirble_sexp_wrap_input (tree, (const char *) file_data.data, file_data.size);
  wirble_bytebuf_destroy (&file_data);
  if (!ok)
    {
      wirble_sexp_report (tree, WIRBLE_DIAG_ERROR,
                          "out of memory while buffering %s",
                          wirble_sexp_safe_source (path));
      return 0;
    }

  return wirble_sexp_parse_buffer (tree);
}

int
wirble_sexp_tree_parse_cstr (WirbleSexpTree *tree, const char *source_name,
                             const char *text)
{
  if (tree == NULL || text == NULL)
    {
      return 0;
    }

  wirble_sexp_tree_reset (tree);
  tree->source_name = wirble_sexp_safe_source (source_name);

  if (!wirble_sexp_wrap_input (tree, text, strlen (text)))
    {
      wirble_sexp_report (tree, WIRBLE_DIAG_ERROR,
                          "out of memory while buffering %s",
                          tree->source_name);
      return 0;
    }

  return wirble_sexp_parse_buffer (tree);
}

const WirbleSexpNode *
wirble_sexp_tree_root (const WirbleSexpTree *tree)
{
  return tree == NULL ? NULL : tree->root;
}

size_t
wirble_sexp_tree_form_count (const WirbleSexpTree *tree)
{
  return tree == NULL ? 0u : tree->form_count;
}

int
wirble_sexp_node_is_atom (const WirbleSexpNode *node)
{
  return node != NULL && node->kind == WIRBLE_SEXP_NODE_ATOM;
}

int
wirble_sexp_node_is_list (const WirbleSexpNode *node)
{
  return node != NULL && node->kind == WIRBLE_SEXP_NODE_LIST;
}

int
wirble_sexp_atom_equals (const WirbleSexpNode *node, const char *text)
{
  if (!wirble_sexp_node_is_atom (node) || text == NULL)
    {
      return 0;
    }

  return strcmp (node->text, text) == 0;
}

const WirbleSexpNode *
wirble_sexp_node_child (const WirbleSexpNode *node, size_t index)
{
  const WirbleSexpNode *child;
  size_t cursor;

  if (!wirble_sexp_node_is_list (node))
    {
      return NULL;
    }

  child = node->first_child;
  for (cursor = 0u; child != NULL && cursor < index; ++cursor)
    {
      child = child->next_sibling;
    }

  return child;
}

const WirbleSexpNode *
wirble_sexp_node_next (const WirbleSexpNode *node)
{
  return node == NULL ? NULL : node->next_sibling;
}

size_t
wirble_sexp_node_child_count (const WirbleSexpNode *node)
{
  return wirble_sexp_node_is_list (node) ? node->child_count : 0u;
}

const WirbleSexpNode *
wirble_sexp_list_find_keyword (const WirbleSexpNode *node, const char *keyword)
{
  const WirbleSexpNode *child;

  if (!wirble_sexp_node_is_list (node) || keyword == NULL)
    {
      return NULL;
    }

  child = node->first_child;
  if (child != NULL)
    {
      child = child->next_sibling;
    }

  while (child != NULL)
    {
      if (wirble_sexp_atom_equals (child, keyword))
        {
          return child->next_sibling;
        }
      child = child->next_sibling;
    }

  return NULL;
}

int
wirble_sexp_extract_rule (const WirbleSexpNode *form, WirbleSexpRuleView *view)
{
  if (!wirble_sexp_node_is_list (form) || view == NULL)
    {
      return 0;
    }

  if (!wirble_sexp_atom_equals (form->first_child, "rule"))
    {
      return 0;
    }

  view->form = form;
  view->name = wirble_sexp_list_find_keyword (form, ":name");
  view->match = wirble_sexp_list_find_keyword (form, ":match");
  view->replace = wirble_sexp_list_find_keyword (form, ":replace");
  view->condition = wirble_sexp_list_find_keyword (form, ":when");

  return view->match != NULL && view->replace != NULL;
}
