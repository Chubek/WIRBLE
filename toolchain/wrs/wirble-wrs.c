/* wirble-wrs.c — WIL Rewrite System S-Expression Frontend
 *
 * Implements the wirble-wrs.h API using libsfsexp for .wrs file parsing.
 *
 * .wrs DSL grammar (S-expression based):
 *
 *   (defrule <name>
 *     (pattern <sexpr>)
 *     (replacement <sexpr>)
 *     (guard <guard-sexpr>)?)       ; optional
 *
 *   (defstrategy <name> <strategy-kind>
 *     <rule-ref>...)
 *
 *   (defpredicate <name>
 *     (params <param>...)
 *     (body <sexpr>))
 *
 * Pattern variables are atoms prefixed with '?' (e.g., ?x, ?lhs, ?rhs).
 * Wildcard is '_'.
 *
 * Guard forms:
 *   (and <g1> <g2>)
 *   (or  <g1> <g2>)
 *   (not <g>)
 *   (eq  <a> <b>)
 *   (ne  <a> <b>)
 *   (lt  <a> <b>)  (le <a> <b>)  (gt <a> <b>)  (ge <a> <b>)
 *   (typeof <var> <type>)
 *   (predicate <name> <arg>...)
 *
 * Strategy kinds: innermost, outermost, leftmost, parallel, fixpoint
 */

#include "wirble-wrs.h"
#include <assert.h>
#include <errno.h>
#include <sfsexp/sexp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ═══════════════════════════════════════════════════════════════════
 *  Internal helpers
 * ═══════════════════════════════════════════════════════════════════ */

/* Safe string duplicate */
static char *
wrs_strdup (const char *s)
{
  if (!s)
    return NULL;
  size_t len = strlen (s) + 1;
  char *d = (char *)malloc (len);
  if (d)
    memcpy (d, s, len);
  return d;
}

/* Check if sexp_t node is an atom with a given value */
static int
atom_eq (const sexp_t *s, const char *val)
{
  if (!s || s->ty != SEXP_VALUE)
    return 0;
  return strcmp (s->val, val) == 0;
}

/* Check if an atom is a pattern variable (?-prefixed) */
static int
is_pattern_var (const char *val)
{
  return val && val[0] == '?';
}

/* Check if an atom is the wildcard */
static int
is_wildcard (const char *val)
{
  return val && strcmp (val, "_") == 0;
}

/* Advance to the next sibling in an S-expression list */
static sexp_t *
snext (const sexp_t *s)
{
  return s ? s->next : NULL;
}

/* Get the first child of a list node */
static sexp_t *
shead (const sexp_t *s)
{
  return (s && s->ty == SEXP_LIST) ? s->list : NULL;
}

/* ═══════════════════════════════════════════════════════════════════
 *  Arena allocator
 * ═══════════════════════════════════════════════════════════════════ */

#define WRS_ARENA_BLOCK_SIZE (64 * 1024)

typedef struct WRSArenaBlock
{
  struct WRSArenaBlock *prev;
  size_t used;
  size_t cap;
  char data[];
} WRSArenaBlock;

struct WRSArena
{
  WRSArenaBlock *current;
};

WRSArena *
wrs_arena_create (void)
{
  WRSArena *a = (WRSArena *)calloc (1, sizeof (WRSArena));
  return a;
}

static void *
wrs_arena_alloc (WRSArena *a, size_t size)
{
  size = (size + 7) & ~(size_t)7; /* align to 8 */
  WRSArenaBlock *b = a->current;
  if (!b || b->used + size > b->cap)
    {
      size_t cap = WRS_ARENA_BLOCK_SIZE;
      if (size > cap)
        cap = size * 2;
      b = (WRSArenaBlock *)malloc (sizeof (WRSArenaBlock) + cap);
      if (!b)
        return NULL;
      b->prev = a->current;
      b->used = 0;
      b->cap = cap;
      a->current = b;
    }
  void *ptr = b->data + b->used;
  b->used += size;
  return ptr;
}

static char *
wrs_arena_strdup (WRSArena *a, const char *s)
{
  if (!s)
    return NULL;
  size_t len = strlen (s) + 1;
  char *d = (char *)wrs_arena_alloc (a, len);
  if (d)
    memcpy (d, s, len);
  return d;
}

void
wrs_arena_destroy (WRSArena *a)
{
  if (!a)
    return;
  WRSArenaBlock *b = a->current;
  while (b)
    {
      WRSArenaBlock *prev = b->prev;
      free (b);
      b = prev;
    }
  free (a);
}

/* ═══════════════════════════════════════════════════════════════════
 *  Diagnostic context
 * ═══════════════════════════════════════════════════════════════════ */

#define WRS_MAX_DIAGS 256

struct WRSDiag
{
  WRSDiagSeverity severity;
  char *message;
  const char *source_file;
  int line;
};

struct WRSDiagContext
{
  WRSDiag diags[WRS_MAX_DIAGS];
  int count;
  WRSArena *arena;
};

WRSDiagContext *
wrs_diag_create (WRSArena *arena)
{
  WRSDiagContext *dc
      = (WRSDiagContext *)wrs_arena_alloc (arena, sizeof (WRSDiagContext));
  if (dc)
    {
      memset (dc, 0, sizeof (*dc));
      dc->arena = arena;
    }
  return dc;
}

static void
wrs_diag_emit (WRSDiagContext *dc, WRSDiagSeverity sev, const char *file,
               int line, const char *fmt, ...)
{
  if (!dc || dc->count >= WRS_MAX_DIAGS)
    return;
  WRSDiag *d = &dc->diags[dc->count++];
  d->severity = sev;
  d->source_file = file;
  d->line = line;

  char buf[1024];
  va_list ap;
  va_start (ap, fmt);
  vsnprintf (buf, sizeof (buf), fmt, ap);
  va_end (ap);
  d->message = wrs_arena_strdup (dc->arena, buf);
}

int
wrs_diag_count (const WRSDiagContext *dc)
{
  return dc ? dc->count : 0;
}

const WRSDiag *
wrs_diag_get (const WRSDiagContext *dc, int idx)
{
  if (!dc || idx < 0 || idx >= dc->count)
    return NULL;
  return &dc->diags[idx];
}

