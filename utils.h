//  -*- Mode: C; -*-                                                       
// 
//  utils.h
// 
//  COPYRIGHT (c) Jamie A. Jennings, 2024

#ifndef utils_h
#define utils_h

#include <stdio.h>
#include <stdlib.h>
#include "logging.h"

#define bail(msg) do {				\
    confess(progname, msg);			\
    exit(-1);					\
  } while (0)


char  *unescape(const char *str);
char  *escape(const char *str);

char **split(const char *in);
char **split_unescape(const char *in);

// For debugging: 
void print_args(char **args);


#endif
