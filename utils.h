//  -*- Mode: C; -*-                                                       
// 
//  utils.h
// 
//  COPYRIGHT (c) Jamie A. Jennings, 2024

#ifndef utils_h
#define utils_h

#include "bestguess.h"
#include <stdio.h>

typedef struct arglist {
  size_t max;
  size_t next;
  char **args;
} arglist;

arglist *new_arglist(size_t limit);
int      add_arg(arglist *args, char *newarg);
void     free_arglist(arglist *args);

char  *unescape(const char *str);
char  *escape(const char *str);

int split(const char *in, arglist *args);
int split_unescape(const char *in, arglist *args);
int ends_in(const char *str, const char *suffix);

// Misc:
FILE *maybe_open(const char *filename, const char *mode);
void  warning(const char *fmt, ...);
void  bail(const char *msg);

// For debugging: 
void print_arglist(arglist *args);


#endif
