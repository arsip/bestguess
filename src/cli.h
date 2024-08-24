//  -*- Mode: C; -*-                                                       
// 
//  cli.h
// 
//  (C) Jamie A. Jennings, 2024

#ifndef cli_exec_h
#define cli_exec_h

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
  OPT_REPORT,
  OPT_BOXPLOT,
  OPT_ALL,
  OPT_ACTION,			// E.g. run, report
  OPT_VERSION,
  OPT_HELP,
};

void print_help(void);

void process_action_options(int argc, char **argv);
void process_exec_options(int argc, char **argv);
void process_report_options(int argc, char **argv);

#endif



