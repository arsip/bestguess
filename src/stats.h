//  -*- Mode: C; -*-                                                       
// 
//  stats.h  Summary statistics
// 
//  COPYRIGHT (c) Jamie A. Jennings, 2024

#ifndef stats_h
#define stats_h

#include "bestguess.h"
#include "utils.h"
#include <sys/resource.h>

// Min, max, and measures of central tendency
typedef struct measures {
  int64_t min;
  int64_t max;
  int64_t mode;
  int64_t median;
  int64_t pct95;
  int64_t pct99;
} measures;

// Statistical summary of a set of runs of a single command
typedef struct summary {
  char     *cmd;
  char     *shell;
  int       runs;
  int       fail_count;
  measures  user;
  measures  system;
  measures  total;
  measures  maxrss;
  measures  vcsw;
  measures  icsw;
  measures  tcsw;
  measures  wall;
} summary;

summary *summarize(char *cmd,
		   int fail_count,
		   Usage *usagedata);

void     free_summary(summary *s);

double zscore(double z);
int    zzscore(int scaled_z);
void   print_zscore_table(void);

#endif
