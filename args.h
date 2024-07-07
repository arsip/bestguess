//  -*- Mode: C; -*-                                                       
// 
//  args.h
// 
//  COPYRIGHT (C) Jamie A. Jennings, 2024

#ifndef args_h
#define args_h

// One letter name is optional
// Every option/switch MUST have a long name
// The number of values an option takes can be 0 or 1
// When present, an optional default value is a string
#define NUL '\0'
#define OptionList(X)			\
  X('w',  "warmup",      1,  "0")	\
  X('r',  "runs",        1,  "1")	\
  X('o',  "output",      1,  NULL)	\
  X('i',  "input",       1,  NULL)	\
  X(NUL,  "show-output", 1,  NULL)	\
  X('S',  "shell",       1,  NULL)	\
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
static const char *const OptionsDefaults[] = { OptionList(X) };
#undef X


char **split(const char *in);

#endif
