#include "frontend.h"

#include <wirble/wirble-mal.h>
#include <wirble/wirble-mds.h>
#include <wirble/wirble-tos.h>
#include <wirble/wirble-wil.h>
#include <wirble/wirble-wrs.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct WirbleFrontendParseNode
{
  WirbleExpr *expr;
} WirbleFrontendParseNode;

#define D_ParseNode_User WirbleFrontendParseNode
#include <dparse.h>

#include "src/wirble/wvm/wvm_encoder.h"

extern D_ParserTables parser_tables_gram;

#ifndef WIRBLE_STDRULE_DIR
#define WIRBLE_STDRULE_DIR "stdrewrite"
#endif

static char *
wirble_strdup (const char *text)
{
  size_t len;
  char *copy;

  if (text == NULL)
    {
      return NULL;
    }
  len = strlen (text);
  copy = (char *) malloc (len + 1u);
  if (copy != NULL)
    {
      memcpy (copy, text, len + 1u);
    }
  return copy;
}

static void
wirble_set_error (char **errorMessage, const char *message)
{
  if (errorMessage == NULL)
    {
      return;
    }
  free (*errorMessage);
  *errorMessage = wirble_strdup (message);
}

static void
wirble_set_errorf (char **errorMessage, const char *prefix, const char *detail)
{
  size_t prefixLen;
  size_t detailLen;
  char *buffer;

  if (errorMessage == NULL)
    {
      return;
    }
  prefixLen = prefix == NULL ? 0u : strlen (prefix);
  detailLen = detail == NULL ? 0u : strlen (detail);
  buffer = (char *) malloc (prefixLen + detailLen + 3u);
  if (buffer == NULL)
    {
      free (*errorMessage);
      *errorMessage = NULL;
      return;
    }
  buffer[0] = '\0';
  if (prefix != NULL)
    {
      memcpy (buffer, prefix, prefixLen);
      buffer[prefixLen] = '\0';
    }
  if (detail != NULL)
    {
      if (prefixLen != 0u)
        {
          memcpy (buffer + prefixLen, ": ", 2u);
          memcpy (buffer + prefixLen + 2u, detail, detailLen + 1u);
        }
      else
        {
          memcpy (buffer, detail, detailLen + 1u);
        }
    }
  free (*errorMessage);
  *errorMessage = buffer;
}

WirbleExpr *
wirbleExprInt (long long value)
{
  WirbleExpr *expr = (WirbleExpr *) calloc (1u, sizeof (*expr));
  if (expr != NULL)
    {
      expr->kind = WIRBLE_EXPR_INT;
      expr->as.intValue = value;
    }
  return expr;
}

WirbleExpr *
wirbleExprUnary (WirbleUnaryOp op, WirbleExpr *operand)
{
  WirbleExpr *expr = (WirbleExpr *) calloc (1u, sizeof (*expr));
  if (expr != NULL)
    {
      expr->kind = WIRBLE_EXPR_UNARY;
      expr->as.unary.op = op;
      expr->as.unary.operand = operand;
    }
  return expr;
}

WirbleExpr *
wirbleExprBinary (WirbleBinaryOp op, WirbleExpr *lhs, WirbleExpr *rhs)
{
  WirbleExpr *expr = (WirbleExpr *) calloc (1u, sizeof (*expr));
  if (expr != NULL)
    {
      expr->kind = WIRBLE_EXPR_BINARY;
      expr->as.binary.op = op;
      expr->as.binary.lhs = lhs;
      expr->as.binary.rhs = rhs;
    }
  return expr;
}

void
wirbleExprDestroy (WirbleExpr *expr)
{
  if (expr == NULL)
    {
      return;
    }
  switch (expr->kind)
    {
    case WIRBLE_EXPR_UNARY:
      wirbleExprDestroy (expr->as.unary.operand);
      break;
    case WIRBLE_EXPR_BINARY:
      wirbleExprDestroy (expr->as.binary.lhs);
      wirbleExprDestroy (expr->as.binary.rhs);
      break;
    default:
      break;
    }
  free (expr);
}

