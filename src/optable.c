//  -*- Mode: C; -*-                                                       
// 
//  optable.c  A minimal (but UTF-8 compatible) CLI arg parser
// 
//  COPYRIGHT (C) Jamie A. Jennings, 2024

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "optable.h"

typedef struct optable_option {
  const char *shortname;
  const char *longname;
  const char *help;
  int numvals;
} optable_option;

// Option configuration:
static optable_option      *Tbl = NULL;
static int             Tbl_size = 0;
static int             Tbl_next = 0;
static int              Tbl_err = 0;
static const char        *Usage = NULL;

// Parsing state:
static const char *ShortnamePtr = NULL;
static int                 Argc = 0;
static char              **Argv = NULL;

__attribute__((unused))
static void dump_table_info(void) {
  printf("Tbl      = %p\n", Tbl);
  printf("Tbl_size = %d\n", Tbl_size);
  printf("Tbl_next = %d\n", Tbl_next);
  printf("Tbl_err  = %d\n", Tbl_err);
  printf("ShortnamePtr = %p\n", ShortnamePtr);
}

// -------------------------------------------------------
// Convenience getters
// -------------------------------------------------------

const char *optable_shortname(int n) {
  if (Tbl && (n >= 0) && (n < Tbl_size))
    return Tbl[n].shortname;
  return NULL;
}

const char *optable_longname(int n) {
  if (Tbl && (n >= 0) && (n < Tbl_size))
    return Tbl[n].longname;
  return NULL;
}

const char *optable_helptext(int n) {
  if (Tbl && (n >= 0) && (n < Tbl_size))
    return Tbl[n].help;
  return NULL;
}

int optable_numvals(int n) {
  if (Tbl && (n >= 0) && (n < Tbl_size))
    return Tbl[n].numvals;
  return OPTABLE_ERR;
}

// Convenience function.  Users can check the result of each call to
// optable_add() for errors, or they can ignore them and call
// optable_error() after a series of optable_add() calls.
//
// NOTE: optable_add() prints messages to stderr as they occur.
//
int optable_error(void) {
  return Tbl_err;
}

// -------------------------------------------------------
// Iterate over the options themselves (not the cli args)
// -------------------------------------------------------

// The table of defined options, Tbl, is built using optable_add(),
// and the caller supplies the (non-negative) option number, which is
// the index into Tbl.  Thus, we cannot rely on these numbers being
// consecutive.

int optable_iter_start(void) {
  for (int i = 0; i < Tbl_size; i++)
    if (Tbl[i].shortname || Tbl[i].longname) return i;
  return OPTABLE_NONE;
}

int optable_iter_next(int i) {
  i++;
  if (i < 0) return OPTABLE_NONE;
  for (; i < Tbl_size; i++)
    if (Tbl[i].shortname || Tbl[i].longname) return i;
  return OPTABLE_NONE;
}

// Count the number of defined options
int optable_count(void) {
  int count = 0;
  for (int i = 0; i < Tbl_size; i++)
    if (Tbl[i].shortname || Tbl[i].longname) count++;
  return count;
}

// -------------------------------------------------------
// Create the table of defined options
// -------------------------------------------------------

// Can also be used to reset to initial state.  
void optable_free(void) {
  if (Tbl) free(Tbl);
  Tbl = NULL;
  Tbl_size = 0;
  Tbl_next = 0;
  Tbl_err = 0;
  ShortnamePtr = NULL;
  Argc = 0;
  Argv = NULL;
}

void optable_reset(void) {
  optable_free();
}  

// Return error code: 1 for out of memory, 0 for success
static int table_init(void) {
  const size_t initsize = 40;
  Tbl = calloc(initsize, sizeof(optable_option));
  if (!Tbl) return 1;
  Tbl_size = initsize;
  return 0;
}

// Return error code: 1 for out of memory, 0 for success
static int ensure_space(int n) {
  if (n < 0) return 1;
  size_t newsize;
  if (!Tbl && table_init()) return 1;
  while (Tbl_size <= n) {
    newsize = 2 * Tbl_size;
    Tbl = realloc(Tbl, newsize * sizeof(optable_option));
    if (!Tbl) return 1;
    memset(&(Tbl[Tbl_size]), 0, Tbl_size * sizeof(optable_option));
    Tbl_size = newsize;
  }
  return 0;
}

// Returns error indicator: 1 for error, 0 for no error.
// Takes ownership of all of the string arguments except 'help'.
int optable_add(int n,
		const char *sname,
		const char *lname,
		int numvals,
		const char *help) {
  if (n < 0) {
    fprintf(stderr, "%s: option index out of range (received %d)\n", __FILE__, n);
    Tbl_err = 1;
    return 1;
  }
  if (!sname && !lname) {
    fprintf(stderr, "%s: one of shortname or longname is required\n", __FILE__);
    Tbl_err = 1;
    return 1;
  }
  if ((sname && !*sname) || (lname && !*lname)) {
    fprintf(stderr, "%s: option name, if not NULL, must not be empty string\n", __FILE__);
    Tbl_err = 1;
    return 1;
  }
  if ((numvals != 0) && (numvals != 1)) {
    fprintf(stderr, "%s: numvals must be 0 or 1 (received %d)\n", __FILE__, numvals);
    Tbl_err = 1;
    return 1;
  }
  if (ensure_space(n)) return 1;
  Tbl[n].shortname = sname;
  Tbl[n].longname = lname;
  Tbl[n].numvals = numvals;
  Tbl[n].help = help;
  return 0;
}