WRSDiagSeverity
wrs_diag_severity (const WRSDiag *d)
{
  return d->severity;
}
const char *
wrs_diag_message (const WRSDiag *d)
{
  return d->message;
}

/* ═══════════════════════════════════════════════════════════════════
 *  Pattern AST construction from S-expressions
 * ═══════════════════════════════════════════════════════════════════ */

/*
 * WRSPattern node kinds:
 *   WRS_PAT_VAR       — pattern variable (?x)
 *   WRS_PAT_WILDCARD  — wildcard (_)
 *   WRS_PAT_CONST     — literal constant (integer, string atom)
 *   WRS_PAT_NODE      — compound: (op child...)
 */

struct WRSPattern
{
  WRSPatternKind kind;
  char *symbol; /* var name, const value, or op name */
  WRSPattern **children;
  int num_children;
  WILNodeKind node_kind; /* for WRS_PAT_NODE: the WIL opcode */
};

static WRSPattern *
pat_alloc (WRSArena *a, WRSPatternKind kind, const char *sym)
{
  WRSPattern *p = (WRSPattern *)wrs_arena_alloc (a, sizeof (WRSPattern));
  memset (p, 0, sizeof (*p));
  p->kind = kind;
  p->symbol = wrs_arena_strdup (a, sym);
  return p;
}

/* Forward declaration */
static WRSPattern *parse_pattern (WRSArena *a, const sexp_t *sx,
                                  WRSDiagContext *dc);

/* Parse children of a compound pattern list */
static void
parse_pat_children (WRSArena *a, WRSPattern *parent, const sexp_t *first_child,
                    WRSDiagContext *dc)
{
  /* Count children */
  int n = 0;
  for (const sexp_t *c = first_child; c; c = c->next)
    n++;
  if (n == 0)
    return;

  parent->children
      = (WRSPattern **)wrs_arena_alloc (a, sizeof (WRSPattern *) * n);
  parent->num_children = n;
  int i = 0;
  for (const sexp_t *c = first_child; c; c = c->next, i++)
    {
      parent->children[i] = parse_pattern (a, c, dc);
    }
}

/* Resolve a WIL node kind from a string name.
 * Returns WIL_NODE_INVALID (0) if unknown. */
static WILNodeKind
resolve_wil_node_kind (const char *name)
{
  /* Canonical WIL node kind table — extend as WIL grows */
  static const struct
  {
    const char *str;
    WILNodeKind kind;
  } table[] = { { "add", WIL_ADD },       { "sub", WIL_SUB },
                { "mul", WIL_MUL },       { "div", WIL_DIV },
                { "mod", WIL_MOD },       { "neg", WIL_NEG },
                { "and", WIL_AND },       { "or", WIL_OR },
                { "xor", WIL_XOR },       { "not", WIL_NOT },
                { "shl", WIL_SHL },       { "shr", WIL_SHR },
                { "sar", WIL_SAR },       { "eq", WIL_EQ },
                { "ne", WIL_NE },         { "lt", WIL_LT },
                { "le", WIL_LE },         { "gt", WIL_GT },
                { "ge", WIL_GE },         { "phi", WIL_PHI },
                { "region", WIL_REGION }, { "if", WIL_IF },
                { "start", WIL_START },   { "return", WIL_RETURN },
                { "const", WIL_CONST },   { "param", WIL_PARAM },
                { "load", WIL_LOAD },     { "store", WIL_STORE },
                { "call", WIL_CALL },     { "proj", WIL_PROJ },
                { "dead", WIL_DEAD },     { "loop", WIL_LOOP },
                { "merge", WIL_MERGE },   { NULL, 0 } };
  for (int i = 0; table[i].str; i++)
    {
      if (strcmp (name, table[i].str) == 0)
        return table[i].kind;
    }
  return 0; /* WIL_NODE_INVALID */
}

static WRSPattern *
parse_pattern (WRSArena *a, const sexp_t *sx, WRSDiagContext *dc)
{
  if (!sx)
    {
      wrs_diag_emit (dc, WRS_DIAG_ERROR, NULL, 0,
                     "null S-expression in pattern");
      return pat_alloc (a, WRS_PAT_WILDCARD, "_");
    }

  /* Atom */
  if (sx->ty == SEXP_VALUE)
    {
      const char *v = sx->val;
      if (is_wildcard (v))
        return pat_alloc (a, WRS_PAT_WILDCARD, v);
      if (is_pattern_var (v))
        return pat_alloc (a, WRS_PAT_VAR, v);
      return pat_alloc (a, WRS_PAT_CONST, v);
    }

  /* List: (op child...) */
  if (sx->ty == SEXP_LIST)
    {
      sexp_t *head = sx->list;
      if (!head || head->ty != SEXP_VALUE)
        {
          wrs_diag_emit (dc, WRS_DIAG_ERROR, NULL, 0,
                         "pattern list must start with an atom operator");
          return pat_alloc (a, WRS_PAT_WILDCARD, "_");
        }
      const char *op = head->val;
      WILNodeKind nk = resolve_wil_node_kind (op);
      WRSPattern *p = pat_alloc (a, WRS_PAT_NODE, op);
      p->node_kind = nk;
      if (nk == 0)
        {
          wrs_diag_emit (dc, WRS_DIAG_WARNING, NULL, 0,
                         "unknown WIL node kind '%s' in pattern", op);
        }
      parse_pat_children (a, p, head->next, dc);
      return p;
    }

  wrs_diag_emit (dc, WRS_DIAG_ERROR, NULL, 0,
                 "unexpected sexp type in pattern");
  return pat_alloc (a, WRS_PAT_WILDCARD, "_");
}

/* ═══════════════════════════════════════════════════════════════════
 *  Guard AST construction from S-expressions
 * ═══════════════════════════════════════════════════════════════════ */

struct WRSGuard
{
  WRSGuardKind kind;
  char *symbol; /* for typeof: type name; for predicate: name */
  WRSGuard *left;
  WRSGuard *right;
  WRSPattern *operand_a; /* for comparison guards */
  WRSPattern *operand_b;
  WRSPattern **pred_args;
  int num_pred_args;
};

