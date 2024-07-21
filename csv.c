//  -*- Mode: C; -*-                                                       
// 
//  csv.c
// 
//  Copyright (C) Jamie A. Jennings, 2024

#include "csv.h"
#include "utils.h"

// -----------------------------------------------------------------------------
// Output file (raw data, per timed run)
// -----------------------------------------------------------------------------

void write_header(FILE *f) {
  for (int i = 0; i < (F_LAST - 1); i++)
    fprintf(f, "%s,", Headers[i]);
  fprintf(f, "%s\n", Headers[F_LAST - 1]);
  fflush(f);
}

#define SEP do {				\
    fputc(',', f);				\
  } while (0)
#define NEWLINE do {				\
    fputc('\n', f);				\
  } while (0)
#define WRITEFMT(f, fmt_string, val) do {	\
    fprintf((f), (fmt_string), (val));		\
  } while (0)
#define WRITESEP(f, fmt_idx, val) do {		\
    WRITEFMT(f, FieldFormats[fmt_idx], val);	\
    SEP;					\
  } while (0)
#define WRITELN(f, fmt_idx, val) do {		\
    WRITEFMT(f, FieldFormats[fmt_idx], val);	\
    NEWLINE;					\
  } while (0)

void write_line(FILE *f, const char *cmd, int code, struct rusage *usage) {
  char *escaped_cmd = escape(cmd);
  char *shell_cmd = shell ? escape(shell) : NULL;

  WRITESEP(f, F_CMD,   escaped_cmd);
  WRITESEP(f, F_EXIT,  code);
  if (shell_cmd) WRITESEP(f, F_SHELL, shell_cmd);
  else SEP;
  // User time in microseconds
  WRITESEP(f, F_USER, usertime(usage));
  // System time in microseconds
  WRITESEP(f, F_SYSTEM, systemtime(usage));
  // Max RSS in kibibytes
  // ru_maxrss (since Linux 2.6.32) This is the maximum resident set size used (in kilobytes).
  // When the above was written, a kilobyte meant 1024 bytes.
  WRITESEP(f, F_RSS, usage->ru_maxrss);
  // Page reclaims (count):
  // The number of page faults serviced without any I/O activity; here
  // I/O activity is avoided by “reclaiming” a page frame from the
  // list of pages awaiting reallocation.
  WRITESEP(f, F_RECLAIMS, usage->ru_minflt);
  // Page faults (count):
  // The number of page faults serviced that required I/O activity.
  WRITESEP(f, F_FAULTS, usage->ru_majflt);
  // Voluntary context switches:
  // The number of times a context switch resulted due to a process
  // voluntarily giving up the processor before its time slice was
  // completed (usually to await availability of a resource).
  WRITESEP(f, F_VCSW, usage->ru_nvcsw);
  // Involuntary context switches:
  // The number of times a context switch resulted due to a higher
  // priority process becoming runnable or because the current process
  // exceeded its time slice.
  WRITELN(f, F_ICSW, usage->ru_nivcsw);

  fflush(f);
  free(escaped_cmd);
  free(shell_cmd);
}

// -----------------------------------------------------------------------------
// Hyperfine-format file
// -----------------------------------------------------------------------------

void write_hf_header(FILE *f) {
  fprintf(f, "command,mean,stddev,median,user,system,min,max\n");
  fflush(f);
}

void write_hf_line(FILE *f, summary *s) {
  const double million = 1000.0 * 1000.0;
  // Command
  WRITEFMT(f, "%s", *(s->cmd) ? s->cmd : shell); SEP;
  // Mean total time (really Median because it's more useful)
  WRITEFMT(f, "%f", (double) s->total / million); SEP;
  // Stddev omitted until we know what to report for a log-normal distribution
  WRITEFMT(f, "%f", (double) 0.0); SEP;
  // Median total time (repeated to be compatible with hf format)
  WRITEFMT(f, "%f", (double) s->total / million); SEP;
  // User time in seconds as double 
  WRITEFMT(f, "%f", (double) s->user / million); SEP;
  // System time in seconds as double
  WRITEFMT(f, "%f", (double) s->system / million); SEP;
  // Min total time in seconds as double 
  WRITEFMT(f, "%f", (double) s->tmin / million); SEP;
  // Max total time in seconds as double
  WRITEFMT(f, "%f", (double) s->tmax / million); NEWLINE;
  fflush(f);
}




