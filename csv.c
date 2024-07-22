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
  for (int i = 0; i < (F_END - 1); i++)
    fprintf(f, "%s,", FieldHeaders[i]);
  fprintf(f, "%s\n", FieldHeaders[F_END - 1]);
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
  WRITESEP(f, F_USER, Ruser(usage));
  // System time in microseconds
  WRITESEP(f, F_SYS, Rsys(usage));
  // Max RSS in kibibytes
  // ru_maxrss (since Linux 2.6.32) This is the maximum resident set size used (in kilobytes).
  // When the above was written, a kilobyte meant 1024 bytes.
  WRITESEP(f, F_RSS, Rrss);
  // Page reclaims (count):
  // The number of page faults serviced without any I/O activity; here
  // I/O activity is avoided by “reclaiming” a page frame from the
  // list of pages awaiting reallocation.
  WRITESEP(f, F_PGREC, Rpgrec);
  // Page faults (count):
  // The number of page faults serviced that required I/O activity.
  WRITESEP(f, F_PGFLT, usage->ru_majflt);
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

const enum FieldCodes stats[] = {F_CMD,
				 F_EXIT,
				 F_SHELL,
				 F_END};


// Assumes at least one field code in 'stats' before F_END
void write_stat_header(FILE *f) {
  int i = 0;
  while (stats[i+1] != F_END) {
    fprintf(f, "%s,", FieldHeaders[stats[i]]);
    i++;
  }
  fprintf(f, "%s\n", FieldHeaders[stats[i]]);
  fflush(f);
}

// TODO: cmd accessor returning this: *(s->cmd) ? s->cmd : shell
// TODO: Put accessors into Field list

void write_stat_line(FILE *f, summary *s) {
  int i = 0;
//   while (stats[i+1] != F_END) {
//     WRITESEP(f, stats[i], accessors[i]( TODO ));
//     i++;
//   }
//   WRITELN(f, stats[i], accessors[i]( TODO ) );
  fflush(f);
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
