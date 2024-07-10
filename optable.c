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

static const char *until_comma(const char *str) {
  if (!str) return NULL;
  while ((*str != '\0') && (*str != ',')) str++;
  return str;
}

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
  const char *start = p;
  const char *end = until_comma(p);
  const char *field;
  field = skip_whitespace(p);
  p = until_whitespace(field);
  if (p > end) goto missing_field;
  if (opt) opt->shortname = maybe_dup(field, p);
  field = skip_whitespace(p);
  p = until_whitespace(field);
  if (p > end) goto missing_field;
  if (opt) opt->longname = maybe_dup(field, p);
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
  if (p > end) goto missing_field;
  p = skip_whitespace(p);
  if (*p) {
    // Not at end of string, which means there must be a comma
    if (*p == ',') return p + 1;
    fprintf(stderr, "%s: expected ',' in config at '%.40s'\n", __FILE__, p);
    return NULL;
  }
  return p;

 missing_field:
  len = end - start;
  fprintf(stderr, "%s: missing field in '%.*s'\n", __FILE__, (int) len, start);
  return NULL;

}

static int count_options(const char *str) {
  int count = 0;
  while ((str = parse_config(str, NULL))) count++;
  return count;
}

void optable_free(optable_options *opts) {
  if (!opts) return;
  for (int i = 0; i < opts->count; i++) {
    free(opts->options[i].shortname);
    free(opts->options[i].longname);
  }
  free(opts);
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
      optable_free(tbl);
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

// Convenience getters

const char *optable_shortname(optable_options *tbl, int n) {
  if (tbl && (n >= 0) && (n < tbl->count))
    return tbl->options[n].shortname;
  return NULL;
}

const char *optable_longname(optable_options *tbl, int n) {
  if (tbl && (n >= 0) && (n < tbl->count))
    return tbl->options[n].longname;
  return NULL;
}

int optable_numvals(optable_options *tbl, int n) {
  if (tbl && (n >= 0) && (n < tbl->count))
    return tbl->options[n].numvals;
  return -1;
}

typedef const char *(name_getter)(optable_options *tbl, int n);

// When return value is non-negative, we found a match.  If '*value'
// is non-null, it points to the value part, e.g. "5" in "-r=5"
static int match_option(const char *arg,
			name_getter get_name,
			optable_options *tbl,
			const char **value) {
  int n;
  int count = tbl->count;
  for (n = 0; n < count; n++) {
    *value = compare_option(arg, get_name(tbl, n));
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
int optable_parse_option(optable_options *tbl,
			 const char *arg,
			 const char **value) {
  if (!tbl || !arg || !value) {
    fprintf(stderr, "%s:%d: invalid args to parse\n", __FILE__, __LINE__);
    return -1;
  }
  int n = match_option(++arg, optable_shortname, tbl, value);
  if (n < 0)
    n = match_option(++arg, optable_longname, tbl, value);
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
int optable_iter(optable_options *tbl,
		 int argc, char *argv[],
		 int *n, const char **value,
		 int i) {
  i++;
  if (!tbl || (argc < 1) || !argv || !n || !value) {
    fprintf(stderr, "%s:%d: invalid args to iterator\n", __FILE__, __LINE__);
    return -1;
  }
  if ((i < 1) || (i >= argc)) return 0;
  if (optable_is_option(argv[i])) {
    *n = optable_parse_option(tbl, argv[i], value);
    if (*n < 0)
      return i;
    if (!*value && tbl->options[*n].numvals && !optable_is_option(argv[i+1]))
      *value = argv[++i];
    return i;
  }
  // Else we have a value that is not the value of an option
  *n = -1;
  *value = argv[i];
  return i;
}


