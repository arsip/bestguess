//  -*- Mode: C; -*-                                                       
// 
//  stats.h
// 
//  COPYRIGHT (c) Jamie A. Jennings, 2024

#ifndef stats_h
#define stats_h

#include "bestguess.h"

typedef struct summary {
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
} summary;

int64_t find_median(struct rusage *usagedata, int field);
summary *summarize(char *cmd, struct rusage *usagedata);
void     free_summary(summary *s);



#endif
