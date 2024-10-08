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

#define SECOND(a, b, c, d) b,
const char *ConfigSettingName[] = {XConfig_Settings(SECOND)};
#undef SECOND
#define THIRD(a, b, c, d) c,
const char *ConfigSettingDesc[] = {XConfig_Settings(THIRD)};
#undef THIRD
#define FOURTH(a, b, c, d) d,
const char *ConfigSettingDefault[] = {XConfig_Settings(FOURTH)};
#undef FOURTH

// Throw a usage error or do nothing if all is ok
static void check_option_value(const char *val, int n) {
  if (val && (optable_numvals(n) == 0))
    USAGE("Option '%s' does not take a value",
	  optable_longname(n) ?: optable_shortname(n));
  if (!val && (optable_numvals(n) > 0))
    USAGE("Option '%s' requires a value",
	  optable_longname(n) ?: optable_shortname(n));
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
		     "Setting [default]:");
  for (ConfigCode i = 0; i < CONFIG_LAST; i++) {
    bufsize -= len;
    len += snprintf(buf + len, bufsize, "\n  %-8s %s [%s]",
		    ConfigSettingName[i],
		    ConfigSettingDesc[i],
		    ConfigSettingDefault[i]);
  }
  configuration_help_string = buf;
  return configuration_help_string;
}

void free_config_help(void) {
  free(configuration_help_string);
}

static void set_width(const char *start, const char *end) {
  config.width = buftoint64(start, end);
  if ((config.width < 40) || (config.width > 1024))
    USAGE("Terminal width (%d) is out of range 40..1024", config.width);
}

static void set_alpha(const char *start, const char *end) {
  config.alpha = buftodouble(start, end);
  if ((config.alpha < 0.0) || (config.alpha > 1.0))
    USAGE("Alpha parameter (%f) is out of range 0..1", config.alpha);
}

static void set_effect(const char *start, const char *end) {
  config.effect = buftoint64(start, end);
  if (config.effect < 0)
    USAGE("Minimum effect size parameter (%" PRId64 ") is < 0", config.effect);
}

static void set_epsilon(const char *start, const char *end) {
  config.epsilon = buftoint64(start, end);
  if (config.epsilon < 0)
    USAGE("Confidence interval epsilon parameter (%" PRId64 ") is < 0", config.epsilon);
}

static void set_super(const char *start, const char *end) {
  config.super = buftodouble(start, end);
  if ((config.super < 0.0) || (config.super > 1.0))
    USAGE("Superiority parameter (%f) is out of range 0..1", config.super);
}

static const char *process_config_setting(const char *val) {
  const char *start = val, *end = val;
  int i = 0;
  while (i >= 0) {
    i = optable_parse_config(end, ConfigSettingName, &start, &end);
    switch (i) {
      case OPTABLE_NONE:
	return NULL;		// OK
      case OPTABLE_ERR: 
	return start;		// ERR
      case CONFIG_WIDTH:
	set_width(start, end);
	continue;
      case CONFIG_ALPHA:
	set_alpha(start, end);
	continue;
      case CONFIG_EFFECT:
	set_effect(start, end);
	continue;
      case CONFIG_EPSILON:
	set_epsilon(start, end);
	continue;
      case CONFIG_SUPER:
	set_super(start, end);
	continue;
      default:
	PANIC("Unhandled configuration setting (%d)", i);
    }
  }
  return NULL;			// OK
}

// Any value < 0 means "uninitialized"
void set_config_defaults(void) {
  if (config.width < 0)
    set_width(ConfigSettingDefault[CONFIG_WIDTH], NULL);
  if (config.alpha < 0)
    set_alpha(ConfigSettingDefault[CONFIG_ALPHA], NULL);
  if (config.alpha < 0)
    set_effect(ConfigSettingDefault[CONFIG_EFFECT], NULL);
  if (config.epsilon < 0)
    set_epsilon(ConfigSettingDefault[CONFIG_EPSILON], NULL);
  if (config.super < 0)
    set_super(ConfigSettingDefault[CONFIG_SUPER], NULL);
}

static void show_setting(int n) {
  if ((n < 0) || (n >= CONFIG_LAST))
    PANIC("Config setting index (%d) out of range", n);
  printf("%7s = ", ConfigSettingName[n]);
  switch (n) {
    case CONFIG_WIDTH:
      printf("%d\n", config.width);
      break;
    case CONFIG_EFFECT:
      printf("%" PRId64 "\n", config.effect);
      break;
    case CONFIG_EPSILON:
      printf("%" PRId64 "\n", config.epsilon);
      break;
    case CONFIG_ALPHA:
      printf("%4.2f\n", config.alpha);
      break;
    case CONFIG_SUPER:
      printf("%4.2f\n", config.super);
      break;
    default:
      PANIC("Config setting index (%d) out of range", n);
  }
}

