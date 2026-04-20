#include "wil_nodes.h"

#include <stdio.h>

static int
wil_verify_node (const WILContext *ctx, const WILNode *node)
{
  WILIndex input_index;
  WILIndex use_index;

  if (node == NULL)
    {
      wil_context_set_verify_errorf ((WILContext *) ctx, "graph contains a null node entry");
      return 0;
    }

  if (node->context != ctx)
    {
      wil_context_set_verify_errorf ((WILContext *) ctx,
                                     "node %lu belongs to a different context",
                                     (unsigned long) node->id);
      return 0;
    }

  if (node->id == 0ul)
    {
      wil_context_set_verify_errorf ((WILContext *) ctx, "node has an invalid id");
      return 0;
    }

  for (input_index = 0u; input_index < node->input_count; ++input_index)
    {
      WILNode *input = node->inputs[input_index].node;
      int found = 0;

      if (input == NULL)
        {
          wil_context_set_verify_errorf ((WILContext *) ctx,
                                         "node %lu has a null input at slot %lu",
                                         (unsigned long) node->id,
                                         (unsigned long) input_index);
          return 0;
        }

      for (use_index = 0u; use_index < input->use_count; ++use_index)
        {
          if (input->uses[use_index] == node)
            {
              found = 1;
              break;
            }
        }

      if (!found)
        {
          wil_context_set_verify_errorf ((WILContext *) ctx,
                                         "node %lu input %lu is missing a reverse use",
                                         (unsigned long) node->id,
                                         (unsigned long) input_index);
          return 0;
        }
    }

  for (use_index = 0u; use_index < node->use_count; ++use_index)
    {
      WILNode *user = node->uses[use_index];
      WILIndex user_input_index;
      int found = 0;

      if (user == NULL)
        {
          wil_context_set_verify_errorf ((WILContext *) ctx,
                                         "node %lu has a null use entry",
                                         (unsigned long) node->id);
          return 0;
        }

      for (user_input_index = 0u; user_input_index < user->input_count;
           ++user_input_index)
        {
          if (user->inputs[user_input_index].node == node)
            {
              found = 1;
              break;
            }
        }

      if (!found)
        {
          wil_context_set_verify_errorf ((WILContext *) ctx,
                                         "node %lu use list references node %lu without matching input",
                                         (unsigned long) node->id,
                                         (unsigned long) user->id);
          return 0;
        }
    }

  return 1;
}

int
wil_verify_graph (const WILContext *ctx)
{
  WILIndex index;

  if (ctx == NULL)
    {
      return 0;
    }

  wil_context_clear_verify_error ((WILContext *) ctx);
  for (index = 0u; index < ctx->graph.node_count; ++index)
    {
      if (!wil_verify_node (ctx, ctx->graph.nodes[index]))
        {
          return 0;
        }
    }

  return 1;
}
