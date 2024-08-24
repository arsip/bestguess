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
#include "cli.h"

#include <stdio.h>
#include <string.h>

Config config = {
  .action = actionNone,
  .helpversion = -1,
  .brief_summary = false,
  .show_graph = false,
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
  .report = NULL,
  .boxplot = false,
  .all = false
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

  if (argc) progname = argv[0];

  if (argc < 2) {
    optable_printusage(progname);
    USAGE("For more information, try %s --help\n", progname);
  }

  // Check for unusual install situation where the executable name is
  // not one recognize.  In that case, the user must supply an ACTION
  // argument.  Look for that argument, ignoring other CLI args for now. 
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
	reduce_data(argc, argv);
	break;
      default:
	PANIC("Action not set");
    }
  }
  optable_free();
  return 0;
}
