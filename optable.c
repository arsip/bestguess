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
  char *shortname;
  char *longname;
  int numvals;
} optable_option;

typedef struct optable_options {
  int count;
  optable_option options[1];
} optable_options;

static const char delimiter = '|';

// Saved parsing state:
static const char *ShortnamePtr = NULL;
static optable_options     *Tbl = NULL;
static int                 Argc = 0;
static char              **Argv = NULL;

// -------------------------------------------------------
// Convenience getters
// -------------------------------------------------------

int optable_count(void) {
  return Tbl ? Tbl->count : 0;
}

const char *optable_shortname(int n) {
  if (Tbl && (n >= 0) && (n < Tbl->count))
    return Tbl->options[n].shortname;
  return NULL;
}

const char *optable_longname(int n) {
  if (Tbl && (n >= 0) && (n < Tbl->count))
    return Tbl->options[n].longname;
  return NULL;
}

int optable_numvals(int n) {
  if (Tbl && (n >= 0) && (n < Tbl->count))
    return Tbl->options[n].numvals;
  return -1;
}

// -------------------------------------------------------
// Dinky parsing utilities
// -------------------------------------------------------

#define MAKE_SKIP_TO(name, predicate)			\
  static const char *skip_##name(const char *str) {	\
    if (!str) return NULL;				\
    while ((*str != '\0') && (predicate)(*str)) str++;	\
    return str;						\
  }

#define MAKE_SKIP_UNTIL(name, predicate)		\
  static const char *until_##name(const char *str) {	\
    if (!str) return NULL;				\
    while ((*str != '\0') && !(predicate)(*str)) str++;	\
    return str;						\
  }

static int digitp(const char c) {
  return ((c >= '0') && (c <= '9'));
}

static int delimiterp(const char c) {
  return (c == delimiter);
}

MAKE_SKIP_TO(delimiters, delimiterp);
MAKE_SKIP_UNTIL(delimiter, delimiterp);

static int whitespacep(const char c) {
  return ((c == ' ') || (c == '\t') ||
	  (c == '\n') || (c == '\r'));
}

MAKE_SKIP_TO(whitespace, whitespacep);
MAKE_SKIP_UNTIL(whitespace, whitespacep);

// -------------------------------------------------------
// String duplication
// -------------------------------------------------------

static char *dup(const char *str, size_t len) {
  char *name = calloc(1, len + 1);
  if (!name) return NULL;
  memcpy(name, str, len);
  return name;
}

// A field that is a single dash, '-', means "no name"
static char *maybe_dup(const char *start, const char *end) {
  if ((*start == '-') && (end == start + 1))
    return NULL;
  else
    return dup(start, end - start);
}

// -------------------------------------------------------
// Parsing one "segment" of a configuration string
// -------------------------------------------------------

// E.g. input is "| show-output 1 | v version 0 |"
//
// Tries to parse one segment (between the delimiters).  Returns a
// pointer to the position where parsing should continue.  When '*err'
// is non-zero, there was an error in the format of the segment.
//
// Note: '*err' is purposely sticky.  If it comes in as 1, it stays 1.
//
static const char *parse_config(const char *p, int *err, optable_option *opt) {
  if (!p) {
    fprintf(stderr, "%s: invalid args to parse_config()\n", __FILE__);
    return NULL;
  }
  if (!*p) return NULL; // Nothing to process
  ssize_t len;
  const char *record_start;
  const char *record_end;
  const char *field_start;
  const char *msg = "missing field";
  p = skip_whitespace(p);
  p = skip_delimiters(p);
  record_start = p;
  record_end = until_delimiter(p);
  // Read shortname
  field_start = skip_whitespace(p);
  if (digitp(*field_start)) goto error;
  p = until_whitespace(field_start);
  // Check for no shortname at all
  if (field_start == p) return NULL;
  if (p > record_end) goto error;
  if (opt) opt->shortname = maybe_dup(field_start, p);
  // Read longname
  field_start = skip_whitespace(p);
  if (digitp(*field_start)) goto error;
  p = until_whitespace(field_start);
  // Check for no longname at all
  if (!*p) goto error;
  if (p > record_end) goto error;
  if (opt) opt->longname = maybe_dup(field_start, p);
  // Read number of values
  p = skip_whitespace(p);
  switch (*p) {
    case '0': if (opt) opt->numvals = 0; break;
    case '1': if (opt) opt->numvals = 1; break;
    default:
      msg = "expected 0 or 1 as last field";
      goto error;
  }
  p++; // Skip the digit 0 or 1
  if (p > record_end) goto error;
  p = skip_whitespace(p);
  if (*p) {
    // End of record but not end of string
    if (delimiterp(*p)) return p + 1;
    msg = "expected delimiter";
    goto error;
  }
  return p;

 error:
  if (err) *err = 1;
  len = record_end - record_start;
  fprintf(stderr, "%s: %s in the config segment '%.*s'\n",
	  __FILE__, msg, (int) len, record_start);
  return record_end;
}

// Count config segments, even if some have errors
static int count_options(const char *str) {
  int err = 0, count = 0;
  while ((str = parse_config(str, &err, NULL))) count++;
  return err ? 0 : count;
}

void optable_free(void) {
  if (!Tbl) return;
  for (int i = 0; i < Tbl->count; i++) {
    free(Tbl->options[i].shortname);
    free(Tbl->options[i].longname);
  }
  free(Tbl);
  Tbl = NULL;
}

// Return error indicator: 0 (false) on success, 1 otherwise
int optable_init(const char *config, int argc, char **argv) {
  int count;
  optable_option opt;

  if (!config || (argc < 1) || !argv) {
    fprintf(stderr, "%s: invalid args to init()\n", __FILE__);
    return 1;
  }

  Argc = argc;
  Argv = argv;
  ShortnamePtr = NULL;

  count = count_options(config);
  if (count <= 0) return 1;

  if (Tbl) optable_free();
  Tbl = malloc(sizeof(optable_options) +
	       (count * sizeof(optable_option)));
  if (!Tbl) return 1;

  Tbl->count = count;
  
  for (int i = 0; i < count; i++) {
    config = parse_config(config, NULL, &opt);
    // We don't test config or err here because count_options() will
    // have flagged parse errors and prevented us from getting here
    Tbl->options[i] = opt;
  }

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
  int count = Tbl->count;
  for (int n = 0; n < count; n++) {
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
  int n;
  int count = Tbl->count;
  for (n = 0; n < count; n++) {
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

// TODO: Put an example here of how to use optable_next()

static int argv_next_is_value(const char *value, int n, int i) {
  return (!value &&
	  !ShortnamePtr &&
	  Tbl->options[n].numvals &&
	  (i + 1 < Argc) &&
	  !optable_is_option(Argv[i+1]));
}

int optable_next(int *n, const char **value, int i) {
  if (!Tbl || !n || !value) {
    fprintf(stderr, "%s: invalid args to next()\n", __FILE__);
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