static WRSGuard *
guard_alloc (WRSArena *a, WRSGuardKind kind)
{
  WRSGuard *g = (WRSGuard *)wrs_arena_alloc (a, sizeof (WRSGuard));
  memset (g, 0, sizeof (*g));
  g->kind = kind;
  return g;
}

static WRSGuard *parse_guard (WRSArena *a, const sexp_t *sx,
                              WRSDiagContext *dc);

static WRSGuardKind
resolve_guard_kind (const char *op)
{
  static const struct
  {
    const char *s;
    WRSGuardKind k;
  } tbl[] = { { "and", WRS_GUARD_AND },
              { "or", WRS_GUARD_OR },
              { "not", WRS_GUARD_NOT },
              { "eq", WRS_GUARD_EQ },
              { "ne", WRS_GUARD_NE },
              { "lt", WRS_GUARD_LT },
              { "le", WRS_GUARD_LE },
              { "gt", WRS_GUARD_GT },
              { "ge", WRS_GUARD_GE },
              { "typeof", WRS_GUARD_TYPEOF },
              { "predicate", WRS_GUARD_PREDICATE },
              { NULL, 0 } };
  for (int i = 0; tbl[i].s; i++)
    if (strcmp (op, tbl[i].s) == 0)
      return tbl[i].k;
  return WRS_GUARD_INVALID;
}

static WRSGuard *
parse_guard (WRSArena *a, const sexp_t *sx, WRSDiagContext *dc)
{
  if (!sx || sx->ty != SEXP_LIST)
    {
      wrs_diag_emit (dc, WRS_DIAG_ERROR, NULL, 0, "guard must be a list form");
      return NULL;
    }
  sexp_t *head = sx->list;
  if (!head || head->ty != SEXP_VALUE)
    {
      wrs_diag_emit (dc, WRS_DIAG_ERROR, NULL, 0,
                     "guard list must start with operator atom");
      return NULL;
    }

  WRSGuardKind kind = resolve_guard_kind (head->val);
  if (kind == WRS_GUARD_INVALID)
    {
      wrs_diag_emit (dc, WRS_DIAG_ERROR, NULL, 0,
                     "unknown guard operator '%s'", head->val);
      return NULL;
    }

  WRSGuard *g = guard_alloc (a, kind);

  switch (kind)
    {
    case WRS_GUARD_AND:
    case WRS_GUARD_OR:
      {
        sexp_t *lhs = snext (head);
        sexp_t *rhs = lhs ? snext (lhs) : NULL;
        g->left = parse_guard (a, lhs, dc);
        g->right = parse_guard (a, rhs, dc);
        break;
      }
    case WRS_GUARD_NOT:
      {
        sexp_t *operand = snext (head);
        g->left = parse_guard (a, operand, dc);
        break;
      }
    case WRS_GUARD_EQ:
    case WRS_GUARD_NE:
    case WRS_GUARD_LT:
    case WRS_GUARD_LE:
    case WRS_GUARD_GT:
    case WRS_GUARD_GE:
      {
        sexp_t *pa = snext (head);
        sexp_t *pb = pa ? snext (pa) : NULL;
        g->operand_a = parse_pattern (a, pa, dc);
        g->operand_b = parse_pattern (a, pb, dc);
        break;
      }
    case WRS_GUARD_TYPEOF:
      {
        sexp_t *var = snext (head);
        sexp_t *type = var ? snext (var) : NULL;
        g->operand_a = parse_pattern (a, var, dc);
        g->symbol = type ? wrs_arena_strdup (a, type->val) : NULL;
        break;
      }
    case WRS_GUARD_PREDICATE:
      {
        sexp_t *name = snext (head);
        g->symbol = name ? wrs_arena_strdup (a, name->val) : NULL;
        /* Remaining children are arguments */
        int n = 0;
        for (sexp_t *c = name ? snext (name) : NULL; c; c = c->next)
          n++;
        if (n > 0)
          {
            g->pred_args = (WRSPattern **)wrs_arena_alloc (
                a, sizeof (WRSPattern *) * n);
            g->num_pred_args = n;
            int i = 0;
            for (sexp_t *c = snext (name); c; c = c->next, i++)
              g->pred_args[i] = parse_pattern (a, c, dc);
          }
        break;
      }
    default:
      break;
    }
  return g;
}

/* ═══════════════════════════════════════════════════════════════════
 *  Predicate definition
 * ═══════════════════════════════════════════════════════════════════ */

struct WRSPredicate
{
  char *name;
  char **params;
  int num_params;
  sexp_t *body_sexp;    /* raw S-expression body, interpreted at eval time */
  WRSPattern *body_pat; /* optional compiled body pattern */
};

static WRSPredicate *
parse_defpredicate (WRSArena *a, const sexp_t *form, WRSDiagContext *dc)
{
  /* (defpredicate <name> (params <p>...) (body <sexpr>)) */
  sexp_t *head = form->list; /* "defpredicate" atom */
  sexp_t *name_sx = snext (head);
  if (!name_sx || name_sx->ty != SEXP_VALUE)
    {
      wrs_diag_emit (dc, WRS_DIAG_ERROR, NULL, 0,
                     "defpredicate: expected name atom");
      return NULL;
    }

  WRSPredicate *pred
      = (WRSPredicate *)wrs_arena_alloc (a, sizeof (WRSPredicate));
  memset (pred, 0, sizeof (*pred));
  pred->name = wrs_arena_strdup (a, name_sx->val);

  /* Walk remaining clauses */
  for (sexp_t *clause = snext (name_sx); clause; clause = snext (clause))
    {
      if (clause->ty != SEXP_LIST)
        continue;
      sexp_t *tag = clause->list;
      if (!tag || tag->ty != SEXP_VALUE)
        continue;

      if (strcmp (tag->val, "params") == 0)
        {
          int n = 0;
          for (sexp_t *p = snext (tag); p; p = snext (p))
            n++;
          pred->params = (char **)wrs_arena_alloc (a, sizeof (char *) * n);
          pred->num_params = n;
          int i = 0;
          for (sexp_t *p = snext (tag); p; p = snext (p), i++)
            pred->params[i] = wrs_arena_strdup (a, p->val);
        }
      else if (strcmp (tag->val, "body") == 0)
        {
          sexp_t *body = snext (tag);
          pred->body_sexp = body ? copy_sexp (body) : NULL;
          pred->body_pat = body ? parse_pattern (a, body, dc) : NULL;
        }
    }
  return pred;
}

