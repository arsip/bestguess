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
typedef struct Measures {
  int64_t min;
  int64_t max;
  int64_t mode;
  int64_t median;
  int64_t pct95;
  int64_t pct99;
} Measures;

// Statistical summary of a set of runs of a single command
typedef struct Summary {
  char     *cmd;
  char     *shell;
  int       runs;
  int       fail_count;
  Measures  user;
  Measures  system;
  Measures  total;
  Measures  maxrss;
  Measures  vcsw;
  Measures  icsw;
  Measures  tcsw;
  Measures  wall;
} Summary;

Summary *summarize(Usage *usage, int *next);
void     free_summary(Summary *s);

double zscore(double z);
int    zzscore(int scaled_z);
void   print_zscore_table(void);

#endif
