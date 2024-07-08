//  -*- Mode: C; -*-                                                       
// 
//  args.c
// 
//  COPYRIGHT (C) Jamie A. Jennings, 2024

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "args.h"

#define MAX_ARGS 1000

/*

  SECURITY NOTE: The processing of command-line args, below, uses
  'strlen' and the like only when the argument is a constant literal
  string.  In such cases, these functions are safe.  Constructing
  elements of OptionList at runtime would violate this safety.

*/

static const char *skip_whitespace(const char *str) {
  if (!str) return NULL;
  while ((*str != '\0') &&
	 ((*str == ' ') || (*str == '\t') ||
	  (*str == '\n') || (*str == '\r')))
    str++;
  return str;
}

// If return value is non-negative, *value points to \0 or =
static int match_short_option(const char *arg, const char **value) {
  const char c = *(++arg);
  arg++;
  if (c && ((*arg == '\0') || (*arg == '='))) {
    int n = 0;
    while (LongOptions[n]) {
      if (c == ShortOptions[n]) {
	*value = arg;
	return n;
      }
      n++;
    }
  }
  return -1;
}

// Cleaner and safer to have a custom char-by-char comparison function
// than to use system calls like 'strlen' and 'strcmp'.
//
// Match ==> Return pointer within str to \0 or =
// No match ==> return NULL
//
static const char *compare_long_option(const char *str, const char *option) {
  if (!str || !option) return NULL;
  while (*option) {
    // Elided unnecessary check for end of str below
    if (*str != *option) return NULL;
    str++; option++;
  }
  if ((*str == '\0') || (*str == '=')) return str;
  return NULL;
}

// If return value is non-negative, *value points to \0 or =
static int match_long_option(const char *arg, const char **value) {
  arg++;
  if (*(arg++) != '-') return -1;
  int n = 0;
  while (LongOptions[n]) {
    *value = compare_long_option(arg, LongOptions[n]);
    if (*value) return n;
    n++;
  }
  return -1;
}

int is_option(const char *arg) {
  return (arg && (*arg == '-'));
}

// Return values:
//  -1    -> 'arg' does not match any option/switch
//   0..N -> 'arg' matches this option number
//
// Caller must check 'is_option()' before using 'parse_option()'
//
int parse_option(const char *arg, const char **value) {
  if (!arg || !value) bail("error calling parse_option");
  arg = skip_whitespace(arg);
  int n = match_short_option(arg, value);
  if (n < 0)
    n = match_long_option(arg, value);
  if (n < 0)
    return -1;
  if (!*value)
    bail("error in option name matching");
  if (**value == '=')
    (*value)++;
  else
    *value = NULL;
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
    -1   -> 'value' (non-null) is ordinary, not an option/switch
            if 'value' is null, argv[i] is an invalid option/switch
    0..N -> 'value' is for option n
            it may have been given as "-w=7" or "-w 7"
	    or the default option if "-w" is followed by another 
	    option/switch

*/   
int iter_argv(int argc, char *argv[],
	      int *n, const char **value,
	      int i) {
  i++;
  if ((argc < 1) || !argv || !n || !value)
    bail("invalid args passed to iter_argv");
  if ((i < 1) || (i >= argc)) return 0;
  if (is_option(argv[i])) {
    *n = parse_option(argv[i], value);
    if (*n < 0) {
      *value = NULL;
      return i;
    }
    if (!*value && OptionNumVals[*n]) {
      if (is_option(argv[i+1]))
	*value = OptionDefaults[*n];
      else
	*value = argv[++i];
    }
    return i;
  } 
  // Else we have a value that is not the value of an option
  *n = -1;
  *value = argv[i];
  return i;
}

char **split(const char *in) {
  char **args = malloc(MAX_ARGS * sizeof(char *));
  if (!args) goto oom;

  int n = 0;
  const char *start = in;
  const char *p;

  p = start;

  while (*p != '\0') {

    // Skip whitespace
    while ((*p != '\0') && ((*p == ' ') || (*p == '\t'))) p++;
    if (*p == '\0') break;

    // Read command or argument
    start = p;
    while ((*p != '\0') && (*p != ' ') && (*p != '\t')) p++;

    // Store it
    args[n] = malloc(p - start + 1);
    if (!args[n]) goto oom;
    memcpy(args[n], start, p - start);
    args[n][p-start] = '\0';
    // printf("[%d] %s\n", n, args[n]);
    n++;
  }

  args[n] = NULL;
  return args;
  
 oom:
  printf("Out of memory\n");
  exit(-2);
}

