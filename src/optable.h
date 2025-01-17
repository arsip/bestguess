//  -*- Mode: C; -*-                                                       
// 
//  optable.h  A minimal (UTF-8 compatible) CLI arg parsing system
// 
//  COPYRIGHT (C) Jamie A. Jennings, 2024

#ifndef optable_h
#define optable_h

/*
  Rules for defining options:

  - All option names are:
      (1) NUL-terminated sequences of arbitrary non-0 bytes,
      (2) containing neither '=' nor ASCII whitespace (" \t\n\r").
      (3) not starting with a dash '-'
  - A name, short or long, can be omitted by supplying NULL.
  - The number of values an option can take may be 0 or 1.  See notes.

  A SINGLE CLI ARG CAN CONTAIN MULTIPLE SHORTNAME OPTIONS:
  
  The option parser accepts multiple short names that are combined,
  e.g. "-pq" where p and q take no values.  The last one can take a
  value, e.g. if r takes a value, then these are allowed: "-pr 4" or
  "-pr=4".

  NOTE ON MULTIPLE VALUES FOR ONE OPTION:

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
  E.g. "-r5" is not allowed; it is NOT read as "-r=5" or "-r 5".

  Rationale: To support a CLI that uses UTF-8, the short option name
  can be many bytes long.  We don't want to validate UTF-8 (and most
  OSes do not) so we choose to rely on the unambiguous presence of '='
  or (ASCII) whitespace to separate the option name from its value.

*/

#define OPTABLE_DONE 0
#define OPTABLE_ERR -3
#define OPTABLE_NONE -5

int         optable_count(void);
const char *optable_shortname(int n);
const char *optable_longname(int n);
int         optable_numvals(int n);
const char *optable_helptext(int n);

int  optable_iter_start(void);
int  optable_iter_next(int i);

int optable_parse_config(const char *arg,
			 const char **parms,
			 const char **start,
			 const char **end);

void optable_reset(void);
int  optable_add(int n,
		 const char *sname,
		 const char *lname,
		 int numvals,
		 const char *help);
int  optable_init(int argc, char **argv);
int  optable_next(int *n, const char **value, int i);
int  optable_is_option(const char *arg);
void optable_free(void);
int  optable_error(void);

void optable_setusage(const char *usagetext);
void optable_printusage(const char *progname);
void optable_printhelp(const char *progname);

#endif