static int
wirble_expr_is_const (const WirbleExpr *expr, long long *outValue)
{
  long long operand;
  long long lhs;
  long long rhs;

  if (expr == NULL || outValue == NULL)
    {
      return 0;
    }
  switch (expr->kind)
    {
    case WIRBLE_EXPR_INT:
      *outValue = expr->as.intValue;
      return 1;
    case WIRBLE_EXPR_UNARY:
      if (!wirble_expr_is_const (expr->as.unary.operand, &operand))
        {
          return 0;
        }
      switch (expr->as.unary.op)
        {
        case WIRBLE_UNARY_NEG:
          *outValue = -operand;
          return 1;
        }
      return 0;
    case WIRBLE_EXPR_BINARY:
      if (!wirble_expr_is_const (expr->as.binary.lhs, &lhs)
          || !wirble_expr_is_const (expr->as.binary.rhs, &rhs))
        {
          return 0;
        }
      switch (expr->as.binary.op)
        {
        case WIRBLE_BINARY_ADD:
          *outValue = lhs + rhs;
          return 1;
        case WIRBLE_BINARY_SUB:
          *outValue = lhs - rhs;
          return 1;
        case WIRBLE_BINARY_MUL:
          *outValue = lhs * rhs;
          return 1;
        case WIRBLE_BINARY_DIV:
          if (rhs == 0)
            {
              return 0;
            }
          *outValue = lhs / rhs;
          return 1;
        }
      return 0;
    }
  return 0;
}

static WirbleExpr *
wirble_expr_fold_constants (WirbleExpr *expr)
{
  long long value;

  if (expr == NULL)
    {
      return NULL;
    }
  switch (expr->kind)
    {
    case WIRBLE_EXPR_UNARY:
      expr->as.unary.operand
          = wirble_expr_fold_constants (expr->as.unary.operand);
      break;
    case WIRBLE_EXPR_BINARY:
      expr->as.binary.lhs = wirble_expr_fold_constants (expr->as.binary.lhs);
      expr->as.binary.rhs = wirble_expr_fold_constants (expr->as.binary.rhs);
      break;
    default:
      break;
    }
  if (!wirble_expr_is_const (expr, &value) || expr->kind == WIRBLE_EXPR_INT)
    {
      return expr;
    }
  wirbleExprDestroy (expr);
  return wirbleExprInt (value);
}

char *
wirbleReadFile (const char *path, char **errorMessage)
{
  FILE *input;
  long size;
  size_t readCount;
  char *buffer;

  if (path == NULL)
    {
      wirble_set_error (errorMessage, "missing input path");
      return NULL;
    }
  input = fopen (path, "rb");
  if (input == NULL)
    {
      wirble_set_errorf (errorMessage, "unable to open input",
                         strerror (errno));
      return NULL;
    }
  if (fseek (input, 0L, SEEK_END) != 0)
    {
      fclose (input);
      wirble_set_errorf (errorMessage, "unable to seek input",
                         strerror (errno));
      return NULL;
    }
  size = ftell (input);
  if (size < 0L)
    {
      fclose (input);
      wirble_set_errorf (errorMessage, "unable to read input size",
                         strerror (errno));
      return NULL;
    }
  rewind (input);
  buffer = (char *) malloc ((size_t) size + 1u);
  if (buffer == NULL)
    {
      fclose (input);
      wirble_set_error (errorMessage, "out of memory while reading input");
      return NULL;
    }
  readCount = fread (buffer, 1u, (size_t) size, input);
  fclose (input);
  buffer[readCount] = '\0';
  return buffer;
}

void
wirbleFreeError (char *errorMessage)
{
  free (errorMessage);
}

void
wirbleInitCompileOptions (WirbleCompileOptions *options)
{
  if (options == NULL)
    {
      return;
    }
  memset (options, 0, sizeof (*options));
  options->target = "x86_64";
  options->wrsRulesPath = WIRBLE_STDRULE_DIR "/arithmetic.wrs";
  options->mrsRulesPath = WIRBLE_STDRULE_DIR "/mal_peephole.wrs";
  options->optimizationLevel = 2u;
  options->verifyIR = 1;
}

