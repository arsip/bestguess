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
#define MAXCMDLEN (1 << 20)

// Maximum length of a single line in our own CSV file format
#define MAXCSVLEN (MAXCMDLEN + 8192)

// Maximum number of timed runs and warmup runs
#define MAXRUNS (1 << 20)

// Change as desired
#define PROGNAME_EXPERIMENT "bestguess"
#define CLI_OPTION_EXPERIMENT "run"
#define PROGNAME_REPORT "bestreport"
#define CLI_OPTION_REPORT "report"

// qsort arg order differs between linux and macos
#ifdef __linux__
  #define sort(base, n, sz, context, compare) qsort_r((base), (n), (sz), (compare), (context))
#else
  #define sort qsort_r
#endif

// -----------------------------------------------------------------------------
// Global configuration (based on CLI args)
// -----------------------------------------------------------------------------

typedef enum Action {
  actionNone,		   // No program action was chosen by the user
  actionExperiment,	   // Run an experiment (measure commands)
  actionReport,		   // Report on already-collected raw data
  actionVersion,	   // Print Bestguess version
  actionHelp,		   // Print Bestguess help
} Action;

typedef struct Config {
  int action;
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
  char *prep_command;
  const char *shell;
  int groups;
} Config;

extern Config config;

// The order of the options below is the order they will appear in the
// printed help text.
enum Options { 
  OPT_WARMUP,
  OPT_RUNS,
  OPT_PREP,
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
  OPT_ACTION,			// E.g. run, report
  OPT_VERSION,
  OPT_HELP,
};

#endif
