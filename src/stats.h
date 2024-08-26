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
  int64_t Q1;
  int64_t Q3;
  double  est_mean;	// Estimated
  double  est_stddev;	// Estimated
  double  ADscore;	// Anderson-Darling normality (distance)
  double  p_normal;	// p-value of ADscore
  double  skew;		// Non-parametric skew
  uint8_t code;		// Info about ADscore and skew
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

// bitmasks
#define HAS_NONE(byte) (byte == 0)
#define HAS(byte, flagname) (((uint8_t)1)<<(flagname) & byte)
#define SET(byte, flagname) do {		\
    byte = (byte) | (((uint8_t)1)<<(flagname));	\
  } while (0)

// flags for 'code' in Measures 
#define CODE_HIGHZ 0
#define CODE_SMALLN 1
#define CODE_LOWVARIANCE 2

Summary *summarize(Usage *usage, int *next);
void     free_summary(Summary *s);

double normalCDF(double z);
void   print_zscore_table(void);


#endif
