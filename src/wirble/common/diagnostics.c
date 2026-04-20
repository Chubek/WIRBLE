#include "diagnostics.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static const char *
wirble_diag_severity_name (WirbleDiagSeverity severity)
{
  switch (severity)
    {
    case WIRBLE_DIAG_NOTE:
      return "note";
    case WIRBLE_DIAG_WARNING:
      return "warning";
    case WIRBLE_DIAG_ERROR:
      return "error";
    default:
      return "unknown";
    }
}

static FILE *
wirble_diag_stream (WirbleDiagnostics *diag)
{
  if (diag == NULL || diag->stream == NULL)
    {
      return stderr;
    }

  return diag->stream;
}

static void
wirble_diag_increment (WirbleDiagnostics *diag, WirbleDiagSeverity severity)
{
  if (diag == NULL)
    {
      return;
    }

  switch (severity)
    {
    case WIRBLE_DIAG_NOTE:
      ++diag->note_count;
      break;
    case WIRBLE_DIAG_WARNING:
      ++diag->warning_count;
      break;
    case WIRBLE_DIAG_ERROR:
      ++diag->error_count;
      break;
    }
}

void
wirble_diag_init (WirbleDiagnostics *diag, FILE *stream,
                  const char *source_name)
{
  if (diag == NULL)
    {
      return;
    }

  diag->stream = stream;
  diag->source_name = source_name;
  diag->note_count = 0u;
  diag->warning_count = 0u;
  diag->error_count = 0u;
}

void
wirble_diag_set_stream (WirbleDiagnostics *diag, FILE *stream)
{
  if (diag != NULL)
    {
      diag->stream = stream;
    }
}

void
wirble_diag_set_source (WirbleDiagnostics *diag, const char *source_name)
{
  if (diag != NULL)
    {
      diag->source_name = source_name;
    }
}

void
wirble_diag_report (WirbleDiagnostics *diag, WirbleDiagSeverity severity,
                    const char *location, const char *message)
{
  FILE *stream;
  const char *source_name;

  stream = wirble_diag_stream (diag);
  source_name = (diag != NULL && diag->source_name != NULL)
                    ? diag->source_name
                    : "wirble";

  fprintf (stream, "%s", source_name);
  if (location != NULL && location[0] != '\0')
    {
      fprintf (stream, ":%s", location);
    }

  fprintf (stream, ": %s: %s\n", wirble_diag_severity_name (severity),
           message == NULL ? "" : message);
  fflush (stream);

  wirble_diag_increment (diag, severity);
}

void
wirble_diag_vreportf (WirbleDiagnostics *diag, WirbleDiagSeverity severity,
                      const char *location, const char *format, va_list args)
{
  char buffer[1024];
  int written;

  if (format == NULL)
    {
      wirble_diag_report (diag, severity, location, "");
      return;
    }

  written = vsnprintf (buffer, sizeof (buffer), format, args);
  if (written < 0)
    {
      wirble_diag_report (diag, severity, location, "formatting error");
      return;
    }

  if ((size_t) written >= sizeof (buffer))
    {
      buffer[sizeof (buffer) - 2u] = '.';
      buffer[sizeof (buffer) - 3u] = '.';
      buffer[sizeof (buffer) - 4u] = '.';
      buffer[sizeof (buffer) - 1u] = '\0';
    }

  wirble_diag_report (diag, severity, location, buffer);
}

void
wirble_diag_reportf (WirbleDiagnostics *diag, WirbleDiagSeverity severity,
                     const char *location, const char *format, ...)
{
  va_list args;

  va_start (args, format);
  wirble_diag_vreportf (diag, severity, location, format, args);
  va_end (args);
}

size_t
wirble_diag_count (const WirbleDiagnostics *diag, WirbleDiagSeverity severity)
{
  if (diag == NULL)
    {
      return 0u;
    }

  switch (severity)
    {
    case WIRBLE_DIAG_NOTE:
      return diag->note_count;
    case WIRBLE_DIAG_WARNING:
      return diag->warning_count;
    case WIRBLE_DIAG_ERROR:
      return diag->error_count;
    default:
      return 0u;
    }
}

int
wirble_diag_has_errors (const WirbleDiagnostics *diag)
{
  return diag != NULL && diag->error_count != 0u;
}

void
wirble_diag_emit (const char *message)
{
  WirbleDiagnostics diag;

  wirble_diag_init (&diag, stderr, "wirble");
  wirble_diag_report (&diag, WIRBLE_DIAG_NOTE, NULL, message);
}
