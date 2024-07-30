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

typedef struct Config {
  int reducing_mode;
  int brief_summary;
  int show_graph;
  int runs;
  int warmups;
  int first_command;
  int show_output;
  int ignore_failure;
  int output_to_stdout;
  char *input_filename;
  char *output_filename;
  char *csv_filename;
  char *hf_filename;
  const char *shell;
  int groups;
} Config;

extern Config config;

// The order of the options below is the order they will appear in the
// printed help text.
enum Options { 
  OPT_WARMUP,
  OPT_RUNS,
  OPT_OUTPUT,
  OPT_FILE,
  OPT_GROUPS,
  OPT_BRIEF,
  OPT_GRAPH,
  OPT_SHOWOUTPUT,
  OPT_IGNORE,
  OPT_SHELL,
  OPT_CSV,			// BestGuess format CSV
  OPT_HFCSV,			// Hyperfine format CSV
  OPT_VERSION,
  OPT_HELP,
};

// -----------------------------------------------------------------------------
// Output file (CSV)
// -----------------------------------------------------------------------------
     
#define XFields(X)					    \
  X(F_CMD,      "Command",                      "\"%s\"")   \
  X(F_EXIT,     "Exit code",                    "%d")	    \
  X(F_SHELL,    "Shell",                        "\"%s\"")   \
  X(F_USER,     "User time (us)",               "%" PRId64) \
  X(F_SYSTEM,   "System time (us)",             "%" PRId64) \
  X(F_TOTAL,    "Total time (us)",              "%" PRId64) \
  X(F_RSS,      "Max RSS (Bytes)",              "%ld")	    \
  X(F_RECLAIMS, "Page Reclaims",                "%ld")	    \
  X(F_FAULTS,   "Page Faults",                  "%ld")	    \
  X(F_VCSW,     "Voluntary Context Switches",   "%ld")	    \
  X(F_ICSW,     "Involuntary Context Switches", "%ld")      \
  X(F_TCSW,     "Total Context Switches",       "%ld")	

#define FIRST(a, b, c) a,
enum FieldCodes {XFields(FIRST) F_LAST};
extern const char *Headers[];
extern const char *FieldFormats[];

#endif
