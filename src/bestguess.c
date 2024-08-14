//  -*- Mode: C; -*-                                                       
// 
//  bestguess.c
// 
//  MIT LICENSE 
//  COPYRIGHT (C) Jamie A. Jennings, 2024

#include "bestguess.h"

const char *progversion = "0.4.0";
const char *progname = "bestguess";

#include "csv.h"
#include "stats.h"
#include "utils.h"
#include "exec.h"
#include "reduce.h"
#include "optable.h"
#include "reports.h"

#include <stdio.h>
#include <string.h>

Config config = {
  .reducing_mode = 0,
  .brief_summary = 0,
  .show_graph = 0,
  .runs = 1,
  .warmups = 0,
  .first_command = 0,
  .show_output = 0,
  .ignore_failure = 0,
  .output_to_stdout = 0,
  .input_filename = NULL,
  .output_filename = NULL,
  .csv_filename = NULL,
  .hf_filename = NULL,
  .shell = "",
  .groups = 0,
};

#define HELP_WARMUP "Number of warmup runs"
#define HELP_RUNS "Number of timed runs"
#define HELP_OUTPUT "Write timing data to CSV <FILE> (use - for stdout)"
#define HELP_FILE "Read commands/data from <FILE>"
#define HELP_GROUPS "Blank lines in the input file delimit groups"
#define HELP_BRIEF "Brief performance summary (shows only total time)"
#define HELP_GRAPH "Show graph of total time for each iteration"
#define HELP_SHOWOUTPUT "Show output of commands as they run"
#define HELP_IGNORE "Ignore non-zero exit codes"
#define HELP_SHELL "Use <SHELL> (e.g. \"/bin/bash -c\") to run commands"
#define HELP_CSV "Write statistical summary to CSV <FILE>"
#define HELP_HFCSV "Write Hyperfine-style summary to CSV <FILE>"

static void init_options(void) {
  optable_add(OPT_WARMUP,     "w",  "warmup",         1, HELP_WARMUP);
  optable_add(OPT_RUNS,       "r",  "runs",           1, HELP_RUNS);
  optable_add(OPT_OUTPUT,     "o",  "output",         1, HELP_OUTPUT);
  optable_add(OPT_FILE,       "f",  "file",           1, HELP_FILE);
  optable_add(OPT_GROUPS,     NULL, "groups",         0, HELP_GROUPS);
  optable_add(OPT_BRIEF,      "b",  "brief",          0, HELP_BRIEF);
  optable_add(OPT_GRAPH,      "g",  "graph",          0, HELP_GRAPH);
  optable_add(OPT_SHOWOUTPUT, NULL, "show-output",    0, HELP_SHOWOUTPUT);
  optable_add(OPT_IGNORE,     "i",  "ignore-failure", 0, HELP_IGNORE);
  optable_add(OPT_SHELL,      "S",  "shell",          1, HELP_SHELL);
  optable_add(OPT_CSV,        NULL, "export-csv",     1, HELP_CSV);
  optable_add(OPT_HFCSV,      NULL, "hyperfine-csv",  1, HELP_HFCSV);
  optable_add(OPT_VERSION,    "v",  "version",        0, "Show version");
  optable_add(OPT_HELP,       "h",  "help",           0, "Show help");
  if (optable_error())
    PANIC("failed to configure command-line option parser");
}

static void check_option_value(const char *val, int n) {
  if (DEBUG)
    fprintf(stderr, "%20s: %s\n", optable_longname(n), val);

  if (val) {
    if (!optable_numvals(n)) {
      fprintf(stderr, "Error: option '%s' does not take a value\n",
	      optable_longname(n));
      exit(1);
    }
  } else {
    if (optable_numvals(n)) {
      fprintf(stderr, "Error: option '%s' requires a value\n",
	      optable_longname(n));
      exit(1);
    }
  }
}

static int process_args(int argc, char **argv) {
  int n, i;
  const char *val;

  // 'i' steps through the args from 1 to argc-1
  i = optable_init(argc, argv);
  if (i < 0) PANIC("failed to initialize option parser");

  while ((i = optable_next(&n, &val, i))) {
    if (n < 0) {
      if (val) {
	// Command argument, not an option or switch
	if ((i == 1) && (strcmp(val, PROCESS_DATA_COMMAND) == 0)) {
	  if (DEBUG) printf("*** Reducing mode ***\n");
	  config.reducing_mode = 1;
	  continue;
	}
	if (!config.first_command) config.first_command = i;
	continue;
      }
      PANIC("invalid option/switch '%s'\n", argv[i]);
    }
    if (config.first_command) {
      PANIC("options found after first command '%s'\n",
	    argv[config.first_command]);
    }
    switch (n) {
      case OPT_BRIEF:
	check_option_value(val, n);
	config.brief_summary = 1;
	break;
      case OPT_GRAPH:
	check_option_value(val, n);
	config.show_graph = 1;
	break;
      case OPT_WARMUP:
	check_option_value(val, n);
	config.warmups = strtoint64(val);
	break;
      case OPT_RUNS:
	check_option_value(val, n);
	config.runs = strtoint64(val);
	break;
      case OPT_OUTPUT:
	check_option_value(val, n);
	if (strcmp(val, "-") == 0) {
	  config.output_to_stdout = 1;
	} else {
	  config.output_filename = strdup(val);
	  config.output_to_stdout = 0;
	}
	break;
      case OPT_FILE:
	check_option_value(val, n);
	config.input_filename = strdup(val);
	break;
      case OPT_GROUPS:
	check_option_value(val, n);
	config.groups = 1;
	break;
      case OPT_SHOWOUTPUT:
	check_option_value(val, n);
	config.show_output = 1;
	break;
      case OPT_IGNORE:
	check_option_value(val, n);
	config.ignore_failure = 1;
	break;
      case OPT_SHELL:
	check_option_value(val, n);
	config.shell = val;
	break;
      case OPT_HFCSV:
	check_option_value(val, n);
	config.hf_filename = strdup(val);
	break;
      case OPT_CSV:
	check_option_value(val, n);
	config.csv_filename = strdup(val);
	break;
      case OPT_VERSION:
	check_option_value(val, n);
	return n;
	break;
      case OPT_HELP:
	check_option_value(val, n);
	return n;
	break;
      default:
	PANIC("invalid option index %d\n", n);
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
    exit(1);
  }

  init_options();
  immediate = process_args(argc, argv);
  
  if (immediate == OPT_VERSION) {
    printf("%s %s\n", progname, progversion);
    exit(0);
  } else if (immediate == OPT_HELP) {
    optable_printhelp(progname);
    exit(0);
  }

  if (config.reducing_mode) {
    code = reduce_data();
    exit(code);
  } else {
    run_all_commands(argc, argv);
  }

  optable_free();
  return 0;
}
