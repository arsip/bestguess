//  -*- Mode: C; -*-                                                       
// 
//  bestguess.h
// 
//  COPYRIGHT (C) Jamie A. Jennings, 2024

#ifndef bestguess_h
#define bestguess_h

#define _DEFAULT_SOURCE
#define _GNU_SOURCE

#include <stdlib.h>
#include <inttypes.h>
#include "stats.h"

extern const char *progversion;
extern const char *progname;

// Change to non-zero to enable debugging output to stdout
#define DEBUG 0

// Maximum number of commands we allow to benchmark
#define MAXCMDS 200

// Maximum number of arguments in one command
// E.g. "ls -l -h *.c" is a command with 3 arguments,
// and "ls -lh *.c" has 2 arguments.
#define MAXARGS 250

// Maximum length of a single command, in bytes
// E.g. "ls -lh" has 7 bytes (6 chars and NUL)
#define MAXCMDLEN 4096

// Maximum length of a single line in our own CSV file format
#define MAXCSVLEN 4096

// Change as desired
#define PROCESS_DATA_COMMAND "reduce"

// qsort arg order differs between linux and macos
#ifdef __linux__
  #define sort(base, n, sz, context, compare) qsort_r((base), (n), (sz), (compare), (context))
#else
  #define sort qsort_r
#endif

// -----------------------------------------------------------------------------
// Global configuration (based on CLI args)
// -----------------------------------------------------------------------------

extern int reducing_mode;
extern int brief_summary;
extern int show_graph;
extern int runs;
extern int warmups;
extern int first_command;
extern int show_output;
extern int ignore_failure;
extern int output_to_stdout;
extern char *input_filename;
extern char *output_filename;
extern char *hf_filename;
extern const char *shell;

// The order of the options below is the order they will appear in the
// printed help text.
enum Options { 
  OPT_WARMUP,
  OPT_RUNS,
  OPT_OUTPUT,
  OPT_FILE,
  OPT_BRIEF,
  OPT_GRAPH,
  OPT_SHOWOUTPUT,
  OPT_IGNORE,
  OPT_SHELL,
  OPT_HFCSV,			// Hyperfine format CSV
  OPT_VERSION,
  OPT_HELP,
};

// -----------------------------------------------------------------------------
// Complete list of fields to write to output files of any kind
// -----------------------------------------------------------------------------
     
typedef union accessor {
  const char *(*get_string)(summary *);
  int         (*get_int)(summary *);
  int64_t     (*get_usage)(struct rusage *);
  int64_t     (*get_stat)(summary *);
} accessor;

#define RFieldDecls(X)						    \
  X(F_USER,      Ruser,      "User time (us)",          "%" PRId64) \
  X(F_SYS,       Rsys,       "System time (us)",        "%" PRId64) \
  X(F_TOTAL,     Rtotal,     "Total time (us)",         "%" PRId64) \
  X(F_RSS,       Rrss,       "Max RSS (Bytes)",         "%ld")      \
  X(F_PGREC,     Rpgrec,     "Page Reclaims",           "%ld")	    \
  X(F_PGFLT,     Rpgflt,     "Page Faults",             "%ld")	    \
  X(F_VCSW,      Rvcsw,      "Vol Ctx Sw",              "%ld")	    \
  X(F_ICSW,      Ricsw,      "Total Ctx Sw",            "%ld")      \
  X(F_TCSW,      Rtcsw,      "Total Ctx Sw",            "%ld")      \
  X(F_R,         NULL,       "Sentinel",                NULL)

#define SFieldDecls(X)						    \
  X(F_CMD = F_R, Scommand,   "Command",                 "\"%s\"")   \
  X(F_EXIT,      Scode,      "Exit code",               "%d")       \
  X(F_SHELL,     Sshell,     "Shell",                   "\"%s\"")   \
       								    \
  X(F_USERMED,   Susermed,   "User time median (us)",   "%" PRId64) \
  X(F_USERMODE,  Susermode,  "User time mode (us)",     "%" PRId64) \
  X(F_USERMIN,   Susermin,   "User time min (us)",      "%" PRId64) \
  X(F_USERMAX,   Susermax,   "User time max (us)",      "%" PRId64) \
       								    \
  X(F_SYSMED,    Ssysmed,    "System time median (us)", "%" PRId64) \
  X(F_SYSMODE,   Ssysmode,   "System time mode (us)",   "%" PRId64) \
  X(F_SYSMIN,    Ssysmin,    "System time min (us)",    "%" PRId64) \
  X(F_SYSMAX,    Ssysmax,    "System time max (us)",    "%" PRId64) \
  								    \
  X(F_TOTALMED,  Stotalmed,  "Total time median (us)",  "%" PRId64) \
  X(F_TOTALMODE, Stotalmode, "Total time mode (us)",    "%" PRId64) \
  X(F_TOTALMIN,  Stotalmin,  "Total time min (us)",     "%" PRId64) \
  X(F_TOTALMAX,  Stotalmax,  "Total time max (us)",     "%" PRId64) \
  								    \
  X(F_RSSMED,    Srssmed,    "Max RSS median (Bytes)",  "%ld")      \
  X(F_RSSMODE,   Srssmode,   "Max RSS mode (Bytes)",    "%ld")      \
  X(F_RSSMIN,    Srssmin,    "Max RSS min (Bytes)",     "%ld")      \
  X(F_RSSMAX,    Srssmax,    "Max RSS max (Bytes)",     "%ld")      \
  								    \
  X(F_PGRECMED,  Spgrecmed,  "Page Reclaims median",    "%ld")	    \
  X(F_PGRECMODE, Spgrecmode, "Page Reclaims mode",      "%ld")	    \
  X(F_PGRECMIN,  Spgrecmin,  "Page Reclaims min",       "%ld")	    \
  X(F_PGRECMAX,  Spgrecmax,  "Page Reclaims max",       "%ld")	    \
								    \
  X(F_PGFLTMED,  Spgfltmed,  "Page Faults median",      "%ld")	    \
  X(F_PGFLTMODE, Spgfltmode, "Page Faults mode",        "%ld")	    \
  X(F_PGFLTMIN,  Spgfltmin,  "Page Faults min",         "%ld")	    \
  X(F_PGFLTMAX,  Spgfltmax,  "Page Faults max",         "%ld")	    \
								    \
  X(F_VCSWMED,   Svcswmed,   "Vol Cts Sw median",       "%ld")      \
  X(F_VCSWMODE,  Svcswmode,  "Vol Cts Sw mode",         "%ld")	    \
  X(F_VCSWMIN,   Svcswmin,   "Vol Cts Sw min",          "%ld")      \
  X(F_VCSWMAX,   Svcswmax,   "Vol Cts Sw max",          "%ld")      \
								    \
  X(F_ICSWMED,   Sicswmed,   "Invol Cts Sw median",     "%ld")	    \
  X(F_ICSWMODE,  Sicswmode,  "Invol Cts Sw mode",       "%ld")	    \
  X(F_ICSWMIN,   Sicswmin,   "Invol Cts Sw min",        "%ld")      \
  X(F_ICSWMAX,   Sicswmax,   "Invol Cts Sw max",        "%ld")      \
  X(F_END,       NULL,       "Sentinel",                NULL)       


#define FIRST(a, b, c, d) a,
enum FieldCodes {RFieldDecls(FIRST) SFieldDecls(FIRST)};
extern const accessor FieldAccessors[];
extern const char *FieldHeaders[];
extern const char *FieldFormats[];

#endif
