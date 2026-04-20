#include "examples/common/frontend.h"

#include <wirble/wirble-wvm.h>

#include <microrl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct IWirbleState
{
  WirbleCompileOptions options;
  int shouldExit;
} IWirbleState;

static IWirbleState *iwirble_state = NULL;

static void
iwirble_print (const char *text)
{
  if (text != NULL && strstr (text, "\033[") != NULL)
    {
      return;
    }
  fputs (text == NULL ? "" : text, stdout);
  fflush (stdout);
}

static void
iwirble_usage (const char *argv0)
{
  fprintf (stderr,
           "Usage: %s [-target x86_64|aarch64|wasm] [file]\n",
           argv0);
}

static int
iwirble_eval_and_print (const char *source, const char *sourceName,
                        const WirbleCompileOptions *options)
{
  WVMValue value;
  char *errorMessage = NULL;
  char *rendered;

  if (!wirbleExecuteSource (source, sourceName, options, &value, &errorMessage))
    {
      fprintf (stderr, "iwirble: %s\n",
               errorMessage == NULL ? "evaluation failed" : errorMessage);
      wirbleFreeError (errorMessage);
      return 0;
    }
  rendered = wvmValueToString (NULL, value);
  if (rendered == NULL)
    {
      fprintf (stderr, "iwirble: unable to render result\n");
      return 0;
    }
  printf ("%s\n", rendered);
  free (rendered);
  wirbleFreeError (errorMessage);
  return 1;
}

static int
iwirble_execute (int argc, const char *const *argv)
{
  char buffer[512];
  size_t offset;
  int index;

  if (argc <= 0 || iwirble_state == NULL)
    {
      return 0;
    }
  if (strcmp (argv[0], ":quit") == 0 || strcmp (argv[0], ":q") == 0)
    {
      iwirble_state->shouldExit = 1;
      return 0;
    }
  if (strcmp (argv[0], ":help") == 0)
    {
      puts (":help  show commands");
      puts (":opt N set optimization level (0-3)");
      puts (":quit  exit the REPL");
      return 0;
    }
  if (strcmp (argv[0], ":opt") == 0)
    {
      if (argc != 2 || argv[1][0] < '0' || argv[1][0] > '3' || argv[1][1] != '\0')
        {
          fprintf (stderr, "iwirble: usage: :opt 0|1|2|3\n");
          return 0;
        }
      iwirble_state->options.optimizationLevel = (unsigned) (argv[1][0] - '0');
      printf ("optimization level: O%u\n", iwirble_state->options.optimizationLevel);
      return 0;
    }
  offset = 0u;
  for (index = 0; index < argc; ++index)
    {
      size_t tokenLen = strlen (argv[index]);
      if (offset + tokenLen + 2u >= sizeof (buffer))
        {
          fprintf (stderr, "iwirble: input line too long\n");
          return 0;
        }
      memcpy (buffer + offset, argv[index], tokenLen);
      offset += tokenLen;
      if (index + 1 < argc)
        {
          buffer[offset++] = ' ';
        }
    }
  buffer[offset++] = ';';
  buffer[offset] = '\0';
  (void) iwirble_eval_and_print (buffer, "<repl>", &iwirble_state->options);
  return 0;
}

int
main (int argc, char **argv)
{
  const char *path = NULL;
  char *source = NULL;
  char line[256];
  microrl_t repl;
  IWirbleState state;
  int index;

  memset (&state, 0, sizeof (state));
  state.options.target = "x86_64";
  state.options.optimizationLevel = 2u;
  state.options.verifyIR = 1;

  for (index = 1; index < argc; ++index)
    {
      if (strncmp (argv[index], "-O", 2u) == 0)
        {
          const char *levelText = argv[index] + 2;
          if (*levelText == '\0' || levelText[1] != '\0' || *levelText < '0'
              || *levelText > '3')
            {
              iwirble_usage (argv[0]);
              return 1;
            }
          state.options.optimizationLevel = (unsigned) (*levelText - '0');
          continue;
        }
      if (strcmp (argv[index], "-target") == 0)
        {
          if (index + 1 >= argc)
            {
              iwirble_usage (argv[0]);
              return 1;
            }
          state.options.target = argv[++index];
          continue;
        }
      if (argv[index][0] == '-')
        {
          iwirble_usage (argv[0]);
          return 1;
        }
      path = argv[index];
    }

  if (path != NULL)
    {
      char *errorMessage = NULL;
      source = wirbleReadFile (path, &errorMessage);
      if (source == NULL)
        {
          fprintf (stderr, "iwirble: %s\n",
                   errorMessage == NULL ? "unable to read file" : errorMessage);
          wirbleFreeError (errorMessage);
          return 1;
        }
      if (!iwirble_eval_and_print (source, path, &state.options))
        {
          free (source);
          return 1;
        }
      free (source);
      return 0;
    }

  iwirble_state = &state;

  microrl_init (&repl, iwirble_print);
  microrl_set_execute_callback (&repl, iwirble_execute);
  repl.prompt_str = "iwirble> ";
  iwirble_print (repl.prompt_str);

  while (!state.shouldExit && fgets (line, sizeof (line), stdin) != NULL)
    {
      size_t pos;
      for (pos = 0u; line[pos] != '\0'; ++pos)
        {
          microrl_insert_char (&repl, line[pos]);
        }
    }
  return 0;
}
