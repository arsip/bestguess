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
  char *shell_cmd = escape(shell);

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
// Summary statistics file
// -----------------------------------------------------------------------------

void write_summary_header(FILE *f) {
  WRITEFMT(f, "%s,", "Command");
  WRITEFMT(f, "%s,", "Shell");
  WRITEFMT(f, "%s,", "Runs (ct)");
  WRITEFMT(f, "%s,", "Failed (ct)");
  WRITEFMT(f, "%s,", "User min (us)");
  WRITEFMT(f, "%s,", "User max (us)");
  WRITEFMT(f, "%s,", "User median (us)");
  WRITEFMT(f, "%s,", "User mode (us)");
  WRITEFMT(f, "%s,", "System min (us)");
  WRITEFMT(f, "%s,", "System max (us)");
  WRITEFMT(f, "%s,", "System median (us)");
  WRITEFMT(f, "%s,", "System mode (us)");
  WRITEFMT(f, "%s,", "Total min (us)");
  WRITEFMT(f, "%s,", "Total max (us)");
  WRITEFMT(f, "%s,", "Total median (us)");
  WRITEFMT(f, "%s,", "Total mode (us)");
  WRITEFMT(f, "%s,", "Max RSS min (bytes)");
  WRITEFMT(f, "%s,", "Max RSS max (bytes)");
  WRITEFMT(f, "%s,", "Max RSS median (bytes)");
  WRITEFMT(f, "%s,", "Max RSS mode (bytes)");
  WRITEFMT(f, "%s,", "Vol Ctx Sw min (ct)");
  WRITEFMT(f, "%s,", "Vol Ctx Sw max (ct)");
  WRITEFMT(f, "%s,", "Vol Ctx Sw median (ct)");
  WRITEFMT(f, "%s,", "Vol Ctx Sw mode (us)");
  WRITEFMT(f, "%s,", "Invol Ctx Sw min (ct)");
  WRITEFMT(f, "%s,", "Invol Ctx Sw max (ct)");
  WRITEFMT(f, "%s,", "Invol Ctx Sw median (ct)");
  WRITEFMT(f, "%s,", "Invol Ctx Sw mode (ct)");
  WRITEFMT(f, "%s,", "Total Ctx Sw min (ct)");
  WRITEFMT(f, "%s,", "Total Ctx Sw max (ct)");
  WRITEFMT(f, "%s,", "Total Ctx Sw median (ct)");
  WRITEFMT(f, "%s\n", "Total Ctx Sw mode (ct)");
  fflush(f);
}

void write_summary_line(FILE *f, summary *s) {
  char *escaped_cmd = escape(s->cmd);
  char *shell_cmd = escape(shell);
  WRITESEP(f, F_CMD, escaped_cmd);
  if (shell)
    WRITESEP(f, F_SHELL, shell_cmd);
  else
    SEP;
  WRITEFMT(f, "%d,", s->runs);
  WRITEFMT(f, "%d,", s->fail_count);
  WRITESEP(f, F_USER, s->user.min);
  WRITESEP(f, F_USER, s->user.max);
  WRITESEP(f, F_USER, s->user.median);
  WRITESEP(f, F_USER, s->user.mode);
  WRITESEP(f, F_SYSTEM, s->system.min);
  WRITESEP(f, F_SYSTEM, s->system.max);
  WRITESEP(f, F_SYSTEM, s->system.median);
  WRITESEP(f, F_SYSTEM, s->system.mode);
  WRITESEP(f, F_TOTAL, s->total.min);
  WRITESEP(f, F_TOTAL, s->total.max);
  WRITESEP(f, F_TOTAL, s->total.median);
  WRITESEP(f, F_TOTAL, s->total.mode);
  WRITESEP(f, F_RSS, s->rss.min);
  WRITESEP(f, F_RSS, s->rss.max);
  WRITESEP(f, F_RSS, s->rss.median);
  WRITESEP(f, F_RSS, s->rss.mode);
  WRITESEP(f, F_VCSW, s->vcsw.min);
  WRITESEP(f, F_VCSW, s->vcsw.max);
  WRITESEP(f, F_VCSW, s->vcsw.median);
  WRITESEP(f, F_VCSW, s->vcsw.mode);
  WRITESEP(f, F_ICSW, s->icsw.min);
  WRITESEP(f, F_ICSW, s->icsw.max);
  WRITESEP(f, F_ICSW, s->icsw.median);
  WRITESEP(f, F_ICSW, s->icsw.mode);
  WRITESEP(f, F_TCSW, s->tcsw.min);
  WRITESEP(f, F_TCSW, s->tcsw.max);
  WRITESEP(f, F_TCSW, s->tcsw.median);
  WRITELN(f,  F_TCSW, s->tcsw.mode);
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
  WRITEFMT(f, "%f", (double) s->total.median / million); SEP;
  // Stddev omitted until we know what to report for a log-normal distribution
  WRITEFMT(f, "%f", (double) 0.0); SEP;
  // Median total time (repeated to be compatible with hf format)
  WRITEFMT(f, "%f", (double) s->total.median / million); SEP;
  // User time in seconds as double 
  WRITEFMT(f, "%f", (double) s->user.median / million); SEP;
  // System time in seconds as double
  WRITEFMT(f, "%f", (double) s->system.median / million); SEP;
  // Min total time in seconds as double 
  WRITEFMT(f, "%f", (double) s->total.min / million); SEP;
  // Max total time in seconds as double
  WRITEFMT(f, "%f", (double) s->total.max / million); NEWLINE;
  fflush(f);
}




