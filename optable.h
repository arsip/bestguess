//  -*- Mode: C; -*-                                                       
// 
//  optable.h  A minimal CLI arg parsing system
// 
//  COPYRIGHT (C) Jamie A. Jennings, 2024

#ifndef optable_h
#define optable_h

/*
  Rules:

  - All option names are:
      (1) NUL-terminated sequences of arbitrary non-0 bytes,
      (2) starting with '-', and
      (3) containing neither '=' nor ASCII whitespace (" \t\n\r")
  - The ShortOption can be omitted by supplying the empty string.
  - Every option/switch MUST have a LongOption name.
  - A NULL Option name indicates the end of the list.
  - The number of values an option can take may be 0 or 1.  See notes.

  NOTE ON MULTIPLE VALUES:

  You can accept zero or more (set OptionNumVals to 0) using the
  iterator, i.e. "i = optable_iter(...)".

  When you see the option that takes any number of values, consume as
  many as you want from argv[] starting at i+1.  Remember to check
  optable_is_option() to know when you've run out of values and
  encountered another option/switch.

  NOTE ON REQUIRED VALUES: 

  For options that take a value (OptionNumVals = 1), the value is
  required.  The option must be followed on the command line by
  whitespace or a single equals sign, and then the value.  We do NOT
  accept the short option name followed immediately by the value.
  E.g. "-r5" is NOT "-r=5" or "-r 5".

  Rationale: To support a CLI that uses UTF-8, the short option name
  can be many bytes long.  We don't want to validate UTF-8 (and most
  OSes do not) so we choose to rely on the unambiguous presence of '='
  or (ASCII) whitespace to separate the option name from its value.

*/

typedef struct optable_option {
  char *shortname;
  char *longname;
  int numvals;
} optable_option;

typedef struct optable_options {
  int count;
  optable_option options[1];
} optable_options;

const char *optable_shortname(optable_options *tbl, int n);
const char *optable_longname(optable_options *tbl, int n);
int         optable_numvals(optable_options *tbl, int n);

optable_options *optable_init(const char *const config);
void             optable_free(optable_options *opts);

int optable_is_option(const char *arg);

int optable_parse_option(optable_options *tbl,
			 const char *arg,
			 const char **value);

int optable_iter(optable_options *tbl,
		 int argc, char *argv[],
		 int *n, const char **value,
		 int i);

#endif
