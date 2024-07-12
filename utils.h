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

__attribute__((unused))
static const char *progname = "null";

#endif
