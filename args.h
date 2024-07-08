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
#define OptionList(X)			\
  X('w',  "warmup",      1,  "0")	\
  X('r',  "runs",        1,  "1")	\
  X('o',  "output",      1,  NULL)	\
  X('i',  "input",       1,  NULL)	\
  X(NUL,  "show-output", 1,  "no")	\
  X('S',  "shell",       1,  "none")	\
  X('v',  "version",     0,  NULL)	\
  X('h',  "help",        0,  NULL)	\
  X(NUL,  NULL,          0,  NULL)

#define X(a, b, c, d) a,
static const char ShortOptions[] = { OptionList(X) };
#undef X
#define X(a, b, c, d) b,
static const char *const LongOptions[] = { OptionList(X) };
#undef X
#define X(a, b, c, d) c,
static const int OptionNumVals[] = { OptionList(X) };
#undef X
#define X(a, b, c, d) d,
static const char *const OptionDefaults[] = { OptionList(X) };
#undef X

int is_option(const char *arg);
int parse_option(const char *arg, const char **value);


char **split(const char *in);

#endif
