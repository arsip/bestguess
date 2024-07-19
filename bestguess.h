//  -*- Mode: C; -*-                                                       
// 
//  bestguess.h
// 
//  COPYRIGHT (C) Jamie A. Jennings, 2024

#ifndef bestguess_h
#define bestguess_h

#include "utils.h"

// Change to non-zero to enable debugging output to stdout
#define DEBUG 0

// Maximum number of commands we allow to benchmark
#define MAXCMDS 200

// Maximum number of arguments in one command
// E.g. "ls -l -h *.c" is a command with 3 arguments,
// and "ls -lh *.c" has 2 arguments.
#define MAXARGS 250

// Maximum length of a single command, in bytes
// E.g. "ls -lh" has 7 bytes (6 chars and NUL)
#define MAXCMDLEN 4096

// Maximum length of a single line in our own CSV file format
#define MAXCSVLEN 4096

// Change as desired
#define PROCESS_DATA_COMMAND "reduce"

#define bail(msg) do {				\
    fprintf(stderr, "%s\n", msg);		\
    fflush(NULL);				\
    exit(-1);					\
  } while (0)

// qsort arg order differs between linux and macos
#ifdef __linux__
  #define sort(base, n, sz, context, compare) qsort_r((base), (n), (sz), (compare), (context))
#else
  #define sort qsort_r
#endif

#endif
