#ifndef WIRBLE_WIL_H
#define WIRBLE_WIL_H

#include "api-boilerplate.h"

WIRBLE_BEGIN_DECLS

/* ------------------------------------------------------------
   Basic Types
   ------------------------------------------------------------ */

typedef unsigned long WILId;
typedef unsigned long WILIndex;

typedef enum {
    WIL_NODE_CONST,
    WIL_NODE_PARAMETER,
    WIL_NODE_OP,
    WIL_NODE_CONTROL,
    WIL_NODE_REGION,
    WIL_NODE_PHI,
    WIL_NODE_END
} WILNodeKind;

typedef enum {
    WIL_OP_ADD,
    WIL_OP_SUB,
    WIL_OP_MUL,
    WIL_OP_DIV,
    WIL_OP_CMP,
    WIL_OP_LOAD,
    WIL_OP_STORE,
    WIL_OP_CALL
} WILOpCode;

/* ------------------------------------------------------------
   Opaque Handles
   ------------------------------------------------------------ */

typedef struct WILContext WILContext;
typedef struct WILNode WILNode;
typedef struct WILArena WILArena;
typedef struct WILSchedule WILSchedule;
typedef struct WILBlock WILBlock;
typedef struct WILCFG WILCFG;
typedef struct WILDomTree WILDomTree;

/* ------------------------------------------------------------
   Context Management
   ------------------------------------------------------------ */

WILContext* wilCreateContext(void);
void        wilDestroyContext(WILContext* ctx);

/* ------------------------------------------------------------
   Memory Safety: Arena Allocator
   ------------------------------------------------------------ */

WILArena* wilCreateArena(void);
void      wilDestroyArena(WILArena* arena);
void*     wilArenaAlloc(WILArena* arena, unsigned long size);
void      wilArenaReset(WILArena* arena);

/* Attach arena to context for automatic node allocation */
void wilContextSetArena(WILContext* ctx, WILArena* arena);

/* ------------------------------------------------------------
   Node Construction
   ------------------------------------------------------------ */

WILNode* wilMakeConst(WILContext* ctx, double value);
WILNode* wilMakeParameter(WILContext* ctx, const char* name);
WILNode* wilMakeOp(WILContext* ctx, WILOpCode opcode, WILNode** inputs, WILIndex inputCount);
WILNode* wilMakeRegion(WILContext* ctx, WILNode** controls, WILIndex controlCount);
WILNode* wilMakePhi(WILContext* ctx, WILNode* region, WILNode** values, WILIndex valueCount);
WILNode* wilMakeIf(WILContext* ctx, WILNode* cond);
WILNode* wilMakeLoopBegin(WILContext* ctx, WILNode* entryControl);
WILNode* wilMakeLoopEnd(WILContext* ctx, WILNode* loopControl);
WILNode* wilMakeEnd(WILContext* ctx, WILNode* control);

/* ------------------------------------------------------------
   Node Inspection
   ------------------------------------------------------------ */

WILId        wilNodeId(const WILNode* node);
WILNodeKind  wilNodeKind(const WILNode* node);
WILOpCode    wilNodeOpcode(const WILNode* node);
WILIndex     wilNodeInputCount(const WILNode* node);
WILNode*     wilNodeInput(const WILNode* node, WILIndex index);

/* ------------------------------------------------------------
   Mutability
   ------------------------------------------------------------ */

void wilAddInput(WILNode* node, WILNode* input);
void wilReplaceInput(WILNode* node, WILIndex index, WILNode* newInput);

/* ------------------------------------------------------------
   CFG Extraction (per semantics paper, Definition 3.3)
   ------------------------------------------------------------ */

/* Extract CFG where nodes are region nodes */
WILCFG* wilExtractCFG(const WILContext* ctx);
void    wilDestroyCFG(WILCFG* cfg);

/* Query CFG */
WILIndex wilCFGBlockCount(const WILCFG* cfg);
WILNode* wilCFGBlockRegion(const WILCFG* cfg, WILIndex blockIndex);
WILIndex wilCFGSuccessorCount(const WILCFG* cfg, WILIndex blockIndex);
WILIndex wilCFGSuccessor(const WILCFG* cfg, WILIndex blockIndex, WILIndex succIndex);

/* ------------------------------------------------------------
   Dominance Analysis
   ------------------------------------------------------------ */

WILDomTree* wilComputeDominance(const WILCFG* cfg);
void        wilDestroyDomTree(WILDomTree* tree);

/* Query dominance: returns 1 if blockA dominates blockB */
int wilDominates(const WILDomTree* tree, WILIndex blockA, WILIndex blockB);

/* Get immediate dominator of a block (returns -1 if entry block) */
long wilImmediateDominator(const WILDomTree* tree, WILIndex block);

/* Get dominator tree children */
WILIndex  wilDomTreeChildCount(const WILDomTree* tree, WILIndex block);
WILIndex  wilDomTreeChild(const WILDomTree* tree, WILIndex block, WILIndex childIndex);

/* ------------------------------------------------------------
   Scheduling (per lazy-sparse-prop paper)
   ------------------------------------------------------------ */

/* Compute schedule: assigns nodes to blocks and orders them */
WILSchedule* wilComputeSchedule(WILContext* ctx, const WILCFG* cfg);
void         wilDestroySchedule(WILSchedule* schedule);

/* Query schedule */
WILIndex wilScheduleBlockCount(const WILSchedule* schedule);
WILIndex wilScheduleNodeCount(const WILSchedule* schedule, WILIndex blockIndex);
WILNode* wilScheduleNode(const WILSchedule* schedule, WILIndex blockIndex, WILIndex nodeIndex);

/* Get block assignment for a node (returns -1 if unscheduled) */
long wilNodeBlock(const WILSchedule* schedule, const WILNode* node);

/* ------------------------------------------------------------
   Verification and Optimization
   ------------------------------------------------------------ */

int  wilVerifyGraph(const WILContext* ctx);
void wilRunSparsePropagation(WILContext* ctx);
void wilRunCSE(WILContext* ctx);
void wilRunDCE(WILContext* ctx);

/* ------------------------------------------------------------
   Debug Printing
   ------------------------------------------------------------ */

void wilPrintNode(const WILNode* node);
void wilPrintGraph(const WILContext* ctx);
void wilPrintCFG(const WILCFG* cfg);
void wilPrintSchedule(const WILSchedule* schedule);
void wilPrintDomTree(const WILDomTree* tree);

WIRBLE_END_DECLS

#endif /* WIRBLE_WIL_H */

