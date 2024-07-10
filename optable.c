//  -*- Mode: C; -*-                                                       
// 
//  optable.c  A minimal CLI arg parsing system
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

static char delimiter = '|';
static int Argc = 0;
static char **Argv = NULL;
static optable_options *Tbl = NULL;

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

// E.g.
// "w warmup 1, r runs 1, show-output 1, v version 0"
//
static const char *parse_config(const char *p, optable_option *opt) {
  if (!p) {
    fprintf(stderr, "%s: invalid args to parse_config\n", __FILE__);
    return NULL;
  }
  if (!*p) return NULL;
  ssize_t len;
  const char *record_start;
  const char *record_end;
  const char *field_start;
  const char *msg = "missing field";
  p = skip_whitespace(p);
  p = skip_delimiters(p);
  record_start = p;
  record_end = until_delimiter(p);
  field_start = skip_whitespace(p);
  if (digitp(*field_start)) goto error;
  p = until_whitespace(field_start);
  if (!*p) return NULL;
  if (p > record_end) goto error;
  if (opt) opt->shortname = maybe_dup(field_start, p);
  field_start = skip_whitespace(p);
  if (digitp(*field_start)) goto error;
  p = until_whitespace(field_start);
  if (!*p) return NULL;
  if (p > record_end) goto error;
  if (opt) opt->longname = maybe_dup(field_start, p);
  p = skip_whitespace(p);
  switch (*p) {
    case '0': 
      if (opt) opt->numvals = 0;
      break;
    case '1':
      if (opt) opt->numvals = 1;
      break;
    default:
      msg = "expected 0 or 1 as last field";
      goto error;
  }
  p++;				// Skip the digit 0/1
  if (p > record_end) goto error;
  p = skip_whitespace(p);
  if (*p) {
    // Not at end of string, which means there must be a delimiter
    if (delimiterp(*p)) return p + 1;
    msg = "expected delimiter";
    goto error;
  }
  return p;

 error:
  len = record_end - record_start;
  fprintf(stderr, "%s: %s in the config segment '%.*s'\n", __FILE__, msg, (int) len, record_start);
  return NULL;

}

static int count_options(const char *str) {
  int count = 0;
  while ((str = parse_config(str, NULL))) count++;
  return count;
}

int optable_count(void) {
  return Tbl ? Tbl->count : 0;
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
    fprintf(stderr, "%s: invalid args to init\n", __FILE__);
    return 1;
  }

  Argc = argc;
  Argv = argv;

  count = count_options(config);
  if (count <= 0) return 0;

  if (Tbl) optable_free();

  Tbl = malloc(sizeof(optable_options) +
	       (count * sizeof(optable_option)));

  if (!Tbl) return 0;
  Tbl->count = count;
  
  for (int i = 0; i < count; i++) {
    config = parse_config(config, &opt);
    if (!config) {
      optable_free();
      return 1;
    }
  Tbl->options[i] = opt;
  }

  return 0;
}

// It's cleaner and safer to have a custom comparison function than to
// use libc functions like 'strlen' and 'strcmp'.
//
// Match ==> Returns a pointer within str to \0 or =
// No match ==> Returns NULL
//
static const char *compare_option(const char *str, const char *name) {
  if (!str || !name || !*name) return NULL;
  while (*name) {
    // Unnecessary check for end of str is elided below
    if (*str != *name) return NULL;
    str++; name++;
  }
  if ((*str == '\0') || (*str == '=')) return str;
  return NULL;
}

// Convenience getters

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

// When return value is non-negative, we found a match.  If '*value'
// is non-null, it points to the value part, e.g. "5" in "-r=5"
static int match_option(const char *arg,
			const char *get_name(int n),
			const char **value) {
  int n;
  int count = Tbl->count;
  for (n = 0; n < count; n++) {
    *value = compare_option(arg, get_name(n));
    if (*value) {
      if (**value == '=') (*value)++;
      else *value = NULL;
      return n;
    }
  }
  return -1;
}

int optable_is_option(const char *arg) {
  return (arg && (*arg == '-'));
}

// Return values:
//  -1    -> 'arg' does not match any option/switch
//   0..N -> 'arg' matches this option number
//
// Caller MUST check 'is_option()' before using 'parse_option()'
// because we assume here that 'arg' begins with '-'.
//
int optable_parse_option(const char *arg, const char **value) {
  if (!Tbl || !arg || !value) {
    fprintf(stderr, "%s:%d: invalid args to parse\n", __FILE__, __LINE__);
    return -1;
  }
  int n = match_option(++arg, optable_shortname, value);
  if (n < 0)
    n = match_option(++arg, optable_longname, value);
  if (n < 0)
    return -1;
  return n;
}

/*

  Start by passing in 0 for i.  The return value is the iterator
  state: pass it back in for i, until the return value is zero.

  XXXXXXXX RE-DO THIS!!  XXXXXXXX RE-DO THIS!!  XXXXXXXX RE-DO THIS!!  XXXXXXXX RE-DO THIS!!
  Example:
      const char *val;
      int n, i = 0;
      printf("Arg  Option  Value\n");
      while ((i = iter_argv(argc, argv, &n, &val, i))) {
	printf("[%2d] %3d     %-20s\n", i, n, val); 
      }

  The returned 'n' will be:
    -1   -> if 'value' is non-null, the arg is ordinary, not an option/switch
            if 'value' is null, argv[i] is an invalid option/switch
    0..N -> 'value' is for option n, given as either "-w=7" or "-w 7"


*/   
int optable_iter(int *n, const char **value, int i) {
  i++;
  if (!Tbl || !n || !value) {
    fprintf(stderr, "%s: invalid args to iterator\n", __FILE__);
    return -1;
  }
  if ((i < 1) || (i >= Argc)) return 0;
  if (optable_is_option(Argv[i])) {
    *n = optable_parse_option(Argv[i], value);
    if (*n < 0)
      return i;
    if (!*value &&
	Tbl->options[*n].numvals &&
	!optable_is_option(Argv[i+1]))
      *value = Argv[++i];
    return i;
  }
  // Else we have a value that is not the value of an option
  *n = -1;
  *value = Argv[i];
  return i;
}


