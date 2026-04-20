#include <wirble/wirble-wvm.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void
usage (const char *argv0)
{
  fprintf (stderr, "Usage: %s [--trace] [--stack] input.wvmo\n", argv0);
}

int
main (int argc, char **argv)
{
  WVMState *vm = NULL;
  WVMPrototype *proto = NULL;
  const char *inputPath = NULL;
  char *rendered = NULL;
  int showStack = 0;
  int index;
  int status = 1;

  for (index = 1; index < argc; ++index)
    {
      if (strcmp (argv[index], "--trace") == 0)
        {
          continue;
        }
      if (strcmp (argv[index], "--stack") == 0)
        {
          showStack = 1;
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

  vm = wvmCreate ();
  if (vm == NULL)
    {
      fprintf (stderr, "wvm-run: unable to create runtime\n");
      goto cleanup;
    }
  proto = wvmReadObject (vm, inputPath);
  if (proto == NULL)
    {
      fprintf (stderr, "wvm-run: unable to read `%s`\n", inputPath);
      goto cleanup;
    }
  if (!wvmExecute (vm, proto))
    {
      fprintf (stderr, "wvm-run: %s\n",
               wvmGetLastError (vm) == NULL ? "execution failed"
                                            : wvmGetLastError (vm));
      goto cleanup;
    }
  rendered = wvmValueToString (vm, wvmGetReg (vm, 0u));
  if (rendered != NULL)
    {
      puts (rendered);
    }
  if (showStack)
    {
      wvmDumpState (vm, stdout);
    }
  status = 0;

cleanup:
  free (rendered);
  wvmFreePrototype (vm, proto);
  wvmDestroy (vm);
  return status;
}
