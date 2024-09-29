//  -*- Mode: C; -*-                                                       
// 
//  cli.c
// 
//  (C) Jamie A. Jennings, 2024

#include "bestguess.h"
#include "cli.h"
#include "utils.h"
#include "reports.h"
#include "optable.h"
#include <stdio.h>
#include <string.h>

static void check_option_value(const char *val, int n) {
  if (val && !optable_numvals(n))
    USAGE("Error: option '%s' does not take a value\n", optable_longname(n));
  if (!val && optable_numvals(n))
    USAGE("Error: option '%s' requires a value\n", optable_longname(n));
}

static char *configuration_help_string = NULL;

// For printing program help.  Caller must free the returned string.
static char *config_help(void) {
  if (configuration_help_string) return configuration_help_string;
  size_t bufsize = 1000;
  char *buf = malloc(bufsize);
  if (!buf) PANIC_OOM();
  int len = snprintf(buf, bufsize,
		     "Configure <SETTING>=<VALUE>, e.g. width=80.\n"
		     "Settings are:");
  for (ConfigCode i = 0; i < CONFIG_LAST; i++) {
    bufsize -= len;
    len += snprintf(buf + len, bufsize, "\n  %-8s %s",
		    ConfigSettingName[i], ConfigSettingDesc[i]);
  }
  configuration_help_string = buf;
  return configuration_help_string;
}

void free_config_help(void) {
  free(configuration_help_string);
}

#define SECOND(a, b, c) b,
const char *ConfigSettingName[] = {XConfig_Settings(SECOND)};
#undef SECOND
#define THIRD(a, b, c) c,
const char *ConfigSettingDesc[] = {XConfig_Settings(THIRD)};
#undef THIRD

#define HELP_GRAPH "Show graph of total time for each iteration"
#define HELP_BOXPLOT "Show box plots of timing data"
#define HELP_WIDTH "Terminal width (default 80 chars)"
#define HELP_ACTION							\
  "In rare circumstances, the Bestguess executables\n"			\
  "are installed under custom names.  In that case, the\n"		\
  "<ACTION> option is required.  See the manual for more." 

static void init_action_options(void) {
  optable_add(OPT_REPORT,     "R",  "report",         1, report_options());
  optable_add(OPT_GRAPH,      "G",  "graph",          0, HELP_GRAPH);
  optable_add(OPT_BOXPLOT,    "B",  "boxplot",        0, HELP_BOXPLOT);
  optable_add(OPT_ACTION,     "A",  "action",         1, HELP_ACTION);
  optable_add(OPT_VERSION,    "v",  "version",        0, "Show version");
  optable_add(OPT_HELP,       "h",  "help",           0, "Show help");
  if (optable_error())
    PANIC("failed to configure command-line option parser");
}

// Check if the ACTION option is given, looking also for HELP and
// VERSION because those override the requested ACTION, and for the
// CLI options that are common to all actions.
void process_action_options(int argc, char **argv) {
  optable_reset();
  init_action_options();
  int n, i;
  const char *val;
  // 'i' steps through the args from 1 to argc-1
  i = optable_init(argc, argv);
  if (i < 0) PANIC("failed to initialize option parser");
  while ((i = optable_next(&n, &val, i))) {
    if (n < 0) continue;
    switch (n) {
      case OPT_VERSION:
	if (config.helpversion == -1)
	  config.helpversion = OPT_VERSION;
	break;
      case OPT_HELP:
	config.helpversion = OPT_HELP;
	break;
      case OPT_ACTION:
	if (val && (strcmp(val, CLI_OPTION_EXPERIMENT) == 0)) {
	  config.action = actionExecute;
	  break;
	} else if (val && (strcmp(val, CLI_OPTION_REPORT) == 0)) {
	  config.action = actionReport;
	  break;
	} else {
	  char *message;
	  asprintf(&message,
		   "Valid actions are:\n"
		   "%-8s  run an experiment (measure runtimes of commands)\n"
		   "%-8s  read raw timing data from a CSV file and produce reports\n",
		   CLI_OPTION_EXPERIMENT, CLI_OPTION_REPORT);
	  USAGE(message);
	}
      case OPT_BOXPLOT:
	check_option_value(val, n);
	config.boxplot = true;
	break;
      case OPT_REPORT:
	check_option_value(val, n);
	config.report = interpret_report_option(val);
	if (config.report == REPORT_ERROR)
	  USAGE(report_options());
	break;
      case OPT_CONFIG:
	check_option_value(val, n);
	printf("Received configuration option '%s'\n", val);
// 	config.width = strtoint64(val);
// 	if ((config.width < 40) || (config.runs > 1024))
// 	  USAGE("Terminal width (%d) is out of range 40..1024", config.width);
	break;
      case OPT_GRAPH:
	check_option_value(val, n);
	config.graph = true;
	break;
      default:
	PANIC("invalid option index %d\n", n);
    }
  }
}

