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
#include <stdbool.h>
#include <string.h>

extern const char *progversion;
extern const char *progname;

// Change to non-zero to enable debugging output to stdout
// E.g. on command-line:  make DEBUG=1
#ifndef DEBUG
#define DEBUG 0
#endif

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

// -----------------------------------------------------------------------------
// Global configuration (based on CLI args)
// -----------------------------------------------------------------------------

typedef enum Action {
  actionNone,		   // No program action was chosen by the user
  actionExecute,	   // Run an experiment (measure commands)
  actionReport,		   // Report on already-collected raw data
} Action;

typedef struct OptionValues {
  int    action;
  int    helpversion;
  int    runs;
  int    warmups;
  int    first;
  bool   show_output;
  bool   ignore_failure;
  int    n_commands;
  const char *commands[MAXCMDS];
  const char *names[MAXCMDS];
  const char  *shell;
  char  *input_filename;
  char  *output_filename;
  char  *csv_filename;
  char  *hf_filename;
  char  *prep_command;
  bool   graph;
  bool   nostats;
  bool   ministats;
  bool   diststats;
  bool   tailstats;
  bool   boxplot;
  bool   explain;
} OptionValues;

extern OptionValues option;

typedef struct Config {
  // Reporting configuration
  int     width;
  // Inferential statistics interpretation
  double  alpha;	 // p-value threshold for significance
  int64_t epsilon;	 // for confidence intervals (μs)
  int64_t effect;	 // minimum effect size (μs)
  double  super;	 // probability threshold for high superiority
} Config;

extern Config config;

#endif
