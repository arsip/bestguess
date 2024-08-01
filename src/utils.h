//  -*- Mode: C; -*-                                                       
// 
//  utils.h
// 
//  COPYRIGHT (c) Jamie A. Jennings, 2024

#ifndef utils_h
#define utils_h

#include "bestguess.h"
#include <stdio.h>
#include <sys/resource.h>

typedef struct arglist {
  size_t max;
  size_t next;
  char **args;
} arglist;

arglist *new_arglist(size_t limit);
int      add_arg(arglist *args, char *newarg);
void     free_arglist(arglist *args);
// For debugging: 
void print_arglist(arglist *args);

// -----------------------------------------------------------------------------
// String operations
// -----------------------------------------------------------------------------

char  *unescape(const char *str);
char  *escape(const char *str);

int split(const char *in, arglist *args);
int split_unescape(const char *in, arglist *args);
int ends_in(const char *str, const char *suffix);

// Misc:
FILE *maybe_open(const char *filename, const char *mode);
void  warning(const char *fmt, ...);
void  bail(const char *msg);

// -----------------------------------------------------------------------------
// Field accessors (rusage) and comparators 
// -----------------------------------------------------------------------------

int64_t rss(struct rusage *usage);
int64_t usertime(struct rusage *usage);
int64_t systemtime(struct rusage *usage);
int64_t totaltime(struct rusage *usage);
int64_t vcsw(struct rusage *usage);
int64_t icsw(struct rusage *usage);
int64_t tcsw(struct rusage *usage);

// The arg order for comparators passed to qsort_r differs between
// linux and macos.
#ifdef __linux__
typedef int (comparator)(const void *, const void *, void *);
#else
typedef int (comparator)(void *, const void *, const void *);
#endif

#define COMPARATOR(accessor) comparator compare_##accessor

COMPARATOR(usertime);
COMPARATOR(systemtime);
COMPARATOR(totaltime);
COMPARATOR(rss);
COMPARATOR(vcsw);
COMPARATOR(icsw);
COMPARATOR(tcsw);


#endif