// -----------------------------------------------------------------------------
// ACTION 'run' (execute experiments)
// -----------------------------------------------------------------------------

#define HELP_WARMUP "Number of warmup runs"
#define HELP_RUNS "Number of timed runs"
#define HELP_NAME "Name to use in reports instead of full command"
#define HELP_OUTPUT "Write timing data to CSV <FILE> (use - for stdout)"
#define HELP_CMDFILE "Read commands from <FILE>"
#define HELP_GROUPS "Blank lines in the input file delimit groups"
#define HELP_SHOWOUTPUT "Show output of commands as they run"
#define HELP_IGNORE "Ignore non-zero exit codes"
#define HELP_SHELL "Use <SHELL> (e.g. \"/bin/bash -c\") to run commands"
#define HELP_CSV "Write statistical summary to CSV <FILE>"
#define HELP_HFCSV "Write Hyperfine-style summary to CSV <FILE>"
#define HELP_PREPARE "Execute <COMMAND> before each benchmarked command"

static void init_exec_options(void) {
  optable_add(OPT_WARMUP,     "w",  "warmup",         1, HELP_WARMUP);
  optable_add(OPT_RUNS,       "r",  "runs",           1, HELP_RUNS);
  optable_add(OPT_PREP,       "p",  "prepare",        1, HELP_PREPARE);
  optable_add(OPT_OUTPUT,     "o",  "output",         1, HELP_OUTPUT);
  optable_add(OPT_FILE,       "f",  "file",           1, HELP_CMDFILE);
  optable_add(OPT_GROUPS,     NULL, "groups",         0, HELP_GROUPS);
  optable_add(OPT_NAME,       "n",  "name",           1, HELP_NAME);
  optable_add(OPT_SHOWOUTPUT, NULL, "show-output",    0, HELP_SHOWOUTPUT);
  optable_add(OPT_IGNORE,     "i",  "ignore-failure", 0, HELP_IGNORE);
  optable_add(OPT_SHELL,      "S",  "shell",          1, HELP_SHELL);
  optable_add(OPT_CSV,        NULL, "export-csv",     1, HELP_CSV);
  optable_add(OPT_HFCSV,      NULL, "hyperfine-csv",  1, HELP_HFCSV);
  optable_add(OPT_GRAPH,      "G",  "graph",          0, HELP_GRAPH);
  optable_add(OPT_REPORT,     "R",  "report",         1, report_options());
  optable_add(OPT_BOXPLOT,    "B",  "boxplot",        0, HELP_BOXPLOT);
  optable_add(OPT_ACTION,     "A",  "action",         1, HELP_ACTION);
  optable_add(OPT_CONFIG,     "x",   NULL,            1, config_help());
  optable_add(OPT_VERSION,    "v",  "version",        0, "Show version");
  optable_add(OPT_HELP,       "h",  "help",           0, "Show help");
  if (optable_error())
    PANIC("failed to configure command-line option parser");
}

