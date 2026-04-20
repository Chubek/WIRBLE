#include "examples/common/frontend.h"

#include <wirble/wirble-wvm.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void
usage (const char *argv0)
{
  fprintf (stderr,
           "Usage: %s [--output file.wvmo] [--debug] [--target t] input.wb\n",
           argv0);
}

int
main (int argc, char **argv)
{
  WirbleCompileOptions options;
  WVMPrototype *proto = NULL;
  WVMState *vm = NULL;
  const char *inputPath = NULL;
  const char *outputPath = "a.out.wvmo";
  char *source = NULL;
  char *errorMessage = NULL;
  int index;
  int status = 1;

  wirbleInitCompileOptions (&options);

  for (index = 1; index < argc; ++index)
    {
      if (strcmp (argv[index], "--output") == 0)
        {
          if (index + 1 >= argc)
            {
              usage (argv[0]);
              goto cleanup;
            }
          outputPath = argv[++index];
          continue;
        }
      if (strcmp (argv[index], "--debug") == 0)
        {
          options.dumpWVM = 1;
          continue;
        }
      if (strcmp (argv[index], "--target") == 0)
        {
          if (index + 1 >= argc)
            {
              usage (argv[0]);
              goto cleanup;
            }
          options.target = argv[++index];
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
      fprintf (stderr, "wvm-emit: %s\n",
               errorMessage == NULL ? "unable to read input" : errorMessage);
      goto cleanup;
    }
  proto = wirbleCompileSourceToPrototype (source, inputPath, &options, &errorMessage);
  if (proto == NULL)
    {
      fprintf (stderr, "wvm-emit: %s\n",
               errorMessage == NULL ? "emit failed" : errorMessage);
      goto cleanup;
    }
  vm = wvmCreate ();
  if (vm == NULL || !wvmWriteObject (vm, proto, outputPath))
    {
      fprintf (stderr, "wvm-emit: unable to write `%s`\n", outputPath);
      goto cleanup;
    }
  printf ("wrote %s\n", outputPath);
  status = 0;

cleanup:
  free (source);
  wirbleFreeError (errorMessage);
  wvmDestroy (vm);
  wvmFreePrototype (NULL, proto);
  return status;
}
