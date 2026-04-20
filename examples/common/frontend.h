#ifndef WIRBLE_EXAMPLES_FRONTEND_H
#define WIRBLE_EXAMPLES_FRONTEND_H

#include <wirble/wirble-mal.h>
#include <wirble/wirble-mds.h>
#include <wirble/wirble-tos.h>
#include <wirble/wirble-wil.h>
#include <wirble/wirble-wvm.h>

typedef enum WirbleExprKind
{
  WIRBLE_EXPR_INT = 0,
  WIRBLE_EXPR_UNARY,
  WIRBLE_EXPR_BINARY
} WirbleExprKind;

typedef enum WirbleUnaryOp
{
  WIRBLE_UNARY_NEG = 0
} WirbleUnaryOp;

typedef enum WirbleBinaryOp
{
  WIRBLE_BINARY_ADD = 0,
  WIRBLE_BINARY_SUB,
  WIRBLE_BINARY_MUL,
  WIRBLE_BINARY_DIV
} WirbleBinaryOp;

typedef struct WirbleExpr WirbleExpr;

struct WirbleExpr
{
  WirbleExprKind kind;
  union
  {
    long long intValue;
    struct
    {
      WirbleUnaryOp op;
      WirbleExpr *operand;
    } unary;
    struct
    {
      WirbleBinaryOp op;
      WirbleExpr *lhs;
      WirbleExpr *rhs;
    } binary;
  } as;
};

WirbleExpr *wirbleExprInt (long long value);
WirbleExpr *wirbleExprUnary (WirbleUnaryOp op, WirbleExpr *operand);
WirbleExpr *wirbleExprBinary (WirbleBinaryOp op, WirbleExpr *lhs,
                              WirbleExpr *rhs);
void wirbleExprDestroy (WirbleExpr *expr);

char *wirbleReadFile (const char *path, char **errorMessage);
void wirbleFreeError (char *errorMessage);

typedef struct WirbleCompileOptions
{
  const char *target;
  const char *wrsRulesPath;
  const char *mrsRulesPath;
  unsigned optimizationLevel;
  int verifyIR;
  int dumpWIL;
  int dumpMAL;
  int dumpMDS;
  int dumpTOS;
  int dumpWVM;
} WirbleCompileOptions;

typedef struct WirblePipelineArtifacts
{
  WirbleExpr *expr;
  WILContext *wilContext;
  WILNode *wilStart;
  WILNode *wilValue;
  WILNode *wilNormalizedValue;
  MALModule *malModule;
  MDSMachine *mdsMachine;
  MDSInstrSelector *mdsSelector;
  MDSProgram *mdsProgram;
  TOSProgram *tosProgram;
  TOSContext *tosContext;
  WVMPrototype *wvmPrototype;
} WirblePipelineArtifacts;

void wirbleInitCompileOptions (WirbleCompileOptions *options);
void wirblePipelineArtifactsInit (WirblePipelineArtifacts *artifacts);
void wirblePipelineArtifactsDestroy (WirblePipelineArtifacts *artifacts);
int wirbleBuildPipelineArtifacts (const char *source, const char *sourceName,
                                  const WirbleCompileOptions *options,
                                  WirblePipelineArtifacts *artifacts,
                                  char **errorMessage);

WVMPrototype *wirbleCompileSourceToPrototype (const char *source,
                                              const char *sourceName,
                                              const WirbleCompileOptions *options,
                                              char **errorMessage);
int wirbleExecuteSource (const char *source, const char *sourceName,
                         const WirbleCompileOptions *options,
                         WVMValue *outValue,
                         char **errorMessage);

#endif