// Process the CLI args and set 'config' parameters
void process_exec_options(int argc, char **argv) {
  optable_reset();
  init_exec_options();
  int n, i;
  const char *val;

  // 'i' steps through the args from 1 to argc-1
  i = optable_init(argc, argv);
  if (i < 0) PANIC("failed to initialize option parser");
  while ((i = optable_next(&n, &val, i))) {
    if (n < 0) {
      if (val) {
	if (!config.first) config.first = i;
	continue;
      }
      USAGE("Invalid option/switch '%s'", argv[i]);
    }
    if (config.first) {
      USAGE("Options found after first command '%s'",
	    argv[config.first]);
    }
    switch (n) {
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
	  config.output_to_stdout = true;
	} else {
	  config.output_filename = strdup(val);
	  config.output_to_stdout = false;
	}
	break;
      case OPT_FILE:
	check_option_value(val, n);
	config.input_filename = strdup(val);
	break;
      case OPT_GROUPS:
	check_option_value(val, n);
	config.groups = true;
	break;
      case OPT_SHOWOUTPUT:
	check_option_value(val, n);
	config.show_output = true;
	break;
      case OPT_IGNORE:
	check_option_value(val, n);
	config.ignore_failure = true;
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
      case OPT_GRAPH:
      case OPT_BOXPLOT:
      case OPT_REPORT:
      case OPT_CONFIG:
      case OPT_VERSION:
      case OPT_HELP:
      case OPT_ACTION:
	break;
      default:
	PANIC("invalid option index %d\n", n);
    }
  }
}

// -----------------------------------------------------------------------------
// ACTION 'report' (process raw data, producing reports)
// -----------------------------------------------------------------------------

static void init_report_options(void) {
  optable_add(OPT_GRAPH,      "G",  "graph",          0, HELP_GRAPH);
  optable_add(OPT_CSV,        NULL, "export-csv",     1, HELP_CSV);
  optable_add(OPT_HFCSV,      NULL, "hyperfine-csv",  1, HELP_HFCSV);
  optable_add(OPT_REPORT,     "R",  "report",         1, report_options());
  optable_add(OPT_BOXPLOT,    "B",  "boxplot",        0, HELP_BOXPLOT);
  optable_add(OPT_ACTION,     "A",  "action",         1, HELP_ACTION);
  optable_add(OPT_CONFIG,     "x",   NULL,            1, config_help());
  optable_add(OPT_VERSION,    "v",  "version",        0, "Show version");
  optable_add(OPT_HELP,       "h",  "help",           0, "Show help");
  if (optable_error())
    PANIC("failed to configure command-line option parser");
}

// Process the CLI args and set 'config' parameters
void process_report_options(int argc, char **argv) {
  optable_reset();
  init_report_options();
  int n, i;
  const char *val;
  // 'i' steps through the args from 1 to argc-1
  i = optable_init(argc, argv);
  if (i < 0) PANIC("failed to initialize option parser");
  while ((i = optable_next(&n, &val, i))) {
    if (n < 0) {
      if (val) {
	if (!config.first) config.first = i;
	continue;
      }
      USAGE("Invalid option/switch '%s'", argv[i]);
    }
    if (config.first) {
      USAGE("Options found after first input filename '%s'",
	    argv[config.first]);
    }
    switch (n) {
      case OPT_FILE:
	check_option_value(val, n);
	config.input_filename = strdup(val);
	break;
      case OPT_HFCSV:
	check_option_value(val, n);
	config.hf_filename = strdup(val);
	break;
      case OPT_CSV:
	check_option_value(val, n);
	config.csv_filename = strdup(val);
	break;
      case OPT_GRAPH:
      case OPT_BOXPLOT:
      case OPT_REPORT:
      case OPT_CONFIG:
      case OPT_VERSION:
      case OPT_HELP:
      case OPT_ACTION:
	break;
      default:
	PANIC("invalid option index %d\n", n);
    }
  }
}

// -----------------------------------------------------------------------------
// Print help specific to running experiments or reporting statistics
// -----------------------------------------------------------------------------

void print_help(void) {
  switch (config.action) {
    case actionExecute:
      init_exec_options();
      break;
    case actionReport:
      init_report_options();
      break;
    default:
      init_action_options();
      break;
  }
  optable_printhelp(progname);
}
