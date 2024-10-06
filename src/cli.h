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
  OPT_IGNORE,
  OPT_SHOWOUTPUT,
  OPT_SHELL,
  OPT_NAME,
  OPT_OUTPUT,			// Raw data output
  OPT_CSV,			// BestGuess-format summary CSV
  OPT_HFCSV,			// Hyperfine-format summary CSV
  OPT_FILE,			// Input file of commands
  OPT_GROUPS,
  OPT_BRIEF,
  OPT_GRAPH,
  OPT_REPORT,
  OPT_BOXPLOT,
  OPT_EXPLAIN,			// Explain the inferential statistics
  OPT_ACTION,			// E.g. run, report
  OPT_CONFIG,			// Settings: -x key=value
  OPT_SHOWCONFIG,	        // Show config parameter settings
  OPT_VERSION,
  OPT_HELP,
};

#define XConfig_Settings(X)						         \
 X(CONFIG_WIDTH,   "width",   "Maximum terminal width for graphs, plots", "80")  \
 X(CONFIG_ALPHA,   "alpha",   "Alpha value for statistics", ".05")	         \
 X(CONFIG_EPSILON, "epsilon", "Epsilon for confidence intervals (μsec)", "250")  \
 X(CONFIG_EFFECT,  "effect",  "Minimum effect size (μsec)", "500")	         \
 X(CONFIG_SUPER,   "super",   "Superiority threshold (probability)", ".333")     \
 X(CONFIG_LAST,     NULL,     "SENTINEL", "SENTINEL")

#define FIRST(a, b, c, d) a,
typedef enum { XConfig_Settings(FIRST) } ConfigCode;
#undef FIRST
extern const char *ConfigSettingName[];
extern const char *ConfigSettingDesc[];
extern const char *ConfigSettingDefault[];

void print_help(void);

void process_common_options(int argc, char **argv);
void process_exec_options(int argc, char **argv);
void process_report_options(int argc, char **argv);

void set_config_defaults(void);
void show_config_settings(void);
void free_config_help(void);

#endif



