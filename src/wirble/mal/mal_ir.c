#include "mal_ir.h"

#include <stdlib.h>
#include <string.h>

#include <wirble/wirble-mds.h>

void *
mal_xcalloc (size_t count, size_t size)
{
  return calloc (count, size);
}

char *
mal_xstrdup (const char *text)
{
  size_t size;
  char *copy;

  if (text == NULL)
    {
      return NULL;
    }
  size = strlen (text) + 1u;
  copy = (char *) malloc (size);
  if (copy != NULL)
    {
      memcpy (copy, text, size);
    }
  return copy;
}

int
mal_append_ptr (void **items, uint32_t itemSize, uint32_t *count,
                uint32_t *capacity, const void *item)
{
  char *grown;
  uint32_t newCap;

  if (items == NULL || count == NULL || capacity == NULL || item == NULL
      || itemSize == 0u)
    {
      return 0;
    }
  if (*count == *capacity)
    {
      newCap = *capacity == 0u ? 8u : (*capacity * 2u);
      grown = (char *) realloc (*items, (size_t) newCap * itemSize);
      if (grown == NULL)
        {
          return 0;
        }
      *items = grown;
      *capacity = newCap;
    }
  memcpy (((char *) (*items)) + ((size_t) (*count) * itemSize), item, itemSize);
  ++(*count);
  return 1;
}

static void
mal_free_inst (MALInst *inst)
{
  if (inst == NULL)
    {
      return;
    }
  free (inst->operands);
  free ((char *) inst->comment);
  free (inst);
}

static void
mal_free_block_members (MALBlock *block)
{
  MALInst *inst;
  MALInst *next;

  if (block == NULL)
    {
      return;
    }
  for (inst = block->first; inst != NULL; inst = next)
    {
      next = inst->next;
      mal_free_inst (inst);
    }
  free (block->preds);
  free (block->succs);
  free (block->domChildren);
  free (block->liveIn);
  free (block->liveOut);
  free ((char *) block->name);
}

MALModule *
mal_module_create (const char *triple)
{
  MALModule *module = (MALModule *) mal_xcalloc (1u, sizeof (*module));
  if (module != NULL)
    {
      module->targetTriple = mal_xstrdup (triple == NULL ? "unknown" : triple);
    }
  return module;
}

void
mal_module_destroy (MALModule *m)
{
  MALFunction *fn;
  MALFunction *nextFn;
  MALBlock *block;
  MALBlock *nextBlock;

  if (m == NULL)
    {
      return;
    }
  for (fn = m->functions; fn != NULL; fn = nextFn)
    {
      nextFn = fn->next;
      for (block = fn->blocks; block != NULL; block = nextBlock)
        {
          nextBlock = block->next;
          mal_free_block_members (block);
          free (block);
        }
      free (fn->paramTypes);
      free ((char *) fn->name);
      free (fn);
    }
  free ((char *) m->targetTriple);
  free (m);
}

MALFunction *
mal_function_create (MALModule *m, const char *name, MALType retType)
{
  MALFunction *fn;
  MALFunction *tail;

  if (m == NULL)
    {
      return NULL;
    }
  fn = (MALFunction *) mal_xcalloc (1u, sizeof (*fn));
  if (fn == NULL)
    {
      return NULL;
    }
  fn->name = mal_xstrdup (name == NULL ? "anonymous" : name);
  fn->retType = retType;
  fn->nextSSAReg = 0u;
  if (m->functions == NULL)
    {
      m->functions = fn;
    }
  else
    {
      for (tail = m->functions; tail->next != NULL; tail = tail->next)
        {
        }
      tail->next = fn;
    }
  ++m->functionCount;
  return fn;
}

MALBlock *
mal_block_create (MALFunction *fn, const char *name)
{
  MALBlock *block;
  MALBlock *tail;

  if (fn == NULL)
    {
      return NULL;
    }
  block = (MALBlock *) mal_xcalloc (1u, sizeof (*block));
  if (block == NULL)
    {
      return NULL;
    }
  block->id = fn->blockCount;
  block->name = mal_xstrdup (name == NULL ? "block" : name);
  if (fn->blocks == NULL)
    {
      fn->blocks = block;
    }
  else
    {
      for (tail = fn->blocks; tail->next != NULL; tail = tail->next)
        {
        }
      tail->next = block;
    }
  ++fn->blockCount;
  return block;
}

MALReg
mal_new_reg (MALFunction *fn)
{
  if (fn == NULL)
    {
      return MAL_INVALID_REG;
    }
  return fn->nextSSAReg++;
}

