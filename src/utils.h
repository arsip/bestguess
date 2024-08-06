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

// How many microseconds in one second
#define MICROSECS 1000000

// 1024 * 1024 = How many things are in a mega-thing
#define MEGA 1048576

// -----------------------------------------------------------------------------
// Custom usage struct with accessors and comparators
// -----------------------------------------------------------------------------

typedef struct usage {
  struct rusage os;		// direct from operating system
  int64_t       wall;		// measured wall clock time
} usage;

int64_t maxrss(usage *usage);
int64_t usertime(usage *usage);
int64_t systemtime(usage *usage);
int64_t totaltime(usage *usage);
int64_t vcsw(usage *usage);
int64_t icsw(usage *usage);
int64_t tcsw(usage *usage);
int64_t minflt(usage *usage);
int64_t majflt(usage *usage);
int64_t wall(usage *usage);

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
COMPARATOR(maxrss);
COMPARATOR(vcsw);
COMPARATOR(icsw);
COMPARATOR(tcsw);
COMPARATOR(wall);

// -----------------------------------------------------------------------------
// Argument lists for calling exec
// -----------------------------------------------------------------------------

typedef struct arglist {
  size_t max;
  size_t next;
  char **args;
} arglist;

arglist *new_arglist(size_t limit);
int      add_arg(arglist *args, char *newarg);
void     free_arglist(arglist *args);
void     print_arglist(arglist *args);

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


#endif