void
wirblePipelineArtifactsInit (WirblePipelineArtifacts *artifacts)
{
  if (artifacts == NULL)
    {
      return;
    }
  memset (artifacts, 0, sizeof (*artifacts));
}

void
wirblePipelineArtifactsDestroy (WirblePipelineArtifacts *artifacts)
{
  if (artifacts == NULL)
    {
      return;
    }
  wvmFreePrototype (NULL, artifacts->wvmPrototype);
  artifacts->wvmPrototype = NULL;
  tos_destroy (artifacts->tosContext);
  artifacts->tosContext = NULL;
  tosDestroyProgram (artifacts->tosProgram);
  artifacts->tosProgram = NULL;
  mdsDestroyProgram (artifacts->mdsProgram);
  artifacts->mdsProgram = NULL;
  mdsDestroySelector (artifacts->mdsSelector);
  artifacts->mdsSelector = NULL;
  mdsDestroyMachine (artifacts->mdsMachine);
  artifacts->mdsMachine = NULL;
  mal_module_destroy (artifacts->malModule);
  artifacts->malModule = NULL;
  wil_context_destroy (artifacts->wilContext);
  artifacts->wilContext = NULL;
  wirbleExprDestroy (artifacts->expr);
  artifacts->expr = NULL;
  artifacts->wilStart = NULL;
  artifacts->wilValue = NULL;
  artifacts->wilNormalizedValue = NULL;
}

static const char *
wirble_machine_path (const char *target)
{
  if (target == NULL || strcmp (target, "x86_64") == 0)
    {
      return "machines/x86_64.json";
    }
  if (strcmp (target, "aarch64") == 0)
    {
      return "machines/aarch64.yaml";
    }
  if (strcmp (target, "wasm") == 0)
    {
      return "machines/wasm.xml";
    }
  return NULL;
}

static MDSSpecFormat
wirble_machine_format (const char *target)
{
  if (target != NULL && strcmp (target, "aarch64") == 0)
    {
      return MDS_FORMAT_YAML;
    }
  if (target != NULL && strcmp (target, "wasm") == 0)
    {
      return MDS_FORMAT_XML;
    }
  return MDS_FORMAT_JSON;
}

static const char *
wirble_target_or_default (const WirbleCompileOptions *options)
{
  if (options == NULL || options->target == NULL || options->target[0] == '\0')
    {
      return "x86_64";
    }
  return options->target;
}

static WirbleExpr *
wirble_parse_source (const char *source, char **errorMessage)
{
  D_Parser *parser;
  D_ParseNode *root;
  char *buffer;
  WirbleExpr *expr;

  parser = NULL;
  root = NULL;
  buffer = NULL;
  expr = NULL;
  if (source == NULL)
    {
      wirble_set_error (errorMessage, "missing source input");
      return NULL;
    }
  buffer = wirble_strdup (source);
  if (buffer == NULL)
    {
      wirble_set_error (errorMessage, "out of memory while copying source");
      return NULL;
    }
  parser = new_D_Parser (&parser_tables_gram, sizeof (WirbleFrontendParseNode));
  if (parser == NULL)
    {
      free (buffer);
      wirble_set_error (errorMessage, "unable to create dparser instance");
      return NULL;
    }
  parser->save_parse_tree = 1;
  parser->loc.pathname = "<input>";
  parser->loc.line = 1;
  parser->loc.col = 0;
  root = dparse (parser, buffer, (int) strlen (buffer));
  if (root == NULL || parser->syntax_errors || root->user.expr == NULL)
    {
      char message[128];
      snprintf (message, sizeof (message), "parse error near line %d column %d",
                parser->loc.line, parser->loc.col);
      wirble_set_error (errorMessage, message);
      if (root != NULL)
        {
          free_D_ParseNode (parser, root);
        }
      free_D_Parser (parser);
      free (buffer);
      return NULL;
    }
  expr = root->user.expr;
  root->user.expr = NULL;
  free_D_ParseNode (parser, root);
  free_D_Parser (parser);
  free (buffer);
  return wirble_expr_fold_constants (expr);
}