MALInst *
mal_inst_create (MALOpcode opc, MALType ty, MALReg dst)
{
  MALInst *inst = (MALInst *) mal_xcalloc (1u, sizeof (*inst));
  if (inst != NULL)
    {
      inst->opcode = opc;
      inst->type = ty;
      inst->dst = dst;
      inst->id = MAL_INVALID_INST;
    }
  return inst;
}

void
mal_inst_add_operand (MALInst *inst, MALOperand op)
{
  if (inst == NULL)
    {
      return;
    }
  if (!mal_append_ptr ((void **) &inst->operands, sizeof (*inst->operands),
                       &inst->operandCount, &inst->operandCap, &op))
    {
      return;
    }
}

MALType
mal_type_from_wil (WILType type)
{
  switch (type)
    {
    case WIL_TYPE_I1: return MAL_TYPE_I1;
    case WIL_TYPE_I8: return MAL_TYPE_I8;
    case WIL_TYPE_I16: return MAL_TYPE_I16;
    case WIL_TYPE_I32: return MAL_TYPE_I32;
    case WIL_TYPE_I64: return MAL_TYPE_I64;
    case WIL_TYPE_F32: return MAL_TYPE_F32;
    case WIL_TYPE_F64: return MAL_TYPE_F64;
    case WIL_TYPE_PTR: return MAL_TYPE_PTR;
    case WIL_TYPE_VOID:
    case WIL_TYPE_CONTROL:
    default:
      return MAL_TYPE_VOID;
    }
}

MALOpcode
mal_opcode_from_wil (WILNodeKind kind, WILType type)
{
  switch (kind)
    {
    case WIL_NODE_ADD: return type == WIL_TYPE_F32 || type == WIL_TYPE_F64 ? MAL_OP_FADD : MAL_OP_ADD;
    case WIL_NODE_SUB: return type == WIL_TYPE_F32 || type == WIL_TYPE_F64 ? MAL_OP_FSUB : MAL_OP_SUB;
    case WIL_NODE_MUL: return type == WIL_TYPE_F32 || type == WIL_TYPE_F64 ? MAL_OP_FMUL : MAL_OP_MUL;
    case WIL_NODE_DIV: return type == WIL_TYPE_F32 || type == WIL_TYPE_F64 ? MAL_OP_FDIV : MAL_OP_SDIV;
    case WIL_NODE_MOD: return MAL_OP_SREM;
    case WIL_NODE_NEG: return type == WIL_TYPE_F32 || type == WIL_TYPE_F64 ? MAL_OP_FNEG : MAL_OP_NEG;
    case WIL_NODE_ABS: return type == WIL_TYPE_F32 || type == WIL_TYPE_F64 ? MAL_OP_FABS : MAL_OP_TARGET;
    case WIL_NODE_AND: return MAL_OP_AND;
    case WIL_NODE_OR: return MAL_OP_OR;
    case WIL_NODE_XOR: return MAL_OP_XOR;
    case WIL_NODE_NOT: return MAL_OP_NOT;
    case WIL_NODE_SHL: return MAL_OP_SHL;
    case WIL_NODE_SHR: return MAL_OP_ASHR;
    case WIL_NODE_EQ: return MAL_OP_ICMP_EQ;
    case WIL_NODE_NE: return MAL_OP_ICMP_NE;
    case WIL_NODE_LT: return MAL_OP_ICMP_SLT;
    case WIL_NODE_LE: return MAL_OP_ICMP_SLE;
    case WIL_NODE_GT: return MAL_OP_ICMP_SGT;
    case WIL_NODE_GE: return MAL_OP_ICMP_SGE;
    case WIL_NODE_SELECT: return MAL_OP_SELECT;
    case WIL_NODE_CAST: return MAL_OP_SEXT;
    case WIL_NODE_BITCAST: return MAL_OP_BITCAST;
    case WIL_NODE_RETURN: return MAL_OP_RET;
    default: return MAL_OP_TARGET;
    }
}

