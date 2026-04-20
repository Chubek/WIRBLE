#include "examples/common/frontend.h"

#include <wirble/wirble-wvm.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void
wirblecc_usage (const char *argv0)
{
  fprintf (stderr,
           "Usage: %s [-o output.wvmo] [-target x86_64|aarch64|wasm] input.wb\n",
           argv0);
}

int
main (int argc, char **argv)
{
  const char *inputPath = NULL;
  const char *outputPath = "a.out.wvmo";
  WirbleCompileOptions options;
  char *source = NULL;
  char *errorMessage = NULL;
  WVMPrototype *proto = NULL;
  int index;
  int status = 1;

  memset (&options, 0, sizeof (options));
  options.target = "x86_64";
  options.optimizationLevel = 2u;
  options.verifyIR = 1;

  for (index = 1; index < argc; ++index)
    {
      if (strncmp (argv[index], "-O", 2u) == 0)
        {
          const char *levelText = argv[index] + 2;
          if (*levelText == '\0' || levelText[1] != '\0' || *levelText < '0'
              || *levelText > '3')
            {
              wirblecc_usage (argv[0]);
              return 1;
            }
          options.optimizationLevel = (unsigned) (*levelText - '0');
          continue;
        }
      if (strcmp (argv[index], "-o") == 0)
        {
          if (index + 1 >= argc)
            {
              wirblecc_usage (argv[0]);
              return 1;
            }
          outputPath = argv[++index];
          continue;
        }
      if (strcmp (argv[index], "-target") == 0)
        {
          if (index + 1 >= argc)
            {
              wirblecc_usage (argv[0]);
              return 1;
            }
          options.target = argv[++index];
          continue;
        }
      if (argv[index][0] == '-')
        {
          wirblecc_usage (argv[0]);
          return 1;
        }
      inputPath = argv[index];
    }

  if (inputPath == NULL)
    {
      wirblecc_usage (argv[0]);
      return 1;
    }

  source = wirbleReadFile (inputPath, &errorMessage);
  if (source == NULL)
    {
      fprintf (stderr, "wirblecc: %s\n",
               errorMessage == NULL ? "unable to read input" : errorMessage);
      goto cleanup;
    }
  proto = wirbleCompileSourceToPrototype (source, inputPath, &options, &errorMessage);
  if (proto == NULL)
    {
      fprintf (stderr, "wirblecc: %s\n",
               errorMessage == NULL ? "compilation failed" : errorMessage);
      goto cleanup;
    }
  {
    WVMState *vm = wvmCreate ();
    if (vm == NULL)
      {
        fprintf (stderr, "wirblecc: unable to create WVM state\n");
        goto cleanup;
      }
    if (!wvmWriteObject (vm, proto, outputPath))
      {
        fprintf (stderr, "wirblecc: unable to write `%s`\n", outputPath);
        wvmDestroy (vm);
        goto cleanup;
      }
    wvmDestroy (vm);
  }
  printf ("wrote %s\n", outputPath);
  status = 0;

cleanup:
  free (source);
  wirbleFreeError (errorMessage);
  wvmFreePrototype (NULL, proto);
  return status;
}