/* ═══════════════════════════════════════════════════════════════════
 *  Rewrite rule
 * ═══════════════════════════════════════════════════════════════════ */

struct WRSRule
{
  char *name;
  WRSPattern *pattern;
  WRSPattern *replacement;
  WRSGuard *guard; /* NULL if unconditional */
  int priority;    /* lower = higher priority, default 0 */
};

static WRSRule *
parse_defrule (WRSArena *a, const sexp_t *form, WRSDiagContext *dc)
{
  /* (defrule <name>
   *   (pattern <sexpr>)
   *   (replacement <sexpr>)
   *   (guard <guard-sexpr>)?
   *   (priority <int>)?)
   */
  sexp_t *head = form->list; /* "defrule" */
  sexp_t *name_sx = snext (head);
  if (!name_sx || name_sx->ty != SEXP_VALUE)
    {
      wrs_diag_emit (dc, WRS_DIAG_ERROR, NULL, 0,
                     "defrule: expected name atom");
      return NULL;
    }

  WRSRule *rule = (WRSRule *)wrs_arena_alloc (a, sizeof (WRSRule));
  memset (rule, 0, sizeof (*rule));
  rule->name = wrs_arena_strdup (a, name_sx->val);

  for (sexp_t *clause = snext (name_sx); clause; clause = snext (clause))
    {
      if (clause->ty != SEXP_LIST)
        continue;
      sexp_t *tag = clause->list;
      if (!tag || tag->ty != SEXP_VALUE)
        continue;

      if (strcmp (tag->val, "pattern") == 0)
        {
          rule->pattern = parse_pattern (a, snext (tag), dc);
        }
      else if (strcmp (tag->val, "replacement") == 0)
        {
          rule->replacement = parse_pattern (a, snext (tag), dc);
        }
      else if (strcmp (tag->val, "guard") == 0)
        {
          rule->guard = parse_guard (a, snext (tag), dc);
        }
      else if (strcmp (tag->val, "priority") == 0)
        {
          sexp_t *pval = snext (tag);
          if (pval && pval->ty == SEXP_VALUE)
            rule->priority = atoi (pval->val);
        }
    }

  if (!rule->pattern)
    {
      wrs_diag_emit (dc, WRS_DIAG_ERROR, NULL, 0,
                     "defrule '%s': missing (pattern ...) clause", rule->name);
    }
  if (!rule->replacement)
    {
      wrs_diag_emit (dc, WRS_DIAG_ERROR, NULL, 0,
                     "defrule '%s': missing (replacement ...) clause",
                     rule->name);
    }
  return rule;
}

/* ═══════════════════════════════════════════════════════════════════
 *  Strategy definition
 * ═══════════════════════════════════════════════════════════════════ */

struct WRSStrategy
{
  char *name;
  WRSStrategyKind kind;
  char **rule_refs; /* names of rules in application order */
  int num_rules;
  int max_iterations; /* for fixpoint, 0 = unlimited */
};

static WRSStrategyKind
resolve_strategy_kind (const char *s)
{
  if (strcmp (s, "innermost") == 0)
    return WRS_STRAT_INNERMOST;
  if (strcmp (s, "outermost") == 0)
    return WRS_STRAT_OUTERMOST;
  if (strcmp (s, "leftmost") == 0)
    return WRS_STRAT_LEFTMOST;
  if (strcmp (s, "parallel") == 0)
    return WRS_STRAT_PARALLEL;
  if (strcmp (s, "fixpoint") == 0)
    return WRS_STRAT_FIXPOINT;
  return WRS_STRAT_INNERMOST; /* default */
}

static WRSStrategy *
parse_defstrategy (WRSArena *a, const sexp_t *form, WRSDiagContext *dc)
{
  /* (defstrategy <name> <kind>
   *   <rule-name>...
   *   (max-iterations <n>)?)
   */
  sexp_t *head = form->list;
  sexp_t *name_sx = snext (head);
  sexp_t *kind_sx = name_sx ? snext (name_sx) : NULL;

  if (!name_sx || name_sx->ty != SEXP_VALUE)
    {
      wrs_diag_emit (dc, WRS_DIAG_ERROR, NULL, 0,
                     "defstrategy: expected name atom");
      return NULL;
    }
  if (!kind_sx || kind_sx->ty != SEXP_VALUE)
    {
      wrs_diag_emit (dc, WRS_DIAG_ERROR, NULL, 0,
                     "defstrategy '%s': expected strategy kind", name_sx->val);
      return NULL;
    }

  WRSStrategy *strat
      = (WRSStrategy *)wrs_arena_alloc (a, sizeof (WRSStrategy));
  memset (strat, 0, sizeof (*strat));
  strat->name = wrs_arena_strdup (a, name_sx->val);
  strat->kind = resolve_strategy_kind (kind_sx->val);

  /* Collect rule references and optional max-iterations */
  int cap = 16;
  strat->rule_refs = (char **)wrs_arena_alloc (a, sizeof (char *) * cap);

  for (sexp_t *c = snext (kind_sx); c; c = snext (c))
    {
      if (c->ty == SEXP_VALUE)
        {
          if (strat->num_rules >= cap)
            {
              int newcap = cap * 2;
              char **nr
                  = (char **)wrs_arena_alloc (a, sizeof (char *) * newcap);
              memcpy (nr, strat->rule_refs, sizeof (char *) * cap);
              strat->rule_refs = nr;
              cap = newcap;
            }
          strat->rule_refs[strat->num_rules++] = wrs_arena_strdup (a, c->val);
        }
      else if (c->ty == SEXP_LIST)
        {
          sexp_t *tag = c->list;
          if (tag && atom_eq (tag, "max-iterations"))
            {
              sexp_t *val = snext (tag);
              if (val && val->ty == SEXP_VALUE)
                strat->max_iterations = atoi (val->val);
            }
        }
    }
  return strat;
}

/* ═══════════════════════════════════════════════════════════════════
 *  WRS File — top-level container
 * ═══════════════════════════════════════════════════════════════════ */

