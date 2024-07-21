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

// This list is the order in which fields will print in the CSV output
#define XFields(X)					    \
  X(F_CMD,      "Command",                      "\"%s\"")   \
  X(F_EXIT,     "Exit code",                    "%d")	    \
  X(F_SHELL,    "Shell",                        "\"%s\"")   \
  X(F_USER,     "User time (us)",               "%" PRId64) \
  X(F_SYSTEM,   "System time (us)",             "%" PRId64) \
  X(F_RSS,      "Max RSS (Bytes)",              "%ld")	    \
  X(F_RECLAIMS, "Page Reclaims",                "%ld")	    \
  X(F_FAULTS,   "Page Faults",                  "%ld")	    \
  X(F_VCSW,     "Voluntary Context Switches",   "%ld")	    \
  X(F_ICSW,     "Involuntary Context Switches", "%ld")	

// Below are pseudo field names used only for computing summary
// statistics.  They are never written to the raw data output file.
#define F_TOTAL -1
#define F_TCSW -2

#define FIRST(a, b, c) a,
static enum FieldCodes {XFields(FIRST) F_LAST};
#define SECOND(a, b, c) b,
static const char *Headers[] = {XFields(SECOND) NULL};
#define THIRD(a, b, c) c,
static const char *FieldFormats[] = {XFields(THIRD) NULL};

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
