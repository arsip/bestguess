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
//     NUL-terminated sequences of arbitrary non-0 bytes,
//     starting with '-', and
//     containing neither '=' nor ASCII whitespace (" \t\n\r")
// - The ShortOption can be omitted by supplying the empty string
// - Every option/switch MUST have a LongOption name
// - A NULL Option name indicates the end of the list
// - The number of values an option can take may be 0 or 1

// NOTE: For options that take a value (OptionNumVals = 1), the value
// is required.  The option must be followed on the command line by
// whitespace or a single equals sign, and then the value.

#define OptionList(X)				\
  X(OPT_WARMUP,     "w",  "warmup",      1)	\
  X(OPT_RUNS,       "r",  "runs",        1)	\
  X(OPT_OUTPUT,     "o",  "output",      1)	\
  X(OPT_INPUT,      "i",  "input",       1)	\
  X(OPT_SHOWOUTPUT, "",   "show-output", 1)	\
  X(OPT_SHELL,      "S",  "shell",       1)	\
  X(OPT_VERSION,    "v",  "version",     0)	\
  X(OPT_HELP,       "h",  "help",        0)	\
  X(OPT_N,          NULL, NULL,          0)

#define X(a, b, c, d) a,
typedef enum Options { OptionList(X) };
#undef X
#define X(a, b, c, d) b,
static const char *const ShortOptions[] = { OptionList(X) };
#undef X
#define X(a, b, c, d) c,
static const char *const LongOptions[] = { OptionList(X) };
#undef X
#define X(a, b, c, d) d,
static const int OptionNumVals[] = { OptionList(X) };
#undef X

int is_option(const char *arg);
int parse_option(const char *arg, const char **value);
int iter_argv(int argc, char *argv[],
	      int *n, const char **value,
	      int i);

char **split(const char *in);

#endif
