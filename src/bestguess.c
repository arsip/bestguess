//  -*- Mode: C; -*-                                                       
// 
//  bestguess.c
// 
//  MIT LICENSE 
//  COPYRIGHT (C) Jamie A. Jennings, 2024

#include "bestguess.h"

const char *progversion = "0.6.1";
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

Config config = {
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
  .groups = false,
  .report = REPORT_SUMMARY,
  .boxplot = false,
  .width = 80,			// terminal width
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

  Usage *usage;
  if (argc) progname = argv[0];
  if (argc < 2) {
    optable_printusage(progname);
    USAGE("For more information, try %s --help\n", progname);
  }

  // Check for either of two unusual cases:
  // 
  // (1) The user supplied an ACTION argument, though this is not
  // normally needed.  The action of running an experiment is assumed
  // if the executable name is 'bestguess'; the action of processing
  // raw data and reporting on it is assumed if the executable name is
  // 'bestreport'.
  //
  // (2) However, if BestGuess was NOT installed using the intended
  // executable names, the user MUST supply an ACTION argument.  We
  // look for that argument, ignoring other CLI args temporarily.
  //
  optable_setusage("-A <action> [options] ...");
  process_action_options(argc, argv);

  // If no ACTION argument was given, deduce it from executable name
  if (config.action == actionNone)
    config.action = action_from_progname(progname);

  if (config.helpversion == OPT_HELP) {
    print_help();
  } else if (config.helpversion == OPT_VERSION) {
    printf("%s %s\n", progname, progversion);
  } else {
    switch (config.action) {
      case actionNone: 
	optable_printusage(progname);
	USAGE("For more information, try %s --help\n", progname);
	break;
      case actionExecute:
	optable_setusage("[options] <cmd1> ...");
	process_exec_options(argc, argv);
	run_all_commands(argc, argv);
	break;
      case actionReport:
	optable_setusage("[options] <datafile1> ...");
	process_report_options(argc, argv);
	usage = read_input_files(argc, argv);
	report(usage);
	free_usage_array(usage);
	break;
      default:
	PANIC("Action not set");
    }
  }
  optable_free();
  return 0;
}
