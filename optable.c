//  -*- Mode: C; -*-                                                       
// 
//  optable.c  A minimal (UTF-8 compatible) CLI arg parsing system
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

int optable_numvals(int n) {
  if (Tbl && (n >= 0) && (n < Tbl_size))
    return Tbl[n].numvals;
  return -1;
}

// Convenience function.  Users can check the result of each call to
// optable_add() for errors, or they can ignore them and call
// optable_error() this after a series of optable_add() calls.
//
// Note that optable_add() prints messages to stderr as they occur.
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
  return -1;
}

int optable_iter_next(int i) {
  i++;
  if (i < 0) return -1;
  for (; i < Tbl_size; i++)
    if (Tbl[i].shortname || Tbl[i].longname) return i;
  return -1;
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

// Return error code: 1 for out of memory, 0 for success
static int table_init(void) {
  const size_t initsize = 1;
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
// Takes ownership of all of the string arguments.
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
    return -1;
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
// points to the value part e.g. "5" in "-r=5".  If there is no '=',
// then '*value' will be NULL.
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
      return -1;
    }
  } // for each possible option name
  return -1;
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
  return -1;
}

int optable_is_option(const char *arg) {
  return (arg && (*arg == '-'));
}

static int argv_next_is_value(const char *value, int n, int i) {
  return (!value &&
	  !ShortnamePtr &&
	  Tbl[n].numvals &&
	  (i + 1 < Argc) &&
	  !optable_is_option(Argv[i+1]));
}

int optable_next(int *n, const char **value, int i) {
  if (!Tbl || !n || !value) {
    fprintf(stderr, "%s: invalid args to iterator\n", __FILE__);
    return 0;
  }
  if ((i < 1) && ShortnamePtr) {
    fprintf(stderr, "%s: inconsistent state in iterator\n", __FILE__);
    return 0;
  }
  if (ShortnamePtr) {
    // Already parsing multiple shortname options like "-plt" or "-plr=5"
    if ((i < 1) || (i >= Argc)) return 0;
    *n = match_short_option(ShortnamePtr, value);
    if (*n < 0) {
      // Error.  Return '*value' that points to the bad part of Argv[i]
      ShortnamePtr = NULL;
      return i;
    }
    if (argv_next_is_value(*value, *n, i))
      *value = Argv[++i];
    return i;
  } 
  // Not already parsing multiple shortname options
  i++;
  if ((i < 1) || (i >= Argc)) return 0;
  if (optable_is_option(Argv[i])) {
    *n = match_short_option(Argv[i]+1, value);
    if (*n < 0) {
      if (optable_is_option(Argv[i]+1)) 
	*n = match_long_option(Argv[i]+2, value);
    } 
    if (*n < 0) return i;
    if (argv_next_is_value(*value, *n, i))
      *value = Argv[++i];
    return i;
  }
  // Else this arg is not an option/switch, so return it
  *n = -1;
  *value = Argv[i];
  return i;
}