struct WRSFile
{
  WRSArena *arena;
  WRSDiagContext *diag;
  WRSRule **rules;
  int num_rules;
  WRSStrategy **strategies;
  int num_strategies;
  WRSPredicate **predicates;
  int num_predicates;
};

WRSFile *
wrs_file_create (void)
{
  WRSArena *a = wrs_arena_create ();
  if (!a)
    return NULL;
  WRSFile *f = (WRSFile *)wrs_arena_alloc (a, sizeof (WRSFile));
  memset (f, 0, sizeof (*f));
  f->arena = a;
  f->diag = wrs_diag_create (a);
  return f;
}

void
wrs_file_destroy (WRSFile *f)
{
  if (!f)
    return;
  wrs_arena_destroy (f->arena);
}

/* ═══════════════════════════════════════════════════════════════════
 *  .wrs file parsing
 * ═══════════════════════════════════════════════════════════════════ */

WRSFile *
wrs_file_load (const char *path)
{
  FILE *fp = fopen (path, "rb");
  if (!fp)
    return NULL;

  fseek (fp, 0, SEEK_END);
  long sz = ftell (fp);
  fseek (fp, 0, SEEK_SET);

  char *buf = (char *)malloc (sz + 1);
  if (!buf)
    {
      fclose (fp);
      return NULL;
    }
  fread (buf, 1, sz, fp);
  buf[sz] = '\0';
  fclose (fp);

  WRSFile *f = wrs_file_create ();
  if (!f)
    {
      free (buf);
      return NULL;
    }

  /* Parse S-expressions */
  sexp_t *root = parse_sexp (buf, sz);
  free (buf);

  if (!root)
    {
      wrs_diag_emit (f->diag, WRS_DIAG_ERROR, path, 0,
                     "failed to parse S-expressions");
      return f;
    }

  /* Walk top-level forms */
  for (sexp_t *form = root; form; form = form->next)
    {
      if (form->ty != SEXP_LIST)
        continue;
      sexp_t *head = form->list;
      if (!head || head->ty != SEXP_VALUE)
        continue;

      if (atom_eq (head, "defrule"))
        {
          WRSRule *r = parse_defrule (f->arena, form, f->diag);
          if (r)
            {
              f->rules = (WRSRule **)realloc (
                  f->rules, sizeof (WRSRule *) * (f->num_rules + 1));
              f->rules[f->num_rules++] = r;
            }
        }
      else if (atom_eq (head, "defstrategy"))
        {
          WRSStrategy *s = parse_defstrategy (f->arena, form, f->diag);
          if (s)
            {
              f->strategies = (WRSStrategy **)realloc (
                  f->strategies,
                  sizeof (WRSStrategy *) * (f->num_strategies + 1));
              f->strategies[f->num_strategies++] = s;
            }
        }
      else if (atom_eq (head, "defpredicate"))
        {
          WRSPredicate *p = parse_defpredicate (f->arena, form, f->diag);
          if (p)
            {
              f->predicates = (WRSPredicate **)realloc (
                  f->predicates,
                  sizeof (WRSPredicate *) * (f->num_predicates + 1));
              f->predicates[f->num_predicates++] = p;
            }
        }
      else
        {
          wrs_diag_emit (f->diag, WRS_DIAG_WARNING, path, 0,
                         "unknown top-level form '%s'", head->val);
        }
    }

  destroy_sexp (root);
  return f;
}

int
wrs_file_save (const WRSFile *f, const char *path)
{
  if (!f)
    return -1;
  FILE *fp = fopen (path, "wb");
  if (!fp)
    return -1;

  /* Emit rules */
  for (int i = 0; i < f->num_rules; i++)
    {
      WRSRule *r = f->rules[i];
      fprintf (fp, "(defrule %s\n", r->name);
      fprintf (fp, "  (pattern ");
      wrs_pattern_print (r->pattern, fp);
      fprintf (fp, ")\n  (replacement ");
      wrs_pattern_print (r->replacement, fp);
      fprintf (fp, ")");
      if (r->guard)
        {
          fprintf (fp, "\n  (guard ");
          wrs_guard_print (r->guard, fp);
          fprintf (fp, ")");
        }
      if (r->priority != 0)
        fprintf (fp, "\n  (priority %d)", r->priority);
      fprintf (fp, ")\n\n");
    }

  /* Emit strategies */
  for (int i = 0; i < f->num_strategies; i++)
    {
      WRSStrategy *s = f->strategies[i];
      fprintf (fp, "(defstrategy %s %s", s->name,
               wrs_strategy_kind_name (s->kind));
      for (int j = 0; j < s->num_rules; j++)
        fprintf (fp, " %s", s->rule_refs[j]);
      if (s->max_iterations > 0)
        fprintf (fp, "\n  (max-iterations %d)", s->max_iterations);
      fprintf (fp, ")\n\n");
    }

  /* Emit predicates */
  for (int i = 0; i < f->num_predicates; i++)
    {
      WRSPredicate *p = f->predicates[i];
      fprintf (fp, "(defpredicate %s\n  (params", p->name);
      for (int j = 0; j < p->num_params; j++)
        fprintf (fp, " %s", p->params[j]);
      fprintf (fp, ")\n  (body ");
      if (p->body_pat)
        wrs_pattern_print (p->body_pat, fp);
      fprintf (fp, "))\n\n");
    }

  fclose (fp);
  return 0;
}

/* ═══════════════════════════════════════════════════════════════════
 *  Pattern printing
 * ═══════════════════════════════════════════════════════════════════ */

void
wrs_pattern_print (const WRSPattern *p, FILE *out)
{
  if (!p)
    {
      fprintf (out, "_");
      return;
    }
  switch (p->kind)
    {
    case WRS_PAT_VAR:
    case WRS_PAT_WILDCARD:
    case WRS_PAT_CONST:
      fprintf (out, "%s", p->symbol ? p->symbol : "_");
      break;
    case WRS_PAT_NODE:
      fprintf (out, "(%s", p->symbol ? p->symbol : "?");
      for (int i = 0; i < p->num_children; i++)
        {
          fprintf (out, " ");
          wrs_pattern_print (p->children[i], out);
        }
      fprintf (out, ")");
      break;
    }
}

