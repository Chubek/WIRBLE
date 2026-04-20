#include "mal_ir.h"

#include <string.h>

static int
mal_find_binding (const char *const *bindingNames, uint32_t bindingCount,
                  const char *name)
{
  uint32_t i;
  for (i = 0u; i < bindingCount; ++i)
    {
      if (bindingNames[i] != NULL && strcmp (bindingNames[i], name) == 0)
        {
          return (int) i;
        }
    }
  return -1;
}

int
mal_mrs_match_rule (const MALRewriteRule *rule, const MALInst *inst,
                    MALOperand *bindings, uint32_t bindingCap,
                    const char **bindingNames, uint32_t *bindingCount)
{
  uint32_t i;

  if (rule == NULL || inst == NULL || bindings == NULL || bindingNames == NULL
      || bindingCount == NULL)
    {
      return 0;
    }
  if (rule->matchOpcode != inst->opcode
      || rule->matchOperandCount != inst->operandCount)
    {
      return 0;
    }
  *bindingCount = 0u;
  for (i = 0u; i < rule->matchOperandCount; ++i)
    {
      const MALRuleOperandPattern *pattern = &rule->matchOperands[i];
      const MALOperand *operand = &inst->operands[i];
      int bindingIndex;
      switch (pattern->kind)
        {
        case MAL_RULE_OPERAND_ANY:
          break;
        case MAL_RULE_OPERAND_IMM_INT:
        case MAL_RULE_OPERAND_IMM_FLOAT:
        case MAL_RULE_OPERAND_BLOCK:
          if (!mal_operand_equal (&pattern->operand, operand))
            {
              return 0;
            }
          break;
        case MAL_RULE_OPERAND_VAR:
          bindingIndex = mal_find_binding (bindingNames, *bindingCount, pattern->name);
          if (bindingIndex >= 0)
            {
              if (!mal_operand_equal (&bindings[bindingIndex], operand))
                {
                  return 0;
                }
            }
          else
            {
              if (*bindingCount >= bindingCap)
                {
                  return 0;
                }
              bindingNames[*bindingCount] = pattern->name;
              bindings[*bindingCount] = *operand;
              ++(*bindingCount);
            }
          break;
        }
    }
  return 1;
}

MALInst *
mal_mrs_build_replacement (const MALRewriteRule *rule, const MALInst *inst,
                           const MALOperand *bindings,
                           const char *const *bindingNames,
                           uint32_t bindingCount)
{
  MALInst *replacement;
  uint32_t i;

  if (rule == NULL || inst == NULL)
    {
      return NULL;
    }
  replacement = mal_inst_create (rule->replaceOpcode, inst->type, inst->dst);
  if (replacement == NULL)
    {
      return NULL;
    }
  replacement->id = inst->id;
  for (i = 0u; i < rule->replaceOperandCount; ++i)
    {
      const MALRuleOperandPattern *pattern = &rule->replaceOperands[i];
      int bindingIndex;
      switch (pattern->kind)
        {
        case MAL_RULE_OPERAND_ANY:
          break;
        case MAL_RULE_OPERAND_VAR:
          bindingIndex = mal_find_binding (bindingNames, bindingCount, pattern->name);
          if (bindingIndex < 0)
            {
              return replacement;
            }
          mal_inst_add_operand (replacement, bindings[bindingIndex]);
          break;
        case MAL_RULE_OPERAND_IMM_INT:
        case MAL_RULE_OPERAND_IMM_FLOAT:
        case MAL_RULE_OPERAND_BLOCK:
          mal_inst_add_operand (replacement, pattern->operand);
          break;
        }
    }
  return replacement;
}