// Returns initial value needed to call the iterator optable_next(),
// or a negative value on error
int optable_init(int argc, char **argv) {
  if ((argc < 1) || !argv) {
    fprintf(stderr, "%s: invalid args to init()\n", __FILE__);
    return OPTABLE_ERR;
  }
  Argc = argc;
  Argv = argv;
  ShortnamePtr = NULL;
  return 0;
}

// It's cleaner and safer to have a custom comparison function than to
// use libc functions like 'strlen' and 'strcmp'.
//
// Match ==> Returns a pointer within str to next byte after match
// No match ==> Returns NULL
//
static const char *compare(const char *str, const char *name) {
  if (!str || !*str || !name || !*name) return NULL;
  while (*name) {
    // Unnecessary check for end of str is elided below
    if (*str != *name) return NULL;
    str++; name++;
  }
  return str;
}

// On success, return value is the option index (>= 0), and '*value'
// points to the value part if it attached to the option name using
// '=', e.g. "--foo=5".  If the option name is not immediately
// followed by '=', then '*value' will be NULL.
static int match_long_option(const char *arg, const char **value) {
  for (int n = 0; n < Tbl_size; n++) {
    *value = compare(arg, optable_longname(n));
    if (*value) {
      if (**value == '\0') {
	*value = NULL;
	return n;
      }
      if (**value == '=') {
	(*value)++;
	return n;
      }
      return OPTABLE_ERR;
    }
  } // for each possible option name
  return OPTABLE_ERR;
}

static int match_short_option(const char *arg, const char **value) {
  for (int n = 0; n < Tbl_size; n++) {
    *value = compare(arg, optable_shortname(n));
    if (*value) {
      if (**value == '\0') {
	ShortnamePtr = NULL;
	*value = NULL;
	return n;
      }
      if (**value == '=') {
	ShortnamePtr = NULL;
	(*value)++;
	return n;
      }
      // Might have multiple short names, e.g. -plt
      // Will get the next one on the next iteration.
      ShortnamePtr = *value;
      *value = NULL;
      return n;
    }
  } // for each possible option name
  return OPTABLE_ERR;
}

// Ensure 'arg' is non-null before calling
static int all_dashes(const char *arg) {
  while (*arg)
    if (*(arg++) != '-') return 0;
  return 1;
}

// A single dash, or any sequence of dashes, is a VALUE on the command
// line, not some kind of empty option or switch
int optable_is_option(const char *arg) {
  return arg && (*arg == '-') && !all_dashes(arg);
}

static int still_need_value(const char *value, int n) {
  return (!value && !ShortnamePtr && Tbl[n].numvals);
}

// On success (non-zero return value), '*n' is set to the option
// number encountered (> 0) or OPTABLE_NONE (< 0) to signal that the
// current arg is not an option.
//
// If the current arg starts with - or --, but is not a valid option
// name, OPTABLE_ERR is returned.
//
// Returns OPTABLE_DONE (zero) when finished.
//
// On success, '*value' is set to the option value.  In the case of
// the "equals syntax" (e.g. "-r=5" or "--foo=bar") '*value' points to
// the first character after the '='.  The ordinary syntax (e.g. "-r
// 5" or "--foo bar") results in '*value' pointing to the next argv.
//
int optable_next(int *n, const char **value, int i) {
  if (!Tbl || !n || !value) {
    fprintf(stderr, "%s: invalid args to iterator\n", __FILE__);
    return OPTABLE_ERR;
  }
  if ((i < 1) && ShortnamePtr) {
    fprintf(stderr, "%s: inconsistent state in iterator\n", __FILE__);
    return OPTABLE_ERR;
  }
  //
  // Are we in the middle of parsing multiple shortname options like
  // "-plt" or "-plr=5"?
  //
  if (ShortnamePtr) {
    if ((i < 1) || (i >= Argc)) return OPTABLE_DONE;
    *n = match_short_option(ShortnamePtr, value);
    if (*n < 0) {
      // Error.  Return '*value' that points to the bad part of Argv[i]
      ShortnamePtr = NULL;
      return i;
    }
    // Since argv ends in a NULL, we can index ahead
    if (still_need_value(*value, *n))
      *value = Argv[++i];
    return i;
  } 
  //
  // Not already parsing multiple shortname options
  //
  i++;
  if ((i < 1) || (i >= Argc)) return OPTABLE_DONE;
  if (optable_is_option(Argv[i])) {
    *n = match_short_option(Argv[i]+1, value);
    if (*n < 0) {
      if (optable_is_option(Argv[i]+1)) 
	*n = match_long_option(Argv[i]+2, value);
    } 
    if (*n < 0) return i;
    // Since argv always has a NULL at the end, we can index ahead
    if (still_need_value(*value, *n))
      *value = Argv[++i];
    return i;
  }
  // Else this arg is not an option/switch, so return it
  *n = OPTABLE_NONE;
  *value = Argv[i];
  return i;
}