void show_config_settings(void) {
  show_setting(CONFIG_WIDTH);
  show_setting(CONFIG_ALPHA);
  show_setting(CONFIG_EFFECT);
  show_setting(CONFIG_EPSILON);
  show_setting(CONFIG_SUPER);
}

#define HELP_EXPLAIN "Show an explanation of the inferential statistics"
#define HELP_GRAPH "Show graph of total time for each iteration"
#define HELP_BOXPLOT "Show box plots of timing data"
#define HELP_ACTION							\
  "In rare circumstances, the Bestguess executables\n"			\
  "are installed under custom names.  In that case, the\n"		\
  "<ACTION> option is required.  See the manual for more." 

static void init_action_options(void) {
  optable_add(OPT_REPORT,     "R",  "report",  1, report_help());
  optable_add(OPT_GRAPH,      "G",  "graph",   0, HELP_GRAPH);
  optable_add(OPT_BOXPLOT,    "B",  "boxplot", 0, HELP_BOXPLOT);
  optable_add(OPT_EXPLAIN,    "E",  "explain", 0, HELP_EXPLAIN);
  optable_add(OPT_ACTION,     "A",  "action",  1, HELP_ACTION);
  optable_add(OPT_CONFIG,     "x",   NULL,     1, config_help());
  optable_add(OPT_SHOWCONFIG, NULL, "config",  0, "Show configuration settings");
  optable_add(OPT_VERSION,    "v",  "version", 0, "Show version");
  optable_add(OPT_HELP,       "h",  "help",    0, "Show help");
  if (optable_error())
    PANIC("Failed to configure command-line option parser");
}