static WILNode *
wirble_lower_expr (WILContext *ctx, const WirbleExpr *expr)
{
  WILNode *lhs;
  WILNode *rhs;

  if (ctx == NULL || expr == NULL)
    {
      return NULL;
    }
  switch (expr->kind)
    {
    case WIRBLE_EXPR_INT:
      return wil_make_const_int (ctx, expr->as.intValue, WIL_TYPE_I32);
    case WIRBLE_EXPR_UNARY:
      lhs = wirble_lower_expr (ctx, expr->as.unary.operand);
      if (lhs == NULL)
        {
          return NULL;
        }
      return expr->as.unary.op == WIRBLE_UNARY_NEG ? wil_make_neg (ctx, lhs)
                                                   : NULL;
    case WIRBLE_EXPR_BINARY:
      lhs = wirble_lower_expr (ctx, expr->as.binary.lhs);
      rhs = wirble_lower_expr (ctx, expr->as.binary.rhs);
      if (lhs == NULL || rhs == NULL)
        {
          return NULL;
        }
      switch (expr->as.binary.op)
        {
        case WIRBLE_BINARY_ADD: return wil_make_add (ctx, lhs, rhs);
        case WIRBLE_BINARY_SUB: return wil_make_sub (ctx, lhs, rhs);
        case WIRBLE_BINARY_MUL: return wil_make_mul (ctx, lhs, rhs);
        case WIRBLE_BINARY_DIV: return wil_make_div (ctx, lhs, rhs);
        }
      break;
    }
  return NULL;
}