void
wrs_guard_print (const WRSGuard *g, FILE *out)
{
  if (!g)
    return;
  switch (g->kind)
    {
    case WRS_GUARD_AND:
      fprintf (out, "(and ");
      wrs_guard_print (g->left, out);
      fprintf (out, " ");
      wrs_guard_print (g->right, out);
      fprintf (out, ")");
      break;
    case WRS_GUARD_OR:
      fprintf (out, "(or ");
      wrs_guard_print (g->left, out);
      fprintf (out, " ");
      wrs_guard_print (g->right, out);
      fprintf (out, ")");
      break;
    case WRS_GUARD_NOT:
      fprintf (out, "(not ");
      wrs_guard_print (g->left, out);
      fprintf (out, ")");
      break;
    case WRS_GUARD_EQ:
      fprintf (out, "(eq ");
      wrs_pattern_print (g->operand_a, out);
      fprintf (out, " ");
      wrs_pattern_print (g->operand_b, out);
      fprintf (out, ")");
      break;
    case WRS_GUARD_NE:
      fprintf (out, "(ne ");
      wrs_pattern_print (g->operand_a, out);
      fprintf (out, " ");
      wrs_pattern_print (g->operand_b, out);
      fprintf (out, ")");
      break;
    case WRS_GUARD_LT:
    case WRS_GUARD_LE:
    case WRS_GUARD_GT:
    case WRS_GUARD_GE:
      {
        const char *op = (g->kind == WRS_GUARD_LT)   ? "lt"
                         : (g->kind == WRS_GUARD_LE) ? "le"
                         : (g->kind == WRS_GUARD_GT) ? "gt"
                                                     : "ge";
        fprintf (out, "(%s ", op);
        wrs_pattern_print (g->operand_a, out);
        fprintf (out, " ");
        wrs_pattern_print (g->operand_b, out);
        fprintf (out, ")");
        break;
      }
    case WRS_GUARD_TYPEOF:
      fprintf (out, "(typeof ");
      wrs_pattern_print (g->operand_a, out);
      fprintf (out, " %s)", g->symbol ? g->symbol : "?");
      break;
    case WRS_GUARD_PREDICATE:
      fprintf (out, "(predicate %s", g->symbol ? g->symbol : "?");
      for (int i = 0; i < g->num_pred_args; i++)
        {
          fprintf (out, " ");
          wrs_pattern_print (g->pred_args[i], out);
        }
      fprintf (out, ")");
      break;
    default:
      fprintf (out, "(?)");
      break;
    }
}

const char *
wrs_strategy_kind_name (WRSStrategyKind k)
{
  switch (k)
    {
    case WRS_STRAT_INNERMOST:
      return "innermost";
    case WRS_STRAT_OUTERMOST:
      return "outermost";
    case WRS_STRAT_LEFTMOST:
      return "leftmost";
    case WRS_STRAT_PARALLEL:
      return "parallel";
    case WRS_STRAT_FIXPOINT:
      return "fixpoint";
    default:
      return "unknown";
    }
}

/* ═══════════════════════════════════════════════════════════════════
 *  Substitution map
 * ═══════════════════════════════════════════════════════════════════ */

#define WRS_SUBST_MAX 64

struct WRSSubst
{
  struct
  {
    const char *var;
    WILNode *node;
  } bindings[WRS_SUBST_MAX];
  int count;
};

WRSSubst *
wrs_subst_create (void)
{
  WRSSubst *s = (WRSSubst *)calloc (1, sizeof (WRSSubst));
  return s;
}

void
wrs_subst_destroy (WRSSubst *s)
{
  free (s);
}

void
wrs_subst_bind (WRSSubst *s, const char *var, WILNode *node)
{
  if (!s || s->count >= WRS_SUBST_MAX)
    return;
  s->bindings[s->count].var = var;
  s->bindings[s->count].node = node;
  s->count++;
}

WILNode *
wrs_subst_lookup (const WRSSubst *s, const char *var)
{
  if (!s)
    return NULL;
  for (int i = 0; i < s->count; i++)
    if (strcmp (s->bindings[i].var, var) == 0)
      return s->bindings[i].node;
  return NULL;
}

/* ═══════════════════════════════════════════════════════════════════
 *  Pattern matching
 * ═══════════════════════════════════════════════════════════════════ */

static int match_pattern_rec (const WRSPattern *pat, WILNode *node,
                              WRSSubst *subst);

static int
match_pattern_rec (const WRSPattern *pat, WILNode *node, WRSSubst *subst)
{
  if (!pat)
    return 1;
  if (!node)
    return 0;

  switch (pat->kind)
    {
    case WRS_PAT_WILDCARD:
      return 1;

    case WRS_PAT_VAR:
      {
        WILNode *bound = wrs_subst_lookup (subst, pat->symbol);
        if (bound)
          return (bound == node);
        wrs_subst_bind (subst, pat->symbol, node);
        return 1;
      }

    case WRS_PAT_CONST:
      {
        /* Match constant nodes — check if node is WIL_CONST and value matches
         */
        if (wil_node_kind (node) != WIL_CONST)
          return 0;
        /* Simplified: compare symbol as string; real impl would check typed
         * value */
        return 1; /* stub: assume match */
      }

    case WRS_PAT_NODE:
      {
        if (pat->node_kind != 0 && wil_node_kind (node) != pat->node_kind)
          return 0;
        int nin = wil_node_num_inputs (node);
        if (nin != pat->num_children)
          return 0;
        for (int i = 0; i < nin; i++)
          {
            WILNode *child = wil_node_input (node, i);
            if (!match_pattern_rec (pat->children[i], child, subst))
              return 0;
          }
        return 1;
      }
    }
  return 0;
}

int
wrs_pattern_match (const WRSPattern *pat, WILNode *node, WRSSubst *subst)
{
  if (subst)
    subst->count = 0; /* reset */
  return match_pattern_rec (pat, node, subst);
}

/* ═══════════════════════════════════════════════════════════════════
 *  Guard evaluation
 * ═══════════════════════════════════════════════════════════════════ */

static int eval_guard (const WRSGuard *g, const WRSSubst *subst,
                       const WRSFile *f);