// Check if the ACTION option is given, looking also for HELP and
// VERSION because those override the requested ACTION, and for the
// CLI options that are common to all actions.
void process_common_options(int argc, char **argv) {
  optable_reset();
  init_action_options();
  int n, i;
  const char *val;
  // 'i' steps through the args from 1 to argc-1
  i = optable_init(argc, argv);
  if (i < 0) PANIC("Failed to initialize option parser");
  while ((i = optable_next(&n, &val, i))) {
    // Skip any CLI args that are not options:
    if (n == OPTABLE_NONE) continue;
    // Will process other options later:
    if (n == OPTABLE_ERR) continue;
    switch (n) {
      case OPT_VERSION:
	if (option.helpversion == -1)
	  option.helpversion = OPT_VERSION;
	break;
      case OPT_HELP:
	option.helpversion = OPT_HELP;
	break;
      case OPT_SHOWCONFIG:
	option.helpversion = OPT_SHOWCONFIG;
	break;
      case OPT_EXPLAIN:
	option.explain = true;
	break;
      case OPT_ACTION:
	if (val && (strcmp(val, CLI_OPTION_EXPERIMENT) == 0)) {
	  option.action = actionExecute;
	  break;
	} else if (val && (strcmp(val, CLI_OPTION_REPORT) == 0)) {
	  option.action = actionReport;
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
	option.boxplot = true;
	break;
      case OPT_REPORT:
	check_option_value(val, n);
	option.report = interpret_report_option(val);
	if (option.report == REPORT_ERROR)
	  USAGE(report_help());
	break;
      case OPT_CONFIG:
	check_option_value(val, n);
	const char *err = process_config_setting(val);
	if (err)
	  USAGE("Invalid configuration setting '%s'", err);
	break;
      case OPT_GRAPH:
	check_option_value(val, n);
	option.graph = true;
	break;
      default:
	PANIC("Invalid option index %d\n", n);
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
  optable_add(OPT_NAME,       "n",  "name",           1, HELP_NAME);
  optable_add(OPT_SHOWOUTPUT, NULL, "show-output",    0, HELP_SHOWOUTPUT);
  optable_add(OPT_IGNORE,     "i",  "ignore-failure", 0, HELP_IGNORE);
  optable_add(OPT_SHELL,      "S",  "shell",          1, HELP_SHELL);
  optable_add(OPT_CSV,        NULL, "export-csv",     1, HELP_CSV);
  optable_add(OPT_HFCSV,      NULL, "hyperfine-csv",  1, HELP_HFCSV);
  optable_add(OPT_GRAPH,      "G",  "graph",          0, HELP_GRAPH);
  optable_add(OPT_REPORT,     "R",  "report",         1, report_help());
  optable_add(OPT_BOXPLOT,    "B",  "boxplot",        0, HELP_BOXPLOT);
  optable_add(OPT_EXPLAIN,    "E",  "explain",        0, HELP_EXPLAIN);
  optable_add(OPT_ACTION,     "A",  "action",         1, HELP_ACTION);
  optable_add(OPT_CONFIG,     "x",   NULL,            1, config_help());
  optable_add(OPT_VERSION,    "v",  "version",        0, "Show version");
  optable_add(OPT_HELP,       "h",  "help",           0, "Show help");
  if (optable_error())
    PANIC("Failed to configure command-line option parser");
}

// Process the CLI args and set 'config' parameters
void process_exec_options(int argc, char **argv) {
  optable_reset();
  init_exec_options();
  int n, i;
  const char *val;

  // 'i' steps through the args from 1 to argc-1
  i = optable_init(argc, argv);
  if (i < 0) PANIC("Failed to initialize option parser");
  while ((i = optable_next(&n, &val, i))) {
    if (n == OPTABLE_NONE) {
      if (!val) PANIC("Expected cli argument value");
      if (!option.first) option.first = i;
      continue;
    }
    if (n == OPTABLE_ERR) {
      USAGE("Invalid option/switch '%s'", argv[i]);
    }
    if (option.first) {
      USAGE("Options found after first command '%s'",
	    argv[option.first]);
    }
    switch (n) {
      case OPT_WARMUP:
	check_option_value(val, n);
	option.warmups = strtoint64(val);
	if ((option.warmups < 0) || (option.warmups > MAXRUNS))
	  USAGE("Number of warmup runs is out of range 0..%d", MAXRUNS);
	break;
      case OPT_RUNS:
	check_option_value(val, n);
	option.runs = strtoint64(val);
	if ((option.runs < 0) || (option.runs > MAXRUNS))
	  USAGE("Number of timed runs is out of range 0..%d", MAXRUNS);
	break;
      case OPT_OUTPUT:
	check_option_value(val, n);
	if (strcmp(val, "-") == 0) {
	  option.output_to_stdout = true;
	} else {
	  option.output_filename = strdup(val);
	  option.output_to_stdout = false;
	}
	break;
      case OPT_FILE:
	check_option_value(val, n);
	option.input_filename = strdup(val);
	break;
      case OPT_SHOWOUTPUT:
	check_option_value(val, n);
	option.show_output = true;
	break;
      case OPT_IGNORE:
	check_option_value(val, n);
	option.ignore_failure = true;
	break;
      case OPT_SHELL:
	check_option_value(val, n);
	option.shell = val;
	break;
      case OPT_HFCSV:
	check_option_value(val, n);
	option.hf_filename = strdup(val);
	break;
      case OPT_CSV:
	check_option_value(val, n);
	option.csv_filename = strdup(val);
	break;
      case OPT_PREP:
	check_option_value(val, n);
	option.prep_command = strdup(val);
	break;
      default:
	break;
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
  optable_add(OPT_REPORT,     "R",  "report",         1, report_help());
  optable_add(OPT_BOXPLOT,    "B",  "boxplot",        0, HELP_BOXPLOT);
  optable_add(OPT_EXPLAIN,    "E",  "explain",        0, HELP_EXPLAIN);
  optable_add(OPT_ACTION,     "A",  "action",         1, HELP_ACTION);
  optable_add(OPT_CONFIG,     "x",   NULL,            1, config_help());
  optable_add(OPT_VERSION,    "v",  "version",        0, "Show version");
  optable_add(OPT_HELP,       "h",  "help",           0, "Show help");
  if (optable_error())
    PANIC("Failed to configure command-line option parser");
}

// Process the CLI args and set 'config' parameters
void process_report_options(int argc, char **argv) {
  optable_reset();
  init_report_options();
  int n, i;
  const char *val;
  // 'i' steps through the args from 1 to argc-1
  i = optable_init(argc, argv);
  if (i < 0) PANIC("Failed to initialize option parser");
  while ((i = optable_next(&n, &val, i))) {
    if (n == OPTABLE_NONE) {
      if (!val) PANIC("Expected cli argument value");
      if (!option.first) option.first = i;
      continue;
    }
    if (n == OPTABLE_ERR) {
      USAGE("Invalid option/switch '%s'", argv[i]);
    }
    if (option.first) {
      USAGE("Options found after first input filename '%s'",
	    argv[option.first]);
    }
    switch (n) {
      case OPT_FILE:
	check_option_value(val, n);
	option.input_filename = strdup(val);
	break;
      case OPT_HFCSV:
	check_option_value(val, n);
	option.hf_filename = strdup(val);
	break;
      case OPT_CSV:
	check_option_value(val, n);
	option.csv_filename = strdup(val);
	break;
      default:
	break;
    }
  }
}

// -----------------------------------------------------------------------------
// Print help specific to running experiments or reporting statistics
// -----------------------------------------------------------------------------

void print_help(void) {
  switch (option.action) {
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