int
wirbleBuildPipelineArtifacts (const char *source, const char *sourceName,
                              const WirbleCompileOptions *options,
                              WirblePipelineArtifacts *artifacts,
                              char **errorMessage)
{
  WILRewriteSystem *wrs;
  MALRewriteSystem *mrs;
  const char *machinePath;
  const char *target;
  unsigned optimizationLevel;
  int verifyIR;
  MALFunction *function;

  if (artifacts == NULL)
    {
      wirble_set_error (errorMessage, "missing pipeline artifact storage");
      return 0;
    }

  wirblePipelineArtifactsInit (artifacts);
  wrs = NULL;
  mrs = NULL;
  target = wirble_target_or_default (options);
  optimizationLevel = options == NULL ? 2u : options->optimizationLevel;
  verifyIR = options == NULL ? 1 : options->verifyIR;
  machinePath = wirble_machine_path (target);
  if (machinePath == NULL)
    {
      wirble_set_error (errorMessage, "unsupported target");
      return 0;
    }

  artifacts->expr = wirble_parse_source (source, errorMessage);
  if (artifacts->expr == NULL)
    {
      return 0;
    }
  artifacts->wilContext = wil_context_create ();
  if (artifacts->wilContext == NULL)
    {
      wirble_set_error (errorMessage, "unable to create WIL context");
      goto cleanup;
    }
  artifacts->wilStart = wil_make_start (artifacts->wilContext);
  artifacts->wilValue = wirble_lower_expr (artifacts->wilContext, artifacts->expr);
  if (artifacts->wilStart == NULL || artifacts->wilValue == NULL)
    {
      wirble_set_error (errorMessage, "unable to lower expression to WIL");
      goto cleanup;
    }
  if (optimizationLevel > 0u)
    {
      wrs = wilRewriteSystemCreate (artifacts->wilContext);
      if (wrs != NULL)
        {
          if (options != NULL && options->wrsRulesPath != NULL)
            {
              WRSFile *wrsFile
                  = wrsFileLoad (artifacts->wilContext, options->wrsRulesPath);
              if (wrsFile == NULL)
                {
                  wirble_set_error (errorMessage, "unable to load WRS rules");
                  goto cleanup;
                }
              wrsFileApplyToSystem (wrsFile, wrs);
              wrsFileDestroy (artifacts->wilContext, wrsFile);
            }
          else
            {
              wilRewriteSystemAddAllBuiltinRules (wrs);
            }
        }
    }
  artifacts->wilNormalizedValue
      = wrs == NULL ? artifacts->wilValue
                    : wilRewriteSystemNormalize (wrs, artifacts->wilValue, 16u);
  if (artifacts->wilNormalizedValue == NULL)
    {
      artifacts->wilNormalizedValue = artifacts->wilValue;
    }
  if (verifyIR && !wil_verify_graph (artifacts->wilContext))
    {
      wirble_set_errorf (errorMessage, "WIL verification failed",
                         wil_verify_error (artifacts->wilContext) == NULL
                             ? "invalid graph"
                             : wil_verify_error (artifacts->wilContext));
      goto cleanup;
    }
  if (options != NULL && options->dumpWIL)
    {
      wil_print_graph (wil_context_graph (artifacts->wilContext), stdout);
    }
  if (wil_make_return (artifacts->wilContext, artifacts->wilStart,
                       artifacts->wilNormalizedValue)
      == NULL)
    {
      wirble_set_error (errorMessage, "unable to build WIL return node");
      goto cleanup;
    }
  artifacts->malModule = mal_module_create (target);
  if (artifacts->malModule == NULL
      || !malLowerFromWIL (artifacts->malModule,
                           wil_context_graph (artifacts->wilContext)))
    {
      wirble_set_error (errorMessage, "unable to lower WIL to MAL");
      goto cleanup;
    }
  if (verifyIR)
    {
      for (function = artifacts->malModule->functions; function != NULL;
           function = function->next)
        {
          if (!mal_verify_ssa (function))
            {
              wirble_set_error (errorMessage, "MAL verification failed");
              goto cleanup;
            }
        }
    }
  if (optimizationLevel > 0u)
    {
      mrs = mal_mrs_load (options == NULL ? NULL : options->mrsRulesPath);
      if (mrs == NULL)
        {
          mrs = mal_mrs_load (NULL);
        }
      if (mrs != NULL && artifacts->malModule->functions != NULL)
        {
          for (function = artifacts->malModule->functions; function != NULL;
               function = function->next)
            {
              (void) malApplyPeepholeToFixpoint (mrs, function);
              if (verifyIR && !mal_verify_ssa (function))
                {
                  wirble_set_error (errorMessage,
                                    "MAL verification failed after peephole");
                  goto cleanup;
                }
            }
        }
    }
  if (options != NULL && options->dumpMAL)
    {
      mal_dump_module (artifacts->malModule);
    }
  artifacts->mdsMachine
      = mdsLoadMachine (machinePath, wirble_machine_format (target));
  if (artifacts->mdsMachine == NULL
      || !mdsValidateMachine (artifacts->mdsMachine))
    {
      wirble_set_error (errorMessage, "unable to load target machine");
      goto cleanup;
    }
  artifacts->mdsSelector = mdsCreateSelector (artifacts->mdsMachine);
  if (artifacts->mdsSelector == NULL)
    {
      wirble_set_error (errorMessage, "unable to create instruction selector");
      goto cleanup;
    }
  artifacts->mdsProgram
      = malLowerToMDS (artifacts->malModule, artifacts->mdsSelector);
  if (artifacts->mdsProgram == NULL)
    {
      wirble_set_error (errorMessage, "unable to lower MAL to MDS");
      goto cleanup;
    }
  if (options != NULL && options->dumpMDS)
    {
      mdsPrintProgram (artifacts->mdsProgram);
    }
  artifacts->tosProgram = tosBuildFromMDS (artifacts->mdsProgram);
  if (artifacts->tosProgram == NULL)
    {
      wirble_set_error (errorMessage, "unable to build TOS program");
      goto cleanup;
    }
  if (optimizationLevel > 0u)
    {
      artifacts->tosContext = tos_create (NULL, artifacts->mdsMachine);
      if (artifacts->tosContext == NULL
          || !tos_optimize_program (artifacts->tosContext, artifacts->tosProgram))
        {
          wirble_set_error (errorMessage, "unable to optimize TOS program");
          goto cleanup;
        }
    }
  else if (!tosFinalizeProgram (artifacts->tosProgram))
    {
      wirble_set_error (errorMessage, "unable to finalize TOS program");
      goto cleanup;
    }
  if (verifyIR && !tosValidateProgram (artifacts->tosProgram))
    {
      wirble_set_error (errorMessage, "TOS verification failed");
      goto cleanup;
    }
  if (options != NULL && options->dumpTOS)
    {
      tosPrintProgram (artifacts->tosProgram);
    }
  artifacts->wvmPrototype
      = wvmPrototypeFromTOS (artifacts->tosProgram, "main",
                             sourceName == NULL ? "<input>" : sourceName);
  if (artifacts->wvmPrototype == NULL)
    {
      wirble_set_error (errorMessage, "unable to emit WVM bytecode");
      goto cleanup;
    }
  if (verifyIR && !wvmVerifyPrototype (NULL, artifacts->wvmPrototype))
    {
      wirble_set_error (errorMessage, "WVM verification failed");
      goto cleanup;
    }
  if (options != NULL && options->dumpWVM)
    {
      char *disasm = wvmDisassemble (NULL, artifacts->wvmPrototype);
      if (disasm != NULL)
        {
          fputs (disasm, stdout);
          free (disasm);
        }
    }
  if (wrs != NULL)
    {
      wilRewriteSystemDestroy (artifacts->wilContext, wrs);
    }
  if (mrs != NULL)
    {
      mal_mrs_destroy (mrs);
    }
  return 1;

cleanup:
  if (wrs != NULL && artifacts->wilContext != NULL)
    {
      wilRewriteSystemDestroy (artifacts->wilContext, wrs);
    }
  if (mrs != NULL)
    {
      mal_mrs_destroy (mrs);
    }
  wirblePipelineArtifactsDestroy (artifacts);
  if (errorMessage != NULL && *errorMessage == NULL)
    {
      wirble_set_error (errorMessage, "compilation failed");
    }
  return 0;
}

