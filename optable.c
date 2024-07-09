//  -*- Mode: C; -*-                                                       
// 
//  optable.c  A minimal CLI arg parsing system
// 
//  COPYRIGHT (C) Jamie A. Jennings, 2024

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "optable.h"

static int whitespacep(const char c) {
  return ((c == ' ') || (c == '\t') ||
	  (c == '\n') || (c == '\r'));
}

static const char *skip_whitespace(const char *str) {
  if (!str) return NULL;
  while ((*str != '\0') && whitespacep(*str)) str++;
  return str;
}

static const char *until_whitespace(const char *str) {
  if (!str) return NULL;
  while ((*str != '\0') && !whitespacep(*str)) str++;
  return str;
}

static char *dup(const char *str, size_t len) {
  char *name = calloc(1, len + 1);
  if (!name) return NULL;
  memcpy(name, str, len);
  return name;
}

// "w warmup 1, r runs 1, show-output 1, v version 0"

// A field that is a single dash, '-', means "no name"
static char *maybe_dup(const char *start, const char *end) {
  if ((*start == '-') && (end == start + 1))
    return NULL;
  else
    return dup(start, end - start);
}

static const char *parse_config(const char *p, optable_option *opt) {
  if (!p) {
    fprintf(stderr, "%s: invalid args to parse_config\n", __FILE__);
    return NULL;
  }
  if (!*p) return NULL;
  const char *start;
  start = skip_whitespace(p);
  p = until_whitespace(start);
  if (opt) opt->shortname = maybe_dup(start, p);
  start = skip_whitespace(p);
  p = until_whitespace(start);
  if (opt) opt->longname = maybe_dup(start, p);
  p = skip_whitespace(p);
  switch (*p) {
    case '0': 
      if (opt) opt->numvals = 0;
      break;
    case '1':
      if (opt) opt->numvals = 1;
      break;
    default:
      fprintf(stderr, "%s: expected 0/1 in config at '%.40s'\n", __FILE__, p);
      return NULL;
  }
  p++;				// Skip the digit 0/1
  p = skip_whitespace(p);
  if (*p) {
    // Not at end of string, which means there must be a comma
    if (*p == ',') return p + 1;
    fprintf(stderr, "%s: expected ',' in config at '%.40s'\n", __FILE__, p);
    return NULL;
  }
  return p;
}

static int count_options(const char *str) {
  int count = 0;
  while ((str = parse_config(str, NULL))) count++;
  return count;
}

optable_options *optable_init(const char *config) {
  int count;
  optable_option opt;
  optable_options *tbl;

  count = count_options(config);
  if (count <= 0) return NULL;

  tbl = malloc(sizeof(optable_options) +
	       (count * sizeof(optable_option)));

  if (!tbl) return NULL;
  tbl->count = count;
  
  for (int i = 0; i < count; i++) {
    config = parse_config(config, &opt);
    if (!config) {
      free(tbl);
      return NULL;
    }
  tbl->options[i] = opt;
  }

  return tbl;
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

// When return value is non-negative, we found a match.  If '*value'
// is non-null, it points to the value part, e.g. "5" in "-r=5"
static int match_option(const char *arg,
			const char *const names[],
			const char **value) {
  int n = 0;
  while (names[n]) {
    *value = compare_option(arg, names[n]);
    if (*value) {
      if (**value == '=') (*value)++;
      else *value = NULL;
      return n;
    }
    n++;
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
// because we assume here that 'arg' begins with '-'
//
int optable_parse_option(const char *arg, const char **value) {
  if (!arg || !value) {
    fprintf(stderr, "%s:%d: invalid args to parse\n", __FILE__, __LINE__);
    return -1;
  }
  int n = match_option(++arg, ShortOptions, value);
  if (n < 0)
    n = match_option(++arg, LongOptions, value);
  if (n < 0)
    return -1;
  return n;
}

/*

  Start by passing in 0 for i.  The return value is the iterator
  state: pass it back in for i, until the return value is zero.

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
int optable_iter(int argc, char *argv[],
		 int *n, const char **value,
		 int i) {
  i++;
  if ((argc < 1) || !argv || !n || !value) {
    fprintf(stderr, "%s: invalid args to iterator\n", __FILE__);
    return -1;
  }
  if ((i < 1) || (i >= argc)) return 0;
  if (optable_is_option(argv[i])) {
    *n = optable_parse_option(argv[i], value);
    if (*n < 0)
      return i;
    if (!*value && OptionNumVals[*n] && !optable_is_option(argv[i+1]))
      *value = argv[++i];
    return i;
  }
  // Else we have a value that is not the value of an option
  *n = -1;
  *value = argv[i];
  return i;
}


