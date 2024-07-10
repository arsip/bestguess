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
  - Short options can be combined, though only the last one can take
    a value. E.g. "-pq" where p and q take 0 values, or "-pr 4" where
    r takes a value.

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

int         optable_count(void);
const char *optable_shortname(int n);
const char *optable_longname(int n);
int         optable_numvals(int n);

int optable_init(const char *config, int argc, char **argv);

void optable_free(void);

int optable_is_option(const char *arg);

int optable_parse_option(const char *arg, const char **value);

int optable_iter(int *n, const char **value, int i);

#endif