static int
mal_find_param_count (const WILGraph *src, MALType **paramTypes, uint32_t *count)
{
  WILIndex index;
  uint32_t paramIndex = 0u;
  MALType *types;

  if (paramTypes == NULL || count == NULL)
    {
      return 0;
    }
  for (index = 0u; index < wil_graph_node_count (src); ++index)
    {
      WILNode *node = wil_graph_get_node (src, index);
      if (node != NULL && wil_node_kind (node) == WIL_NODE_PARAM)
        {
          ++paramIndex;
        }
    }
  *count = paramIndex;
  if (*count == 0u)
    {
      *paramTypes = NULL;
      return 1;
    }
  types = (MALType *) mal_xcalloc (*count, sizeof (*types));
  if (types == NULL)
    {
      return 0;
    }
  paramIndex = 0u;
  for (index = 0u; index < wil_graph_node_count (src); ++index)
    {
      WILNode *node = wil_graph_get_node (src, index);
      if (node != NULL && wil_node_kind (node) == WIL_NODE_PARAM && paramIndex < *count)
        {
          types[paramIndex++] = mal_type_from_wil (wil_node_type (node));
        }
    }
  *paramTypes = types;
  return 1;
}

static MALOperand
mal_operand_from_wil_input (WILNode *input, MALReg *nodeRegs, WILIndex nodeCount)
{
  WILNodeId id;
  if (input == NULL)
    {
      return mal_operand_imm_int (0, MAL_TYPE_I32);
    }
  if (wil_node_is_const (input))
    {
      switch (wil_node_kind (input))
        {
        case WIL_NODE_CONST_FLOAT:
          return mal_operand_imm_float (wil_node_const_float (input),
                                        mal_type_from_wil (wil_node_type (input)));
        case WIL_NODE_CONST_BOOL:
          return mal_operand_imm_int (wil_node_const_bool (input), MAL_TYPE_I1);
        case WIL_NODE_CONST_INT:
        default:
          return mal_operand_imm_int (wil_node_const_int (input),
                                      mal_type_from_wil (wil_node_type (input)));
        }
    }
  id = wil_node_id (input);
  if (id == 0u || id > nodeCount)
    {
      return mal_operand_imm_int (0, MAL_TYPE_I32);
    }
  return mal_operand_reg (nodeRegs[id - 1u], mal_type_from_wil (wil_node_type (input)));
}

