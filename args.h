//  -*- Mode: C; -*-                                                       
// 
//  args.h
// 
//  COPYRIGHT (C) Jamie A. Jennings, 2024

#ifndef args_h
#define args_h

// Rules:
//
// - All option names are:
//     NUL-terminated sequences of arbitrary bytes,
//     starting with '-', and
//     containing neither '=' nor ASCII whitespace (" \t\n\r")
// - The ShortOption (one letter) is optional
// - Every option/switch MUST have a LongOption name
// - A NULL LongOption name indicates the end of the list
// - The number of values an option can take may be 0 or 1
// - When present, the OptionDefault is the default value (a string)

// NOTE: Options that take a value must be followed on the command
// line by whitespace (space/tab) or a single equals sign, and then
// the value

#define NUL '\0'
#define OptionList(X)					\
  X(OPT_WARMUP,     'w',  "warmup",      1,  "0")	\
  X(OPT_RUNS,       'r',  "runs",        1,  "1")	\
  X(OPT_OUTPUT,     'o',  "output",      1,  NULL)	\
  X(OPT_INPUT,      'i',  "input",       1,  NULL)	\
  X(OPT_SHOWOUTPUT, NUL,  "show-output", 1,  "no")	\
  X(OPT_SHELL,      'S',  "shell",       1,  "none")	\
  X(OPT_VERSION,    'v',  "version",     0,  NULL)	\
  X(OPT_HELP,       'h',  "help",        0,  NULL)	\
  X(OPT_N,          NUL,  NULL,          0,  NULL)

#define X(a, b, c, d, e) a,
typedef enum Options { OptionList(X) };
#undef X
#define X(a, b, c, d, e) b,
static const char ShortOptions[] = { OptionList(X) };
#undef X
#define X(a, b, c, d, e) c,
static const char *const LongOptions[] = { OptionList(X) };
#undef X
#define X(a, b, c, d, e) d,
static const int OptionNumVals[] = { OptionList(X) };
#undef X
#define X(a, b, c, d, e) e,
static const char *const OptionDefaults[] = { OptionList(X) };
#undef X

int is_option(const char *arg);
int parse_option(const char *arg, const char **value);
int iter_argv(int argc, char *argv[],
	      int *n, const char **value,
	      int i);

char **split(const char *in);

#endif
