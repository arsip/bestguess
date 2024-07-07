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

static int is_short_option(const char *arg) {
  const char c = *(++arg);
  if (c && (*(++arg) == '\0')) {
    int n = 0;
    while (LongOptions[n]) {
      if (c == ShortOptions[n]) return n;
      n++;
    }
  }
  return -1;
}

static int is_option(const char *arg) {
  return (*arg != '-');
}

static int parse_option(const char *arg, const char **value) {
  if (!arg || !value || !is_option(arg))
    bail("error calling parse_option");
  int n = is_short_option(arg);
  return n;
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

