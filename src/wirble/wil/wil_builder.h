#ifndef WIRBLE_WIL_BUILDER_H
#define WIRBLE_WIL_BUILDER_H

#include "wil_nodes.h"

WILNode *wil_build_unary (WILContext *ctx, WILNodeKind kind, WILNode *operand,
                          WILType type_hint);
WILNode *wil_build_binary (WILContext *ctx, WILNodeKind kind, WILNode *lhs,
                           WILNode *rhs, WILType type_hint);
WILNode *wil_build_nary (WILContext *ctx, WILNodeKind kind, WILNode **inputs,
                         const WILEdgeKind *edge_kinds, WILIndex count,
                         WILType type_hint, const char *name);

#endif /* WIRBLE_WIL_BUILDER_H */
