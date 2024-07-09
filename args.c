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

char **split(const char *in) {
  char **args = malloc(MAX_ARGS * sizeof(char *));
  if (!args) goto oom;

  int n = 0;
  const char *start = in;
  const char *p;

  p = start;

  while (*p != '\0') {

    p = skip_whitespace(p);
    if (*p == '\0') break;

    // Read command or argument
    start = p;
    p = until_whitespace(p);

    // Store it
    args[n] = malloc(p - start + 1);
    if (!args[n]) goto oom;
    memcpy(args[n], start, p - start);
    args[n][p-start] = '\0';
    n++;
  }

  args[n] = NULL;
  return args;
  
 oom:
    warn("args", "Out of memory");
    return NULL;
}

