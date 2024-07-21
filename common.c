//  -*- Mode: C; -*-                                                       
// 
//  common.c
// 
//  Copyright (C) Jamie A. Jennings, 2024

#include <stdio.h>
#include <stdlib.h>
#include "common.h"

__attribute__((noreturn))
void bail(const char *msg) {
  fputs(msg, stderr);
  fflush(NULL);
  exit(-1);
}

Summary *new_Summary(void) {
  Summary *s = calloc(1, sizeof(Summary));
  if (!s) bail("Out of memory");
  return s;
}

void free_Summary(Summary *s) {
  free(s->cmd);
  free(s);
}

int64_t rss(struct rusage *usage) {
  return usage->ru_maxrss;
}

int64_t usertime(struct rusage *usage) {
  return usage->ru_utime.tv_sec * 1000 * 1000 + usage->ru_utime.tv_usec;
}

int64_t systemtime(struct rusage *usage) {
  return usage->ru_stime.tv_sec * 1000 * 1000 + usage->ru_stime.tv_usec;
}

int64_t totaltime(struct rusage *usage) {
  return usertime(usage) + systemtime(usage);
}

int64_t vcsw(struct rusage *usage) {
  return usage->ru_nvcsw;
}

int64_t icsw(struct rusage *usage) {
  return usage->ru_nivcsw;
}

int64_t tcsw(struct rusage *usage) {
  return vcsw(usage) + icsw(usage);
}

// The arg order for comparators passed to qsort_r differs between
// linux and macos.
#ifdef __linux__
#define MAKE_COMPARATOR(accessor)					\
  static int compare_##accessor(const void *idx_ptr1,			\
				const void *idx_ptr2,			\
				void *context) {			\
    struct rusage *usagedata = context;					\
    const int idx1 = *((const int *)idx_ptr1);				\
    const int idx2 = *((const int *)idx_ptr2);				\
    if (accessor(&usagedata[idx1]) > accessor(&usagedata[idx2]))	\
      return 1;								\
    if (accessor(&usagedata[idx1]) < accessor(&usagedata[idx2]))	\
      return -1;							\
    return 0;								\
  }
#else
#define MAKE_COMPARATOR(accessor)					\
  int compare_##accessor(void *context,					\
			 const void *idx_ptr1,				\
			 const void *idx_ptr2) {			\
    struct rusage *usagedata = context;					\
    const int idx1 = *((const int *)idx_ptr1);				\
    const int idx2 = *((const int *)idx_ptr2);				\
    if (accessor(&usagedata[idx1]) > accessor(&usagedata[idx2]))	\
      return 1;								\
    if (accessor(&usagedata[idx1]) < accessor(&usagedata[idx2]))	\
      return -1;							\
    return 0;								\
  }
#endif

MAKE_COMPARATOR(usertime)
MAKE_COMPARATOR(systemtime)
MAKE_COMPARATOR(totaltime)
MAKE_COMPARATOR(rss)
MAKE_COMPARATOR(tcsw)
