#include "mal_ir.h"

#include <stdio.h>

char *
mal_mrs_rule_to_string (const MALRewriteRule *rule)
{
  WirbleByteBuf buf;
  uint32_t i;
  char tmp[64];

  if (rule == NULL)
    {
      return NULL;
    }
  wirble_bytebuf_init (&buf);
  (void) wirble_bytebuf_append_cstr (&buf, rule->name == NULL ? "rule" : rule->name);
  (void) wirble_bytebuf_append_cstr (&buf, ": ");
  (void) wirble_bytebuf_append_cstr (&buf, mal_opcode_name (rule->matchOpcode));
  (void) wirble_bytebuf_append_cstr (&buf, " -> ");
  (void) wirble_bytebuf_append_cstr (&buf, mal_opcode_name (rule->replaceOpcode));
  for (i = 0u; i < rule->matchOperandCount; ++i)
    {
      snprintf (tmp, sizeof (tmp), i == 0u ? " (%u" : ", %u", i);
      (void) wirble_bytebuf_append_cstr (&buf, tmp);
    }
  if (rule->matchOperandCount != 0u)
    {
      (void) wirble_bytebuf_append_cstr (&buf, ")");
    }
  (void) wirble_bytebuf_append_byte (&buf, 0u);
  return (char *) buf.data;
}