int
malLowerFromWIL (MALModule *dst, const WILGraph *src)
{
  MALFunction *fn;
  MALBlock *block;
  MALReg *nodeRegs;
  MALType *paramTypes;
  uint32_t paramCount;
  WILIndex index;
  MALInstrId nextId = 0u;

  if (dst == NULL || src == NULL)
    {
      return 0;
    }
  fn = mal_function_create (dst, "wil_main", MAL_TYPE_VOID);
  if (fn == NULL)
    {
      return 0;
    }
  block = mal_block_create (fn, "entry");
  if (block == NULL)
    {
      return 0;
    }
  if (!mal_find_param_count (src, &paramTypes, &paramCount))
    {
      return 0;
    }
  fn->paramTypes = paramTypes;
  fn->paramCount = paramCount;
  for (index = 0u; index < paramCount; ++index)
    {
      (void) mal_new_reg (fn);
    }

  nodeRegs = (MALReg *) mal_xcalloc (wil_graph_node_count (src), sizeof (*nodeRegs));
  if (nodeRegs == NULL && wil_graph_node_count (src) != 0u)
    {
      return 0;
    }
  {
    uint32_t nextParamReg = 0u;
    for (index = 0u; index < wil_graph_node_count (src); ++index)
      {
        WILNode *node = wil_graph_get_node (src, index);
        MALInst *inst;
        MALOpcode opcode;
        MALReg dstReg;
        WILIndex inIndex;

        if (node == NULL)
          {
            continue;
          }
        switch (wil_node_kind (node))
          {
          case WIL_NODE_START:
          case WIL_NODE_REGION:
          case WIL_NODE_IF:
          case WIL_NODE_LOOP:
          case WIL_NODE_JUMP:
          case WIL_NODE_PROJ:
          case WIL_NODE_CONST_INT:
          case WIL_NODE_CONST_FLOAT:
          case WIL_NODE_CONST_BOOL:
          case WIL_NODE_UNDEF:
            break;
          case WIL_NODE_PARAM:
            if (wil_node_id (node) != 0u
                && wil_node_id (node) <= wil_graph_node_count (src))
              {
                nodeRegs[wil_node_id (node) - 1u] = nextParamReg++;
              }
            break;
          case WIL_NODE_RETURN:
            if (wil_node_input_count (node) > 1u)
              {
                MALOperand valueOperand
                    = mal_operand_from_wil_input (wil_node_input (node, 1u),
                                                  nodeRegs,
                                                  wil_graph_node_count (src));
                if (valueOperand.kind != MAL_OPND_REG)
                  {
                    MALReg copyReg = mal_new_reg (fn);
                    MALInst *copyInst
                        = mal_inst_create (MAL_OP_COPY,
                                           mal_type_from_wil (
                                               wil_node_type (
                                                   wil_node_input (node, 1u))),
                                           copyReg);
                    if (copyInst == NULL)
                      {
                        free (nodeRegs);
                        return 0;
                      }
                    mal_inst_add_operand (copyInst, valueOperand);
                    copyInst->id = nextId++;
                    mal_block_insert_end (block, copyInst);
                    valueOperand = mal_operand_reg (
                        copyReg,
                        mal_type_from_wil (
                            wil_node_type (wil_node_input (node, 1u))));
                  }
                inst = mal_inst_create (MAL_OP_RET, MAL_TYPE_VOID,
                                        MAL_INVALID_REG);
                if (inst == NULL)
                  {
                    free (nodeRegs);
                    return 0;
                  }
                mal_inst_add_operand (inst, valueOperand);
                fn->retType
                    = mal_type_from_wil (wil_node_type (wil_node_input (node, 1u)));
              }
            else
              {
                inst = mal_inst_create (MAL_OP_RET, MAL_TYPE_VOID,
                                        MAL_INVALID_REG);
                if (inst == NULL)
                  {
                    free (nodeRegs);
                    return 0;
                  }
              }
            inst->id = nextId++;
            mal_block_insert_end (block, inst);
            break;
          default:
            opcode = mal_opcode_from_wil (wil_node_kind (node), wil_node_type (node));
            dstReg = mal_new_reg (fn);
            inst = mal_inst_create (opcode, mal_type_from_wil (wil_node_type (node)),
                                    dstReg);
            if (inst == NULL)
              {
                free (nodeRegs);
                return 0;
              }
            if (wil_node_kind (node) == WIL_NODE_CAST
                || wil_node_kind (node) == WIL_NODE_BITCAST)
              {
                mal_inst_add_operand (
                    inst,
                    mal_operand_from_wil_input (wil_node_input (node, 0u), nodeRegs,
                                                wil_graph_node_count (src)));
                mal_inst_add_operand (
                    inst, (MALOperand){ .kind = MAL_OPND_TYPE,
                                        .type = MAL_TYPE_VOID,
                                        .asType = mal_type_from_wil (wil_node_type (node)) });
              }
            else
              {
                for (inIndex = 0u; inIndex < wil_node_input_count (node); ++inIndex)
                  {
                    if (wil_node_input_edge_kind (node, inIndex) == WIL_EDGE_CONTROL)
                      {
                        continue;
                      }
                    mal_inst_add_operand (
                        inst,
                        mal_operand_from_wil_input (
                            wil_node_input (node, inIndex), nodeRegs,
                            wil_graph_node_count (src)));
                  }
              }
            inst->id = nextId++;
            mal_block_insert_end (block, inst);
            if (wil_node_id (node) != 0u
                && wil_node_id (node) <= wil_graph_node_count (src))
              {
                nodeRegs[wil_node_id (node) - 1u] = dstReg;
              }
            break;
          }
      }
  }
  free (nodeRegs);
  return 1;
}

const char *
mal_version_string (void)
{
  return "WIRBLE MAL stage7";
}

int mal_binarize (const struct MDSProgram *prog, const char *path,
                  const MALBinaryOpts *opts)
{ (void) prog; (void) path; (void) opts; return 0; }
int mal_link (const char *outputPath, const char **objectFiles,
              uint32_t objectCount)
{ (void) outputPath; (void) objectFiles; (void) objectCount; return 0; }
int mal_write_sexpr (const MALModule *m, const char *path)
{ (void) m; (void) path; return 0; }
MALModule *mal_read_sexpr (const char *path)
{ (void) path; return NULL; }
struct MDSProgram *malLowerToMDS (const MALModule *m,
                                  const struct MDSInstrSelector *isel)
{
  const MDSMachine *machine;

  if (m == NULL)
    {
      return NULL;
    }
  machine = isel != NULL ? isel->machine : m->mdsMachine;
  if (machine == NULL && m->targetTriple != NULL)
    {
      machine = mdsLoadMachine (m->targetTriple, MDS_FORMAT_JSON);
    }
  return mdsLowerFromMAL (m, isel, machine);
}
void mal_run_default_pipeline (MALFunction *fn, struct MDSMachine *machine,
                               struct MDSInstrSelector *isel)
{ (void) fn; (void) machine; (void) isel; }
void mal_optimize_function (MALFunction *fn)
{ (void) fn; }
int malRegisterAllocate (MALFunction *fn, const MALRAOptions *opts,
                         const struct MDSMachine *machine)
{ (void) fn; (void) opts; (void) machine; return 0; }
