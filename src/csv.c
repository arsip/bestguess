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
#define WRITEFMT(fmt_string, val) do {	\
    fprintf(f, (fmt_string), (val));		\
  } while (0)
#define WRITESEP(fmt_idx, val) do {		\
    WRITEFMT(FieldFormats[fmt_idx], val);	\
    SEP;					\
  } while (0)
#define WRITELN(fmt_idx, val) do {		\
    WRITEFMT(FieldFormats[fmt_idx], val);	\
    NEWLINE;					\
  } while (0)

void write_line(FILE *f, const char *cmd, int code, usage *usage) {
  char *escaped_cmd = escape(cmd);
  char *shell_cmd = escape(config.shell);

  WRITESEP(F_CMD, escaped_cmd);
  WRITESEP(F_EXIT, code);
  if (shell_cmd) WRITESEP(F_SHELL, shell_cmd);
  else SEP;
  // User time in microseconds
  WRITESEP(F_USER, usertime(usage));
  // System time in microseconds
  WRITESEP(F_SYSTEM, systemtime(usage));
  // Max RSS in kibibytes
  // ru_maxrss (since Linux 2.6.32) This is the maximum resident set size used (in kilobytes).
  // When the above was written, a kilobyte meant 1024 bytes.
  WRITESEP(F_MAXRSS, maxrss(usage));
  // Page reclaims (count):
  // The number of page faults serviced without any I/O activity; here
  // I/O activity is avoided by “reclaiming” a page frame from the
  // list of pages awaiting reallocation.
  WRITESEP(F_RECLAIMS, minflt(usage));
  // Page faults (count):
  // The number of page faults serviced that required I/O activity.
  WRITESEP(F_FAULTS, majflt(usage));
  // Voluntary context switches:
  // The number of times a context switch resulted due to a process
  // voluntarily giving up the processor before its time slice was
  // completed (e.g. to await availability of a resource).
  WRITESEP(F_VCSW, vcsw(usage));
  // Involuntary context switches:
  // The number of times a context switch resulted due to a higher
  // priority process becoming runnable or because the current process
  // exceeded its time slice.
  WRITESEP(F_ICSW, icsw(usage));
  // Elapsed wall clock time in microseconds
  WRITELN(F_WALL, wall(usage));

  fflush(f);
  free(escaped_cmd);
  free(shell_cmd);
}

// -----------------------------------------------------------------------------
// Summary statistics file
// -----------------------------------------------------------------------------

void write_summary_header(FILE *f) {
  WRITEFMT("%s,", "Command");	      // 1
  WRITEFMT("%s,", "Shell");	      // 2
  WRITEFMT("%s,", "Runs (ct)");	      // 3
  WRITEFMT("%s,", "Failed (ct)");     // 4
  WRITEFMT("%s,", "Total mode (us)"); // 5
  WRITEFMT("%s,", "Total min (us)");
  WRITEFMT("%s,", "Total median (us)");
  WRITEFMT("%s,", "Total p95 (us)");
  WRITEFMT("%s,", "Total p99 (us)");
  WRITEFMT("%s,", "Total max (us)");
  WRITEFMT("%s,", "User mode (us)");
  WRITEFMT("%s,", "User min (us)");
  WRITEFMT("%s,", "User median (us)");
  WRITEFMT("%s,", "User p95 (us)");
  WRITEFMT("%s,", "User p99 (us)");
  WRITEFMT("%s,", "User max (us)");
  WRITEFMT("%s,", "System mode (us)");
  WRITEFMT("%s,", "System min (us)");
  WRITEFMT("%s,", "System median (us)");
  WRITEFMT("%s,", "System p95 (us)");
  WRITEFMT("%s,", "System p99 (us)");
  WRITEFMT("%s,", "System max (us)");
  WRITEFMT("%s,", "Max RSS mode (bytes)");
  WRITEFMT("%s,", "Max RSS min (bytes)");
  WRITEFMT("%s,", "Max RSS median (bytes)");
  WRITEFMT("%s,", "Max RSS p95 (bytes)");
  WRITEFMT("%s,", "Max RSS p99 (bytes)");
  WRITEFMT("%s,", "Max RSS max (bytes)");
  WRITEFMT("%s,", "Vol Ctx Sw mode (us)");
  WRITEFMT("%s,", "Vol Ctx Sw min (ct)");
  WRITEFMT("%s,", "Vol Ctx Sw median (ct)");
  WRITEFMT("%s,", "Vol Ctx Sw p95 (us)");
  WRITEFMT("%s,", "Vol Ctx Sw p99 (us)");
  WRITEFMT("%s,", "Vol Ctx Sw max (ct)");
  WRITEFMT("%s,", "Invol Ctx Sw mode (ct)");
  WRITEFMT("%s,", "Invol Ctx Sw min (ct)");
  WRITEFMT("%s,", "Invol Ctx Sw median (ct)");
  WRITEFMT("%s,", "Invol Ctx Sw p95 (ct)");
  WRITEFMT("%s,", "Invol Ctx Sw p99 (ct)");
  WRITEFMT("%s,", "Invol Ctx Sw max (ct)");
  WRITEFMT("%s,", "Total Ctx Sw mode (ct)");
  WRITEFMT("%s,", "Total Ctx Sw min (ct)");
  WRITEFMT("%s,", "Total Ctx Sw median (ct)");
  WRITEFMT("%s,", "Total Ctx Sw p95 (ct)");
  WRITEFMT("%s,", "Total Ctx Sw p99 (ct)");
  WRITEFMT("%s,", "Total Ctx Sw max (ct)");
  WRITEFMT("%s,", "Wall mode (us)");
  WRITEFMT("%s,", "Wall min (us)");
  WRITEFMT("%s,", "Wall median (us)");
  WRITEFMT("%s,", "Wall p95 (us)");
  WRITEFMT("%s,", "Wall p99 (us)");
  WRITEFMT("%s\n", "Wall max (us)");
  fflush(f);
}

