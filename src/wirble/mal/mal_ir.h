#ifndef WIRBLE_MAL_IR_H
#define WIRBLE_MAL_IR_H

#include <stdio.h>

#include <wirble/wirble-mal.h>
#include <wirble/wirble-wil.h>

#include "common/bytebuf.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MALRuleOperandPattern
{
  enum
  {
    MAL_RULE_OPERAND_ANY = 0,
    MAL_RULE_OPERAND_VAR,
    MAL_RULE_OPERAND_IMM_INT,
    MAL_RULE_OPERAND_IMM_FLOAT,
    MAL_RULE_OPERAND_BLOCK
  } kind;
  const char *name;
  MALOperand operand;
} MALRuleOperandPattern;

struct MALRewriteRule
{
  const char *name;
  MALOpcode matchOpcode;
  MALRuleOperandPattern *matchOperands;
  uint32_t matchOperandCount;
  MALOpcode replaceOpcode;
  MALRuleOperandPattern *replaceOperands;
  uint32_t replaceOperandCount;
};

void *mal_xcalloc (size_t count, size_t size);
char *mal_xstrdup (const char *text);
int mal_append_ptr (void **items, uint32_t itemSize, uint32_t *count,
                    uint32_t *capacity, const void *item);

MALType mal_type_from_wil (WILType type);
MALOpcode mal_opcode_from_wil (WILNodeKind kind, WILType type);
const char *mal_type_name (MALType type);
const char *mal_opcode_name (MALOpcode opcode);

MALOperand mal_operand_reg (MALReg reg, MALType type);
MALOperand mal_operand_imm_int (int64_t value, MALType type);
MALOperand mal_operand_imm_float (double value, MALType type);
MALOperand mal_operand_block (MALBlockId block);

int mal_operand_equal (const MALOperand *lhs, const MALOperand *rhs);
int mal_operand_is_zero (const MALOperand *operand);
int mal_operand_is_one (const MALOperand *operand);
int mal_inst_is_terminator (const MALInst *inst);
void mal_print_operand (FILE *out, const MALOperand *operand);
void mal_print_inst (FILE *out, const MALInst *inst);
void mal_print_function_to_file (FILE *out, const MALFunction *fn);
void mal_print_module_to_file (FILE *out, const MALModule *m);

int mal_mrs_match_rule (const MALRewriteRule *rule, const MALInst *inst,
                        MALOperand *bindings, uint32_t bindingCap,
                        const char **bindingNames, uint32_t *bindingCount);
MALInst *mal_mrs_build_replacement (const MALRewriteRule *rule,
                                    const MALInst *inst,
                                    const MALOperand *bindings,
                                    const char *const *bindingNames,
                                    uint32_t bindingCount);
char *mal_mrs_rule_to_string (const MALRewriteRule *rule);

#ifdef __cplusplus
}
#endif

#endif /* WIRBLE_MAL_IR_H */
