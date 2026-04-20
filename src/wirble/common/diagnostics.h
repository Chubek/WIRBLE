#ifndef WIRBLE_COMMON_DIAGNOSTICS_H
#define WIRBLE_COMMON_DIAGNOSTICS_H

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

typedef enum WirbleDiagSeverity
{
  WIRBLE_DIAG_NOTE = 0,
  WIRBLE_DIAG_WARNING,
  WIRBLE_DIAG_ERROR
} WirbleDiagSeverity;

typedef struct WirbleDiagnostics
{
  FILE *stream;
  const char *source_name;
  size_t note_count;
  size_t warning_count;
  size_t error_count;
} WirbleDiagnostics;

void wirble_diag_init (WirbleDiagnostics *diag, FILE *stream,
                       const char *source_name);
void wirble_diag_set_stream (WirbleDiagnostics *diag, FILE *stream);
void wirble_diag_set_source (WirbleDiagnostics *diag, const char *source_name);

void wirble_diag_report (WirbleDiagnostics *diag, WirbleDiagSeverity severity,
                         const char *location, const char *message);
void wirble_diag_reportf (WirbleDiagnostics *diag, WirbleDiagSeverity severity,
                          const char *location, const char *format, ...);
void wirble_diag_vreportf (WirbleDiagnostics *diag, WirbleDiagSeverity severity,
                           const char *location, const char *format,
                           va_list args);

size_t wirble_diag_count (const WirbleDiagnostics *diag,
                          WirbleDiagSeverity severity);
int wirble_diag_has_errors (const WirbleDiagnostics *diag);

void wirble_diag_emit (const char *message);

#endif /* WIRBLE_COMMON_DIAGNOSTICS_H */
