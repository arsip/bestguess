//  -*- Mode: C; -*-                                                       
// 
//  bestguess.c
// 
//  MIT LICENSE 
//  COPYRIGHT (C) Jamie A. Jennings, 2024

#include "bestguess.h"

const char *progversion = "0.6.7-beta";
const char *progname = "bestguess";

#include "csv.h"
#include "stats.h"
#include "utils.h"
#include "exec.h"
#include "optable.h"
#include "reports.h"
#include "cli.h"

#include <stdio.h>
#include <string.h>

OptionValues option = {
  .action = actionNone,
  .helpversion = -1,
  .graph = false,
  .runs = 1,
  .warmups = 0,
  .first = 0,
  .show_output = false,
  .ignore_failure = false,
  .output_to_stdout = false,
  .input_filename = NULL,
  .output_filename = NULL,
  .csv_filename = NULL,
  .hf_filename = NULL,
  .prep_command = NULL,
  .shell = "",
  .n_commands = 0,
  .commands = {NULL},
  .names = {NULL},
  .report = REPORT_SUMMARY,
  .boxplot = false,
  .explain = false,
};

// Sentinel value of -1 means "uninitialized"
Config config = {
  .width = -1,			// terminal width
  .alpha = -1,			// p-value threshold
  .epsilon = -1,		// μs
  .effect = -1,			// μs
  .super = -1,			// probability
};

// -----------------------------------------------------------------------------
//
// Bestguess is one executable.  The user can supply an ACTION
// argument on the command line to either 'run' an experiment or
// 'report' on previously-collected data.
//
// A standard installation (via `make install`) copies the `bestguess`
// executable to the install directory, and then creates a symlink to
// it from `bestreport`.  By checking the name by which Bestguess is
// executed, the ACTION defaults to 'run' (bestguess) or 'report'
// (bestreport).
//
// -----------------------------------------------------------------------------

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

static Action action_from_progname(const char *executable) {
  if (strcmp(base(executable), PROGNAME_EXPERIMENT) == 0)
    return actionExecute;
  else if (strcmp(base(executable), PROGNAME_REPORT) == 0)
    return actionReport;
  return actionNone;
}

int main(int argc, char *argv[]) {

  Ranking *ranking;
  
  if (argc) progname = argv[0];
  if (argc < 2) {
    optable_printusage(progname);
    USAGE("For more information, try %s --help\n", progname);
  }

  // Check for either of two unusual cases:
  // 
  // (1) The user supplied an ACTION argument, though this is not
  // normally needed.  If the executable name is 'bestguess', the
  // default action is to run an experiment.  If the executable name
  // is 'bestreport', the default action is to process raw data and
  // produce a report.
  //
  // (2) However, if BestGuess was NOT installed using the intended
  // executable names, the user MUST supply an ACTION argument.  We
  // look for that argument, ignoring other CLI args temporarily.
  //
  optable_setusage("[-A <action>] [options] ...");
  process_common_options(argc, argv);
  set_config_defaults();

  // If no ACTION argument was given, deduce it from executable name
  if (option.action == actionNone)
    option.action = action_from_progname(progname);

  if (option.helpversion == OPT_HELP) {
    print_help();
  } else if (option.helpversion == OPT_VERSION) {
    printf("%s %s\n", progname, progversion);
  } else if (option.helpversion == OPT_SHOWCONFIG) {
    show_config_settings();
  } else {
    switch (option.action) {
      case actionNone: 
	optable_printusage(progname);
	USAGE("For more information, try %s --help\n", progname);
	break;
      case actionExecute:
	optable_setusage("[options] <cmd1> ...");
	process_exec_options(argc, argv);
	ranking = run_all_commands();
	report(ranking);
	free_ranking(ranking);
	break;
      case actionReport:
	optable_setusage("[options] <datafile1> ...");
	process_report_options(argc, argv);
	ranking = read_input_files(argc, argv);
	report(ranking);
	free_ranking(ranking);
	break;
      default:
	PANIC("Action not set");
    }
  }
  optable_free();
  free_report_help();
  free_config_help();
  return 0;
}
