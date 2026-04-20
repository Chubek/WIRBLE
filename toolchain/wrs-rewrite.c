#include "examples/common/frontend.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void
usage (const char *argv0)
{
  fprintf (stderr,
           "Usage: %s [--rules path] [--fixpoint] [--debug] input.wb\n",
           argv0);
}

int
main (int argc, char **argv)
{
  WirbleCompileOptions options;
  WirblePipelineArtifacts artifacts;
  const char *inputPath = NULL;
  char *source = NULL;
  char *errorMessage = NULL;
  int index;
  int status = 1;

  wirbleInitCompileOptions (&options);
  options.optimizationLevel = 1u;
  options.dumpWIL = 1;
  wirblePipelineArtifactsInit (&artifacts);

  for (index = 1; index < argc; ++index)
    {
      if (strcmp (argv[index], "--rules") == 0)
        {
          if (index + 1 >= argc)
            {
              usage (argv[0]);
              goto cleanup;
            }
          options.wrsRulesPath = argv[++index];
          continue;
        }
      if (strcmp (argv[index], "--fixpoint") == 0 || strcmp (argv[index], "--debug") == 0)
        {
          continue;
        }
      if (argv[index][0] == '-')
        {
          usage (argv[0]);
          goto cleanup;
        }
      inputPath = argv[index];
    }

  if (inputPath == NULL)
    {
      usage (argv[0]);
      goto cleanup;
    }

  source = wirbleReadFile (inputPath, &errorMessage);
  if (source == NULL)
    {
      fprintf (stderr, "wrs-rewrite: %s\n",
               errorMessage == NULL ? "unable to read input" : errorMessage);
      goto cleanup;
    }
  if (!wirbleBuildPipelineArtifacts (source, inputPath, &options, &artifacts,
                                     &errorMessage))
    {
      fprintf (stderr, "wrs-rewrite: %s\n",
               errorMessage == NULL ? "rewrite failed" : errorMessage);
      goto cleanup;
    }
  status = 0;

cleanup:
  free (source);
  wirbleFreeError (errorMessage);
  wirblePipelineArtifactsDestroy (&artifacts);
  return status;
}