void optable_setusage(const char *usagetext) {
  Usage = usagetext;
}

void optable_printusage(const char *progname) {
  fprintf(stderr, "Usage: %s %s\n", progname, Usage ?: "");
  fflush(stderr);
}

void optable_printhelp(const char *progname) {
  int i;
  optable_printusage(progname);
  printf("\n");
  for (i = optable_iter_start(); i >= 0; i = optable_iter_next(i)) {
    printf("  %1s%-1s  %2s%-14s  ",
	   optable_shortname(i) ? "-" : " ",
	   optable_shortname(i) ?: "",
	   optable_longname(i) ? "--": "  ",
	   optable_longname(i) ?: "");
    const char *help = optable_helptext(i);
    const char *nl;
    while ((nl = strchr(help, '\n'))) {
      printf("%.*s\n", (int) (nl - help), help);
      help = nl + 1;
      printf("%*s", 24, "");
    }
    printf("%s\n", help);
  } // for each program option
  fflush(stdout);
}

// -----------------------------------------------------------------------------
// Parsing utilities
// -----------------------------------------------------------------------------

static int commap(const char c) {
  return (c == ',');
}

static const char *until(const char *str, int stop(const char c)) {
  if (!str) return NULL;
  while ((*str != '\0') && !stop(*str)) str++;
  return str;
}

// Find a char that matches 'c', returning a pointer to one byte
// beyond it.  Ignore 'c' if it appears escaped with backslash.
// Returns NULL if 'c' never appears un-escaped.
static const char *match(const char *str, char c) {
  if (!str) return NULL;
  while (*str != '\0') {
    if (*str == c) return str + 1;
    if ((*str == '\\') && (*(str+1) == c)) str++;
    str++;
  }
  return NULL;
}

// Return ptr to one byte beyond the last byte of a value.  The value
// starts at 'p' and may be bare or quoted (single or double).
static const char *read_value(const char *p, int stop(const char c)) {
  if (*p == '\"') {
    return match(++p, '\"');
  } else if (*p == '\'') {
    return match(++p, '\'');
  } else {
    p = until(p, stop);
    return p;
  }
}

// -----------------------------------------------------------------------------
// Parse configuration-style options, e.g. -D width=120,height=32
// -----------------------------------------------------------------------------

// On success, return value is the config index (>= 0), 'start' points
// to the value part of the arg string, and 'end' points one byte past
// the last character of the value.  Note that 'end' may point to
// whitespace or NUL.
//
// If the configuration parameter name is followed by '=' but no
// value, then 'start' and 'end' will both point to the byte after the
// '='.  This is a zero-length value string.
//
// Usage: Suppose -D is a 'define' option that takes a value.  Valid
// uses include these:
//
//    -D width=80
//    -D=width=80
//    -D="width=80"
//    -D width=
//    -D width=80,height=32,colors='red:0xFF0000;blue:0x0000FF'
//    -D width=,height=32,depth=4
//
// Call 'optable_parse_config' with 'arg' pointing at the value
// argument, e.g. width=80.  'parms' should be a NULL-terminated list
// of config parameter names, in this example it should include
// "width".  If "width" is first, 'optable_parse_config' will return
// index 0 (into 'parms'), 'start' will point to 80, and 'end' will
// point one byte past 80.
//
// If 'end' points at NUL, there are no more configuration parameters
// in 'arg'.  Caller can check that condition or simply call
// 'optable_parse_config' again using 'end' as the new 'arg'.  To get
// all parameters, keep calling until OPTABLE_NONE is returned.
//  
// SYNTAX ERROR: If the configuration parameter name is NOT followed
// by '=', the syntax is illegal and OPTABLE_ERR is returned.
//
// SYNTAX ERROR: If the configuration parameter does not match any of
// the 'parms' (a NULL-terminated list of config names), then
// OPTABLE_ERR is returned.
//
int optable_parse_config(const char *arg,
			 const char **parms,
			 const char **start,
			 const char **end) {
  if (!arg || !parms || !start || !end) return OPTABLE_ERR;
  int idx = 0;
  if (commap(*arg)) arg++;
  if (!*arg) return OPTABLE_NONE;
  while (*parms) {
    *start = compare(arg, *parms);
    if (*start) {
      if (**start == '=') {
	// Read the value, which may be quoted.
	*end = read_value(++(*start), commap);
      }
      return idx;
    }
    parms++;
    idx++;
  } // for each possible option name
  return OPTABLE_ERR;
}
