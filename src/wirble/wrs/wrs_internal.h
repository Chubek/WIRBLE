#ifndef WIRBLE_WRS_INTERNAL_H
#define WIRBLE_WRS_INTERNAL_H

#include <stdint.h>

#include "common/bytebuf.h"
#include "support/sexp_reader.h"
#include "wil/wil_nodes.h"
#include "wirble/wirble-wrs.h"

typedef enum WILPatternValueKind
{
  WIL_PATTERN_VALUE_NONE = 0,
  WIL_PATTERN_VALUE_INT,
  WIL_PATTERN_VALUE_FLOAT,
  WIL_PATTERN_VALUE_BOOL,
  WIL_PATTERN_VALUE_STRING
} WILPatternValueKind;

struct WILPattern
{
  WILPatternKind kind;
  const char *name;
  WILNodeKind node_kind;
  WILPattern **inputs;
  uint32_t input_count;
  int64_t int_value;
  double float_value;
  int bool_value;
  WILPatternValueKind value_kind;
  WILPatternPredicate predicate;
  void *predicate_user_data;
  WILPattern *element_pattern;
  WILPattern *base_pattern;
};

struct WILRewriteRule
{
  const char *name;
  WILPattern *lhs;
  WILPattern *rhs;
  WILRewriteAction action;
  void *user_data;
  int enabled;
  WRSSExpr *guard;
  uint32_t stats_index;
};

typedef struct WILBinding
{
  const char *name;
  WILNode *node;
} WILBinding;

struct WILSubstitution
{
  WILBinding *bindings;
  uint32_t count;
  uint32_t capacity;
};

struct WILMatch
{
  int succeeded;
  WILNode *node;
  WILSubstitution substitution;
};

struct WILRewriteSystem
{
  WILContext *ctx;
  WILRewriteRule **rules;
  uint32_t rule_count;
  uint32_t rule_capacity;
  WILRewriteStrategy strategy;
  WILCustomStrategy custom_strategy;
  void *custom_strategy_user_data;
  int tracing;
  WILRewriteStats stats;
};

struct WRSFile
{
  WILContext *ctx;
  WILRewriteRule **rules;
  uint32_t rule_count;
  uint32_t rule_capacity;
  WILRewriteStrategy strategy;
};

WILPattern *wrs_pattern_alloc (WILContext *ctx);
WILRewriteRule *wrs_rule_alloc (WILContext *ctx);
int wrs_append_rule (WILRewriteRule ***rules, uint32_t *count,
                     uint32_t *capacity, WILRewriteRule *rule);
void wrs_append_text (WirbleByteBuf *buffer, const char *text);
void wrs_append_format (WirbleByteBuf *buffer, const char *format, ...);
const char *wrs_node_kind_symbol (WILNodeKind kind);
WILNodeKind wrs_parse_node_kind_symbol (const char *symbol);
WRSSExpr *wrs_sexpr_from_tree (WILContext *ctx, const WirbleSexpNode *node);
void wrs_sexpr_destroy_internal (WRSSExpr *expr);
char *wrs_sexpr_to_string_internal (WILContext *ctx, const WRSSExpr *expr);

#endif /* WIRBLE_WRS_INTERNAL_H */
