{
#include <stdlib.h>

#include "examples/common/frontend.h"

typedef struct WirbleFrontendParseNode {
  WirbleExpr *expr;
} WirbleFrontendParseNode;

#define D_ParseNode_User WirbleFrontendParseNode
}

translation_unit
  : statement
    { $$.expr = $0.expr; }
  ;

statement
  : expr ';' { $$.expr = $0.expr; }
  ;

expr
  : integer                  { $$.expr = wirbleExprInt (strtoll ($n0.start_loc.s, NULL, 10)); }
  | '-' expr $unary_op_right 30
                           { $$.expr = wirbleExprUnary (WIRBLE_UNARY_NEG, $1.expr); }
  | expr '+' expr $binary_op_left 10
                           { $$.expr = wirbleExprBinary (WIRBLE_BINARY_ADD, $0.expr, $2.expr); }
  | expr '-' expr $binary_op_left 10
                           { $$.expr = wirbleExprBinary (WIRBLE_BINARY_SUB, $0.expr, $2.expr); }
  | expr '*' expr $binary_op_left 20
                           { $$.expr = wirbleExprBinary (WIRBLE_BINARY_MUL, $0.expr, $2.expr); }
  | expr '/' expr $binary_op_left 20
                           { $$.expr = wirbleExprBinary (WIRBLE_BINARY_DIV, $0.expr, $2.expr); }
  | '(' expr ')'             { $$.expr = $1.expr; }
  ;

integer
  : "[0-9]+"
  ;

whitespace
  : "[ \t\r\n]*"
  ;