#define MAYBE(write, field_code, numeric_field) do { \
    if (numeric_field < 0)			     \
      SEP;					     \
    else					     \
      write(field_code, numeric_field);		     \
  } while (0)

void write_summary_line(FILE *f, summary *s) {
  char *escaped_cmd = escape(s->cmd);
  char *shell_cmd = escape(config.shell);
  WRITESEP(F_CMD, escaped_cmd);
  if (config.shell)
    WRITESEP(F_SHELL, shell_cmd);
  else
    SEP;
  WRITEFMT("%d,", s->runs);
  WRITEFMT("%d,", s->fail_count);
  WRITESEP(F_TOTAL, s->total.mode);
  WRITESEP(F_TOTAL, s->total.min);
  WRITESEP(F_TOTAL, s->total.median);
  MAYBE(WRITESEP, F_TOTAL, s->total.pct95);
  MAYBE(WRITESEP, F_TOTAL, s->total.pct99);
  WRITESEP(F_TOTAL, s->total.max);
  WRITESEP(F_USER, s->user.mode);
  WRITESEP(F_USER, s->user.min);
  WRITESEP(F_USER, s->user.median);
  MAYBE(WRITESEP, F_USER, s->user.pct95);
  MAYBE(WRITESEP, F_USER, s->user.pct99);
  WRITESEP(F_USER, s->user.max);
  WRITESEP(F_SYSTEM, s->system.mode);
  WRITESEP(F_SYSTEM, s->system.min);
  WRITESEP(F_SYSTEM, s->system.median);
  MAYBE(WRITESEP, F_SYSTEM, s->system.pct95);
  MAYBE(WRITESEP, F_SYSTEM, s->system.pct99);
  WRITESEP(F_SYSTEM, s->system.max);
  WRITESEP(F_MAXRSS, s->maxrss.mode);
  WRITESEP(F_MAXRSS, s->maxrss.min);
  WRITESEP(F_MAXRSS, s->maxrss.median);
  MAYBE(WRITESEP, F_MAXRSS, s->maxrss.pct95);
  MAYBE(WRITESEP, F_MAXRSS, s->maxrss.pct99);
  WRITESEP(F_MAXRSS, s->maxrss.max);
  WRITESEP(F_VCSW, s->vcsw.mode);
  WRITESEP(F_VCSW, s->vcsw.min);
  WRITESEP(F_VCSW, s->vcsw.median);
  MAYBE(WRITESEP, F_VCSW, s->vcsw.pct95);
  MAYBE(WRITESEP, F_VCSW, s->vcsw.pct99);
  WRITESEP(F_VCSW, s->vcsw.max);
  WRITESEP(F_ICSW, s->icsw.mode);
  WRITESEP(F_ICSW, s->icsw.min);
  WRITESEP(F_ICSW, s->icsw.median);
  MAYBE(WRITESEP, F_ICSW, s->icsw.pct95);
  MAYBE(WRITESEP, F_ICSW, s->icsw.pct99);
  WRITESEP(F_ICSW, s->icsw.max);
  WRITESEP(F_TCSW, s->tcsw.mode);
  WRITESEP(F_TCSW, s->tcsw.min);
  WRITESEP(F_TCSW, s->tcsw.median);
  MAYBE(WRITESEP, F_TCSW, s->tcsw.pct95);
  MAYBE(WRITESEP, F_TCSW, s->tcsw.pct99);
  WRITESEP(F_TCSW, s->tcsw.max);
  WRITESEP(F_WALL, s->wall.mode);
  WRITESEP(F_WALL, s->wall.min);
  WRITESEP(F_WALL, s->wall.median);
  MAYBE(WRITESEP, F_WALL, s->wall.pct95);
  MAYBE(WRITESEP, F_WALL, s->wall.pct99);
  WRITELN(F_WALL, s->wall.max);
  fflush(f);
  free(escaped_cmd);
  free(shell_cmd);
}


// -----------------------------------------------------------------------------
// Hyperfine-format file
// -----------------------------------------------------------------------------

// Differences:
// - Mean has been replaced by mode for total time, user time, system time
// - Stddev is omitted

void write_hf_header(FILE *f) {
  fprintf(f, "command,mode,stddev,median,user,system,min,max\n");
  fflush(f);
}

void write_hf_line(FILE *f, summary *s) {
  const double million = MICROSECS;
  // Command
  WRITEFMT("%s", *(s->cmd) ? s->cmd : config.shell); SEP;
  // Mode total time (written to the mean field because mean is useless)
  WRITEFMT("%f", (double) s->total.mode / million); SEP;
  // Stddev omitted til we get an appropriate variance measure for
  // long-tailed (skewed) multi-modal distributions
  SEP;
  // Mode total time (repeated to be compatible with hf format)
  WRITEFMT("%f", (double) s->total.mode / million); SEP;
  // User time in seconds as double 
  WRITEFMT("%f", (double) s->user.mode / million); SEP;
  // System time in seconds as double
  WRITEFMT("%f", (double) s->system.mode / million); SEP;
  // Min total time in seconds as double 
  WRITEFMT("%f", (double) s->total.min / million); SEP;
  // Max total time in seconds as double
  WRITEFMT("%f", (double) s->total.max / million); NEWLINE;
  fflush(f);
}




