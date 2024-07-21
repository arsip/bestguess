//  -*- Mode: C; -*-                                                       
// 
//  common.h
// 
//  Copyright (C) Jamie A. Jennings, 2024

#ifndef common_h
#define common_h

#include <sys/time.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <inttypes.h>
#include "bestguess.h"

void bail(const char *msg);

typedef struct Summary {
  char   *cmd;
  int     runs;
  int64_t user;			// median
  int64_t umin;
  int64_t umax;
  int64_t system;		// median
  int64_t smin;
  int64_t smax;
  int64_t total;		// median
  int64_t tmin;
  int64_t tmax;
  int64_t rss;			// median
  int64_t rssmin;
  int64_t rssmax;
  int64_t csw;			// median
  int64_t cswmin;
  int64_t cswmax;
} Summary;

Summary *new_Summary(void);
void     free_Summary(Summary *s);

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

#define COMPARATOR(accessor) comparator compare_##accessor;

COMPARATOR(usertime)
COMPARATOR(systemtime)
COMPARATOR(totaltime)
COMPARATOR(rss)
COMPARATOR(tcsw)


#endif
