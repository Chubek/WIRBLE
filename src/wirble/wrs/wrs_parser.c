#include "wrs_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static WRSSExpr *
wrs_alloc_expr (WILContext *ctx)
{
  return (WRSSExpr *) wil_context_alloc (ctx, sizeof (WRSSExpr),
                                         _Alignof (WRSSExpr));
}

static int
wrs_is_integer (const char *text)
{
  size_t index;

  if (text == NULL || text[0] == '\0')
    {
      return 0;
    }

  index = (text[0] == '-' || text[0] == '+') ? 1u : 0u;
  if (text[index] == '\0')
    {
      return 0;
    }

  for (; text[index] != '\0'; ++index)
    {
      if (text[index] < '0' || text[index] > '9')
        {
          return 0;
        }
    }
  return 1;
}

static int
wrs_is_float (const char *text)
{
  size_t index;
  int seen_dot;

  if (text == NULL || text[0] == '\0')
    {
      return 0;
    }

  seen_dot = 0;
  index = (text[0] == '-' || text[0] == '+') ? 1u : 0u;
  if (text[index] == '\0')
    {
      return 0;
    }

  for (; text[index] != '\0'; ++index)
    {
      if (text[index] == '.')
        {
          if (seen_dot)
            {
              return 0;
            }
          seen_dot = 1;
          continue;
        }
      if (text[index] < '0' || text[index] > '9')
        {
          return 0;
        }
    }
  return seen_dot;
}

WRSSExpr *
wrs_sexpr_from_tree (WILContext *ctx, const WirbleSexpNode *node)
{
  WRSSExpr *expr;
  uint32_t index;
  const WirbleSexpNode *child;

  if (node == NULL)
    {
      return NULL;
    }

  expr = wrs_alloc_expr (ctx);
  if (expr == NULL)
    {
      return NULL;
    }

  if (wirble_sexp_node_is_atom (node))
    {
      if (node->atom_kind == WIRBLE_SEXP_ATOM_DOUBLE_QUOTED)
        {
          expr->kind = WRS_SEXPR_STRING;
          expr->atom = (char *) wil_context_strdup (ctx, node->text);
        }
      else if (wrs_is_integer (node->text))
        {
          expr->kind = WRS_SEXPR_NUMBER;
          expr->intVal = strtoll (node->text, NULL, 10);
        }
      else if (wrs_is_float (node->text))
        {
          expr->kind = WRS_SEXPR_NUMBER;
          expr->floatVal = strtod (node->text, NULL);
        }
      else
        {
          expr->kind = WRS_SEXPR_ATOM;
          expr->atom = (char *) wil_context_strdup (ctx, node->text);
        }
      return expr;
    }

  expr->kind = WRS_SEXPR_LIST;
  expr->list.count = (uint32_t) wirble_sexp_node_child_count (node);
  if (expr->list.count == 0u)
    {
      return expr;
    }

  expr->list.elements = (WRSSExpr **) wil_context_alloc (
      ctx, (size_t) expr->list.count * sizeof (*expr->list.elements),
      _Alignof (WRSSExpr *));
  if (expr->list.elements == NULL)
    {
      return NULL;
    }

  child = wirble_sexp_node_child (node, 0u);
  for (index = 0u; index < expr->list.count; ++index)
    {
      expr->list.elements[index] = wrs_sexpr_from_tree (ctx, child);
      child = wirble_sexp_node_next (child);
    }
  return expr;
}

WRSSExpr *
wrsSExprParse (WILContext *ctx, const char *source)
{
  WirbleDiagnostics diag;
  WirbleSexpTree tree;
  const WirbleSexpNode *root;
  const WirbleSexpNode *form;
  WRSSExpr *expr;

  wirble_diag_init (&diag, stderr, "wrs");
  wirble_sexp_tree_init (&tree, &diag);
  if (!wirble_sexp_tree_parse_cstr (&tree, "<string>", source))
    {
      wirble_sexp_tree_destroy (&tree);
      return NULL;
    }

  root = wirble_sexp_tree_root (&tree);
  form = wirble_sexp_node_child (root, 0u);
  expr = wrs_sexpr_from_tree (ctx, form);
  wirble_sexp_tree_destroy (&tree);
  return expr;
}

void
wrs_sexpr_destroy_internal (WRSSExpr *expr)
{
  (void) expr;
}

void
wrsSExprDestroy (WILContext *ctx, WRSSExpr *expr)
{
  (void) ctx;
  (void) expr;
}

WRSSExprKind wrsSExprGetKind (const WRSSExpr *expr)
{ return expr == NULL ? WRS_SEXPR_ATOM : expr->kind; }
const char *wrsSExprGetAtom (const WRSSExpr *expr)
{ return expr == NULL ? NULL : expr->atom; }
int64_t wrsSExprGetInt (const WRSSExpr *expr)
{ return expr == NULL ? 0 : expr->intVal; }
double wrsSExprGetFloat (const WRSSExpr *expr)
{ return expr == NULL ? 0.0 : expr->floatVal; }
uint32_t wrsSExprListLength (const WRSSExpr *expr)
{ return (expr == NULL || expr->kind != WRS_SEXPR_LIST) ? 0u : expr->list.count; }
WRSSExpr *wrsSExprListGet (const WRSSExpr *expr, uint32_t index)
{ return (expr == NULL || expr->kind != WRS_SEXPR_LIST || index >= expr->list.count) ? NULL : expr->list.elements[index]; }

static void
wrs_stringify_expr (WirbleByteBuf *buffer, const WRSSExpr *expr)
{
  uint32_t index;
  char local[64];

  if (expr == NULL)
    {
      return;
    }

  switch (expr->kind)
    {
    case WRS_SEXPR_ATOM:
      wrs_append_text (buffer, expr->atom);
      break;
    case WRS_SEXPR_STRING:
      wrs_append_text (buffer, "\"");
      wrs_append_text (buffer, expr->atom);
      wrs_append_text (buffer, "\"");
      break;
    case WRS_SEXPR_NUMBER:
      snprintf (local, sizeof (local), "%lld", (long long) expr->intVal);
      wrs_append_text (buffer, local);
      break;
    case WRS_SEXPR_LIST:
      wrs_append_text (buffer, "(");
      for (index = 0u; index < expr->list.count; ++index)
        {
          if (index != 0u)
            {
              wrs_append_text (buffer, " ");
            }
          wrs_stringify_expr (buffer, expr->list.elements[index]);
        }
      wrs_append_text (buffer, ")");
      break;
    }
}

char *
wrs_sexpr_to_string_internal (WILContext *ctx, const WRSSExpr *expr)
{
  WirbleByteBuf buffer;
  char *text;
  (void) ctx;

  wirble_bytebuf_init (&buffer);
  wrs_stringify_expr (&buffer, expr);
  (void) wirble_bytebuf_append_byte (&buffer, 0u);
  text = (char *) buffer.data;
  return text;
}

char *
wrsSExprToString (WILContext *ctx, const WRSSExpr *expr)
{
  return wrs_sexpr_to_string_internal (ctx, expr);
}
