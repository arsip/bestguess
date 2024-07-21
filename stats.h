//  -*- Mode: C; -*-                                                       
// 
//  stats.h  Summary statistics
// 
//  COPYRIGHT (c) Jamie A. Jennings, 2024

#ifndef stats_h
#define stats_h

#include "bestguess.h"

// Min, max, and measures of central tendency
typedef struct measures {
  int64_t min;
  int64_t max;
  int64_t mode;
  int64_t median;
} measures;

// Statistical summary of a set of runs of a single command
typedef struct summary {
  char     *cmd;
  int       runs;
  measures  user;
  measures  system;
  measures  total;
  measures  rss;
  measures  tcsw;
} summary;

summary *summarize(char *cmd, struct rusage *usagedata);
void     free_summary(summary *s);



#endif