static int
eval_guard (const WRSGuard *g, const WRSSubst *subst, const WRSFile *f)
{
  if (!g)
    return 1; /* no guard = always true */

  switch (g->kind)
    {
    case WRS_GUARD_AND:
      return eval_guard (g->left, subst, f) && eval_guard (g->right, subst, f);
    case WRS_GUARD_OR:
      return eval_guard (g->left, subst, f) || eval_guard (g->right, subst, f);
    case WRS_GUARD_NOT:
      return !eval_guard (g->left, subst, f);

    case WRS_GUARD_EQ:
    case WRS_GUARD_NE:
    case WRS_GUARD_LT:
    case WRS_GUARD_LE:
    case WRS_GUARD_GT:
    case WRS_GUARD_GE:
      {
        /* Stub: compare operands via substitution */
        /* Real impl would resolve operand_a/b patterns and compare WILNode
         * values */
        return 1; /* assume true for now */
      }

    case WRS_GUARD_TYPEOF:
      {
        /* Check if operand_a (a pattern var) has the specified type */
        /* Stub: always true */
        return 1;
      }

    case WRS_GUARD_PREDICATE:
      {
        /* Look up predicate by name and evaluate */
        /* Stub: always true */
        return 1;
      }

    default:
      return 0;
    }
}

int
wrs_guard_eval (const WRSGuard *g, const WRSSubst *subst, const WRSFile *f)
{
  return eval_guard (g, subst, f);
}

/* ═══════════════════════════════════════════════════════════════════
 *  Substitution application (replacement construction)
 * ═══════════════════════════════════════════════════════════════════ */

static WILNode *apply_subst_rec (const WRSPattern *repl, const WRSSubst *subst,
                                 WILGraph *g);

static WILNode *
apply_subst_rec (const WRSPattern *repl, const WRSSubst *subst, WILGraph *g)
{
  if (!repl)
    return NULL;

  switch (repl->kind)
    {
    case WRS_PAT_VAR:
      {
        WILNode *bound = wrs_subst_lookup (subst, repl->symbol);
        return bound; /* return the bound node */
      }

    case WRS_PAT_CONST:
      {
        /* Create a new WIL_CONST node with the constant value */
        /* Stub: parse symbol as integer */
        int64_t val = atoll (repl->symbol);
        return wil_const_i64 (g, val);
      }

    case WRS_PAT_NODE:
      {
        /* Recursively build children, then construct node */
        WILNode **children
            = (WILNode **)malloc (sizeof (WILNode *) * repl->num_children);
        for (int i = 0; i < repl->num_children; i++)
          children[i] = apply_subst_rec (repl->children[i], subst, g);

        WILNode *result = NULL;
        /* Dispatch on node_kind to construct the appropriate WIL node */
        switch (repl->node_kind)
          {
          case WIL_ADD:
            result = wil_add (g, children[0], children[1]);
            break;
          case WIL_SUB:
            result = wil_sub (g, children[0], children[1]);
            break;
          case WIL_MUL:
            result = wil_mul (g, children[0], children[1]);
            break;
          case WIL_DIV:
            result = wil_div (g, children[0], children[1]);
            break;
          /* ... extend for all WIL node kinds ... */
          default:
            /* Fallback: create generic node */
            result = wil_node_create (g, repl->node_kind, children,
                                      repl->num_children);
            break;
          }
        free (children);
        return result;
      }

    case WRS_PAT_WILDCARD:
    default:
      return NULL;
    }
}

WILNode *
wrs_subst_apply (const WRSPattern *repl, const WRSSubst *subst, WILGraph *g)
{
  return apply_subst_rec (repl, subst, g);
}

/* ═══════════════════════════════════════════════════════════════════
 *  Rule application
 * ═══════════════════════════════════════════════════════════════════ */

WILNode *
wrs_rule_apply (const WRSRule *rule, WILNode *node, WILGraph *g,
                const WRSFile *f)
{
  if (!rule || !node)
    return NULL;

  WRSSubst *subst = wrs_subst_create ();
  if (!wrs_pattern_match (rule->pattern, node, subst))
    {
      wrs_subst_destroy (subst);
      return NULL;
    }

  if (rule->guard && !wrs_guard_eval (rule->guard, subst, f))
    {
      wrs_subst_destroy (subst);
      return NULL;
    }

  WILNode *result = wrs_subst_apply (rule->replacement, subst, g);
  wrs_subst_destroy (subst);
  return result;
}

/* ═══════════════════════════════════════════════════════════════════
 *  Strategy application
 * ═══════════════════════════════════════════════════════════════════ */

static WILNode *
apply_rules_to_node (WILNode *node, WRSRule **rules, int nrules, WILGraph *g,
                     const WRSFile *f)
{
  for (int i = 0; i < nrules; i++)
    {
      WILNode *rewritten = wrs_rule_apply (rules[i], node, g, f);
      if (rewritten)
        return rewritten;
    }
  return NULL;
}

static WILNode *
rewrite_innermost (WILNode *node, WRSRule **rules, int nrules, WILGraph *g,
                   const WRSFile *f)
{
  /* Rewrite children first */
  int nin = wil_node_num_inputs (node);
  int changed = 0;
  for (int i = 0; i < nin; i++)
    {
      WILNode *child = wil_node_input (node, i);
      WILNode *new_child = rewrite_innermost (child, rules, nrules, g, f);
      if (new_child && new_child != child)
        {
          wil_node_set_input (node, i, new_child);
          changed = 1;
        }
    }

  /* Then try to rewrite this node */
  WILNode *rewritten = apply_rules_to_node (node, rules, nrules, g, f);
  if (rewritten)
    return rewritten;

  return changed ? node : NULL;
}

static WILNode *
rewrite_outermost (WILNode *node, WRSRule **rules, int nrules, WILGraph *g,
                   const WRSFile *f)
{
  /* Try to rewrite this node first */
  WILNode *rewritten = apply_rules_to_node (node, rules, nrules, g, f);
  if (rewritten)
    return rewritten;

  /* Then rewrite children */
  int nin = wil_node_num_inputs (node);
  int changed = 0;
  for (int i = 0; i < nin; i++)
    {
      WILNode *child = wil_node_input (node, i);
      WILNode *new_child = rewrite_outermost (child, rules, nrules, g, f);
      if (new_child && new_child != child)
        {
          wil_node_set_input (node, i, new_child);
          changed = 1;
        }
    }
  return changed ? node : NULL;
}