WVMPrototype *
wirbleCompileSourceToPrototype (const char *source, const char *sourceName,
                                const WirbleCompileOptions *options,
                                char **errorMessage)
{
  WirblePipelineArtifacts artifacts;
  WVMPrototype *proto;
  proto = NULL;
  wirblePipelineArtifactsInit (&artifacts);
  if (!wirbleBuildPipelineArtifacts (source, sourceName, options, &artifacts,
                                     errorMessage))
    {
      return NULL;
    }
  proto = artifacts.wvmPrototype;
  artifacts.wvmPrototype = NULL;
  wirblePipelineArtifactsDestroy (&artifacts);
  return proto;
}

int
wirbleExecuteSource (const char *source, const char *sourceName,
                     const WirbleCompileOptions *options, WVMValue *outValue,
                     char **errorMessage)
{
  WVMPrototype *proto;
  WVMState *vm;
  const char *runtimeError;

  proto = wirbleCompileSourceToPrototype (source, sourceName, options, errorMessage);
  if (proto == NULL)
    {
      return 0;
    }
  vm = wvmCreate ();
  if (vm == NULL)
    {
      wvmFreePrototype (NULL, proto);
      wirble_set_error (errorMessage, "unable to create WVM runtime");
      return 0;
    }
  if (!wvmExecute (vm, proto))
    {
      runtimeError = wvmGetLastError (vm);
      wirble_set_errorf (errorMessage, "runtime error",
                         runtimeError == NULL ? "execution failed" : runtimeError);
      wvmDestroy (vm);
      wvmFreePrototype (NULL, proto);
      return 0;
    }
  if (outValue != NULL)
    {
      *outValue = wvmGetReg (vm, 0u);
    }
  wvmDestroy (vm);
  wvmFreePrototype (NULL, proto);
  return 1;
}
