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

// -----------------------------------------------------------------------------
// Field accessors (rusage) and comparators 
// -----------------------------------------------------------------------------

#define DECLARE_RACCESSOR(name) \
  int64_t name(struct rusage *usage)

DECLARE_RACCESSOR(Rrss);
DECLARE_RACCESSOR(Ruser);
DECLARE_RACCESSOR(Rsys);
DECLARE_RACCESSOR(Rtotal);
DECLARE_RACCESSOR(Rvcsw);
DECLARE_RACCESSOR(Ricsw);
DECLARE_RACCESSOR(Rtcsw);
DECLARE_RACCESSOR(Rpgrec);
DECLARE_RACCESSOR(Rpgflt);

// The arg order for comparators passed to qsort_r differs between
// linux and macos.
#ifdef __linux__
typedef int (comparator)(const void *, const void *, void *);
#else
typedef int (comparator)(void *, const void *, const void *);
#endif

#define DECLARE_COMPARATOR(accessor) comparator compare_##accessor

DECLARE_COMPARATOR(Ruser);
DECLARE_COMPARATOR(Rsys);
DECLARE_COMPARATOR(Rtotal);
DECLARE_COMPARATOR(Rrss);
DECLARE_COMPARATOR(Rtcsw);

#endif