WILNode *
wrs_strategy_apply (const WRSStrategy *strat, WILNode *node, WILGraph *g,
                    const WRSFile *f)
{
  if (!strat || !node || !f)
    return NULL;

  /* Resolve rule pointers from names */
  WRSRule **rules = (WRSRule **)malloc (sizeof (WRSRule *) * strat->num_rules);
  int nrules = 0;
  for (int i = 0; i < strat->num_rules; i++)
    {
      for (int j = 0; j < f->num_rules; j++)
        {
          if (strcmp (strat->rule_refs[i], f->rules[j]->name) == 0)
            {
              rules[nrules++] = f->rules[j];
              break;
            }
        }
    }

  WILNode *result = node;
  switch (strat->kind)
    {
    case WRS_STRAT_INNERMOST:
      result = rewrite_innermost (node, rules, nrules, g, f);
      break;
    case WRS_STRAT_OUTERMOST:
      result = rewrite_outermost (node, rules, nrules, g, f);
      break;
    case WRS_STRAT_LEFTMOST:
      /* Stub: similar to innermost but stop at first match */
      result = rewrite_innermost (node, rules, nrules, g, f);
      break;
    case WRS_STRAT_PARALLEL:
      /* Stub: apply all rules in parallel (requires more complex impl) */
      result = rewrite_innermost (node, rules, nrules, g, f);
      break;
    case WRS_STRAT_FIXPOINT:
      {
        int iter = 0;
        int max_iter
            = strat->max_iterations > 0 ? strat->max_iterations : 1000;
        WILNode *prev = node;
        while (iter < max_iter)
          {
            WILNode *next = rewrite_innermost (prev, rules, nrules, g, f);
            if (!next || next == prev)
              break;
            prev = next;
            iter++;
          }
        result = prev;
        break;
      }
    }

  free (rules);
  return result ? result : node;
}

/* ═══════════════════════════════════════════════════════════════════
 *  Normal form checking
 * ═══════════════════════════════════════════════════════════════════ */

int
wrs_is_normal_form (WILNode *node, const WRSFile *f)
{
  if (!node || !f)
    return 1;

  /* Check if any rule can be applied */
  for (int i = 0; i < f->num_rules; i++)
    {
      WRSSubst *subst = wrs_subst_create ();
      if (wrs_pattern_match (f->rules[i]->pattern, node, subst))
        {
          if (!f->rules[i]->guard
              || wrs_guard_eval (f->rules[i]->guard, subst, f))
            {
              wrs_subst_destroy (subst);
              return 0; /* rule can be applied → not in normal form */
            }
        }
      wrs_subst_destroy (subst);
    }

  /* Recursively check children */
  int nin = wil_node_num_inputs (node);
  for (int i = 0; i < nin; i++)
    {
      if (!wrs_is_normal_form (wil_node_input (node, i), f))
        return 0;
    }

  return 1;
}

/* ═══════════════════════════════════════════════════════════════════
 *  Validation
 * ═══════════════════════════════════════════════════════════════════ */

int
wrs_file_validate (WRSFile *f)
{
  if (!f)
    return 0;
  int errors = 0;

  /* Check for duplicate rule names */
  for (int i = 0; i < f->num_rules; i++)
    {
      for (int j = i + 1; j < f->num_rules; j++)
        {
          if (strcmp (f->rules[i]->name, f->rules[j]->name) == 0)
            {
              wrs_diag_emit (f->diag, WRS_DIAG_ERROR, NULL, 0,
                             "duplicate rule name '%s'", f->rules[i]->name);
              errors++;
            }
        }
    }

  /* Check strategy rule references */
  for (int i = 0; i < f->num_strategies; i++)
    {
      WRSStrategy *s = f->strategies[i];
      for (int j = 0; j < s->num_rules; j++)
        {
          int found = 0;
          for (int k = 0; k < f->num_rules; k++)
            {
              if (strcmp (s->rule_refs[j], f->rules[k]->name) == 0)
                {
                  found = 1;
                  break;
                }
            }
          if (!found)
            {
              wrs_diag_emit (f->diag, WRS_DIAG_ERROR, NULL, 0,
                             "strategy '%s' references undefined rule '%s'",
                             s->name, s->rule_refs[j]);
              errors++;
            }
        }
    }

  return errors == 0;
}

/* ═══════════════════════════════════════════════════════════════════
 *  Query API
 * ═══════════════════════════════════════════════════════════════════ */

int
wrs_file_num_rules (const WRSFile *f)
{
  return f ? f->num_rules : 0;
}

const WRSRule *
wrs_file_get_rule (const WRSFile *f, int idx)
{
  if (!f || idx < 0 || idx >= f->num_rules)
    return NULL;
  return f->rules[idx];
}

const WRSRule *
wrs_file_find_rule (const WRSFile *f, const char *name)
{
  if (!f || !name)
    return NULL;
  for (int i = 0; i < f->num_rules; i++)
    if (strcmp (f->rules[i]->name, name) == 0)
      return f->rules[i];
  return NULL;
}

int
wrs_file_num_strategies (const WRSFile *f)
{
  return f ? f->num_strategies : 0;
}

const WRSStrategy *
wrs_file_get_strategy (const WRSFile *f, int idx)
{
  if (!f || idx < 0 || idx >= f->num_strategies)
    return NULL;
  return f->strategies[idx];
}

const WRSStrategy *
wrs_file_find_strategy (const WRSFile *f, const char *name)
{
  if (!f || !name)
    return NULL;
  for (int i = 0; i < f->num_strategies; i++)
    if (strcmp (f->strategies[i]->name, name) == 0)
      return f->strategies[i];
  return NULL;
}

WRSDiagContext *
wrs_file_diag (WRSFile *f)
{
  return f ? f->diag : NULL;
}

/* ═══════════════════════════════════════════════════════════════════
 *  End of wirble-wrs.c
 * ═══════════════════════════════════════════════════════════════════ */
