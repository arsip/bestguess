//  -*- Mode: C; -*-                                                       
// 
//  bestguess.c
// 
//  MIT LICENSE 
//  COPYRIGHT (C) Jamie A. Jennings, 2024

#define _DEFAULT_SOURCE
#define _GNU_SOURCE

#include "common.h"
#include "csv.h"
#include "stats.h"
#include "utils.h"
#include "exec.h"
#include "reduce.h"
#include "optable.h"
#include "reports.h"
#include "bestguess.h"

#include <stdio.h>
#include <string.h>

int reducing_mode = 0;
int brief_summary = 0;
int show_graph = 0;
int runs = 1;
int warmups = 0;
int first_command = 0;
int show_output = 0;
int ignore_failure = 0;
int output_to_stdout = 0;
char *input_filename = NULL;
char *output_filename = NULL;
char *hf_filename = NULL;
const char *shell = NULL;

#define HELP_WARMUP "Number of warmup runs"
#define HELP_RUNS "Number of timed runs"
#define HELP_OUTPUT "Write timing data to CSV <FILE> (use - for stdout)"
#define HELP_FILE "Read commands/data from <FILE>"
#define HELP_BRIEF "Brief performance summary (shows only total time)"
#define HELP_GRAPH "Show graph of total time for each iteration"
#define HELP_SHOWOUTPUT "Show output of commands as they run"
#define HELP_IGNORE "Ignore non-zero exit codes"
#define HELP_SHELL "Use <SHELL> (e.g. \"/bin/bash -c\") to run commands"
#define HELP_HFCSV "Write Hyperfine-style CSV summary to <FILE>"

static void init_options(void) {
  optable_add(OPT_WARMUP,     "w",  "warmup",         1, HELP_WARMUP);
  optable_add(OPT_RUNS,       "r",  "runs",           1, HELP_RUNS);
  optable_add(OPT_OUTPUT,     "o",  "output",         1, HELP_OUTPUT);
  optable_add(OPT_FILE,       "f",  "file",           1, HELP_FILE);
  optable_add(OPT_BRIEF,      "b",  "brief",          0, HELP_BRIEF);
  optable_add(OPT_GRAPH,      "g",  "graph",          0, HELP_GRAPH);
  optable_add(OPT_SHOWOUTPUT, NULL, "show-output",    0, HELP_SHOWOUTPUT);
  optable_add(OPT_IGNORE,     "i",  "ignore-failure", 0, HELP_IGNORE);
  optable_add(OPT_SHELL,      "S",  "shell",          1, HELP_SHELL);
  optable_add(OPT_HFCSV,      NULL, "hyperfine-csv",  1, HELP_HFCSV);
  optable_add(OPT_VERSION,    "v",  "version",        0, "Show version");
  optable_add(OPT_HELP,       "h",  "help",           0, "Show help");
  if (optable_error())
    bail("\nERROR: failed to configure command-line option parser");
}

static int bad_option_value(const char *val, int n) {
  if (DEBUG)
    printf("%20s: %s\n", optable_longname(n), val);

  if (val) {
    if (!optable_numvals(n)) {
      printf("Error: option '%s' does not take a value\n",
	     optable_longname(n));
      return 1;
    }
  } else {
    if (optable_numvals(n)) {
      printf("Error: option '%s' requires a value\n",
	     optable_longname(n));
      return 1;
    }
  }
  return 0;
}

#define SCANINT(val, intvar, posvar, desc) do {			\
    if (bad_option_value(val, n)) exit(-1);			\
    int scancount = sscanf(val, "%d%n", &(intvar), &(posvar));	\
    if ((scancount != 1) || (posvar != (int) strlen(val))) {	\
      fprintf(stderr, "Failed to get %s from '%s'\n",		\
	      desc, val);					\
      exit(-1);							\
    }								\
  } while (0)

static int process_args(int argc, char **argv) {
  int n, i;
  int posn;
  const char *val;

  // 'i' steps through the args from 1 to argc-1
  i = optable_init(argc, argv);
  if (i < 0) bail("ERROR: Failed to initialize option parser");

  while ((i = optable_next(&n, &val, i))) {
    if (n < 0) {
      if (val) {
	// Command argument, not an option or switch
	if ((i == 1) && (strcmp(val, PROCESS_DATA_COMMAND) == 0)) {
	  if (DEBUG) printf("*** Reducing mode ***\n");
	  reducing_mode = 1;
	  continue;
	}
	if (!first_command) first_command = i;
	continue;
      }
      fprintf(stderr, "Error: invalid option/switch '%s'\n", argv[i]);
      exit(-1);
    }
    if (first_command) {
      fprintf(stderr, "Error: options found after first command '%s'\n",
	      argv[first_command]);
      exit(-1);
    }
    switch (n) {
      case OPT_BRIEF:
	if (bad_option_value(val, n)) exit(-1);
	brief_summary = 1;
	break;
      case OPT_GRAPH:
	if (bad_option_value(val, n)) exit(-1);
	show_graph = 1;
	break;
      case OPT_WARMUP:
	SCANINT(val, warmups, posn, "number of warmups");
	break;
      case OPT_RUNS:
	SCANINT(val, runs, posn, "number of runs");
	break;
      case OPT_OUTPUT:
	if (bad_option_value(val, n)) exit(-1);
	if (strcmp(val, "-") == 0) {
	  output_to_stdout = 1;
	} else {
	  output_filename = strdup(val);
	  output_to_stdout = 0;
	}
	break;
      case OPT_FILE:
	if (bad_option_value(val, n)) exit(-1);
	input_filename = strdup(val);
	break;
      case OPT_SHOWOUTPUT:
	if (bad_option_value(val, n)) exit(-1);
	show_output = 1;
	break;
      case OPT_IGNORE:
	if (bad_option_value(val, n)) exit(-1);
	ignore_failure = 1;
	break;
      case OPT_SHELL:
	if (bad_option_value(val, n)) exit(-1);
	shell = val;
	break;
      case OPT_HFCSV:
	if (bad_option_value(val, n)) exit(-1);
	hf_filename = strdup(val);
	break;
      case OPT_VERSION:
	if (bad_option_value(val, n)) exit(-1);
	return n;
	break;
      case OPT_HELP:
	if (bad_option_value(val, n)) exit(-1);
	return n;
	break;
      default:
	fprintf(stderr, "Error: invalid option index %d\n", n);
	exit(-1);
    }
  }
  return -1;
}

int main(int argc, char *argv[]) {

  int immediate, code;
  if (argc) progname = argv[0];
  optable_setusage("[options] <cmd> ...");

  if (argc < 2) {
    optable_printusage(progname);
    fprintf(stderr, "For more information, try %s --help\n", progname);
    exit(-1);
  }

  init_options();
  immediate = process_args(argc, argv);
  
  if (immediate == OPT_VERSION) {
    printf("%s: %s\n", progname, progversion);
    exit(0);
  } else if (immediate == OPT_HELP) {
    optable_printhelp(progname);
    exit(0);
  }

  if (reducing_mode) {
    code = reduce_data();
    exit(code);
  } else {
    run_all_commands(argc, argv);
  }

  optable_free();
  return 0;
}
