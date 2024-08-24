//  -*- Mode: C; -*-                                                       
// 
//  bestguess.c
// 
//  MIT LICENSE 
//  COPYRIGHT (C) Jamie A. Jennings, 2024

#include "bestguess.h"

const char *progversion = "0.5.2";
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
  .action = actionNone,
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
  .prep_command = NULL,
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
#define HELP_PREPARE "Execute <COMMAND> before each benchmarked command"
#define HELP_ACTION							\
  "In rare circumstances, the Bestguess executables\n"			\
  "are installed under custom names.  In that case, the\n"		\
  "<ACTION> option is required.  See the manual for more." 

// "Normally,\n"								\
//   "the `bestguess` command runs an experiment (executes commands)\n"	\
//   "and the `bestreport` command reads raw timing data from a CSV\n"	\
//   "file to produce various reports."

static void init_options(void) {
  optable_add(OPT_WARMUP,     "w",  "warmup",         1, HELP_WARMUP);
  optable_add(OPT_RUNS,       "r",  "runs",           1, HELP_RUNS);
  optable_add(OPT_PREP,       "p",  "prepare",        1, HELP_PREPARE);
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
  optable_add(OPT_ACTION,     "A",  "action",         1, HELP_ACTION);
  optable_add(OPT_VERSION,    "v",  "version",        0, "Show version");
  optable_add(OPT_HELP,       "h",  "help",           0, "Show help");
  if (optable_error())
    PANIC("failed to configure command-line option parser");
}

static void check_option_value(const char *val, int n) {
  if (DEBUG)
    fprintf(stderr, "%20s: %s\n", optable_longname(n), val);

  if (val) {
    if (!optable_numvals(n))
      USAGE("Error: option '%s' does not take a value\n", optable_longname(n));
  } else {
    if (optable_numvals(n))
      USAGE("Error: option '%s' requires a value\n", optable_longname(n));
  }
}

// Cheap version of basename() for Unix only
static const char *base(const char *str) {
  if (!str) PANIC_NULL();
  const char *p = str;
  while (*p) p++;
  while (p >= str)
    if (*p == '/') break;
    else p--;
  const char *ans = (*p == '/') ? p+1 : str;
  return ans;
}

// If installed program name is 'bestguess' then by default run an
// experiment.  If 'bestreport' then by default produce a report.
static Action action_from_progname(const char *executable) {
  if (strcmp(base(executable), PROGNAME_EXPERIMENT) == 0)
    return actionExperiment;
  else if (strcmp(base(executable), PROGNAME_REPORT) == 0)
    return actionReport;
  return actionNone;
}

// Process the CLI args and set 'config' parameters
static void process_args(int argc, char **argv) {
  int n, i;
  const char *val;

  // 'i' steps through the args from 1 to argc-1
  i = optable_init(argc, argv);
  if (i < 0) PANIC("failed to initialize option parser");

  while ((i = optable_next(&n, &val, i))) {
    if (n < 0) {
      if (val) {
	if (!config.first_command) config.first_command = i;
	continue;
      }
      USAGE("Invalid option/switch '%s'", argv[i]);
    }
    if (config.first_command) {
      USAGE("Options found after first command '%s'",
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
	if ((config.warmups < 0) || (config.warmups > MAXRUNS))
	  USAGE("Number of warmup runs is out of range 0..%d", MAXRUNS);
	break;
      case OPT_RUNS:
	check_option_value(val, n);
	config.runs = strtoint64(val);
	if ((config.runs < 0) || (config.runs > MAXRUNS))
	  USAGE("Number of timed runs is out of range 0..%d", MAXRUNS);
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
      case OPT_PREP:
	check_option_value(val, n);
	config.prep_command = strdup(val);
	break;
      case OPT_VERSION:
	check_option_value(val, n);
	if (config.action != actionHelp)
	  config.action = actionVersion;
	break;
      case OPT_HELP:
	check_option_value(val, n);
	config.action = actionHelp;
	return;
      case OPT_ACTION:
	check_option_value(val, n);
	if (config.action != actionHelp) {
	  if (strcmp(val, CLI_OPTION_EXPERIMENT) == 0) {
	    config.action = actionExperiment;
	  } else if (strcmp(val, CLI_OPTION_REPORT) == 0) {
	    config.action = actionReport;
	  } else {
	    char *message;
	    asprintf(&message,
		     "Valid actions are:\n"
		     "%-8s  run an experiment (measure runtimes of commands)\n"
		     "%-8s  read raw timing data from a CSV file and produce reports\n",
		     CLI_OPTION_EXPERIMENT, CLI_OPTION_REPORT);
	    USAGE(message);
	  }
	}
	break;
      default:
	PANIC("invalid option index %d\n", n);
    }
  }
  return;
}

int main(int argc, char *argv[]) {

  if (argc) progname = argv[0];
  optable_setusage("[options] <cmd> ...");

  if (argc < 2) {
    optable_printusage(progname);
    USAGE("For more information, try %s --help\n", progname);
  }

  int code = 0;
  init_options();
  process_args(argc, argv);	// Set the 'config' parms
  
  if (config.action == actionNone)
    config.action = action_from_progname(progname);

  // Check for help first, in case the user supplies conflicting
  // options like -v -h.
  switch (config.action) {
    case actionHelp:
      optable_printhelp(progname);
      break;
    case actionVersion:
      printf("%s %s\n", progname, progversion);
      break;
    case actionExperiment:
      run_all_commands(argc, argv);
      break;
    case actionReport:
      code = reduce_data();
      break;
    default:
      PANIC("Action not set");
  }

  optable_free();
  return code;
}
