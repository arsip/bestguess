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

// Bitmasks
#define HAS_NONE(byte) (byte == 0)
#define HAS(byte, flagname) (((uint8_t)1)<<(flagname) & byte)
#define SET(byte, flagname) do {		\
    byte = (byte) | (((uint8_t)1)<<(flagname));	\
  } while (0)

// Flags for 'code' in Measures
enum MeasureCodes {
  CODE_HIGHZ,
  CODE_SMALLN,
  CODE_LOWVARIANCE,
};

// IMPORTANT: A RankedCombinedSample may contain n1+n2 elements (size
// of 'X' and 'ranks') or n1*n2 elements.
typedef struct RankedCombinedSample {
  int      n1;		// size of sample 1
  int      n2;		// size of sample 2
  int64_t *X;		// ranked (sorted) differences
  int     *index;	// sign of difference
  double  *rank;	// assigned ranks for X[]
} RankedCombinedSample;

// Inferential statistics comparing a sample to a reference sample
// (like the fastest performer)
typedef struct Inference {
  int     index;	// 0-based index into Summary array
  uint8_t results;	// See below
  double  W;		// Mann-Whitney-Wilcoxon rank sum
  double  p;		// p value that η₁ ≠ η₂ (conservative)
  double  p_adj;	// p value adjusted for ties
  double  shift;	// Hodges-Lehmann estimation of median shift
  double  confidence;	// Confidence for shift, e.g. 0.955 for 95.5%
  int64_t ci_low;	// Confidence interval for shift
  int64_t ci_high;
  double  p_super;	// Probability of superiority
} Inference;

// Flags for 'results' in Inference (results of evaluating inferential
// statistics).  When 'results' is zero, we have a significant,
// sizable difference between two samples.
enum InferenceFlags {
  INF_NONSIG,		// p value not significant
  INF_CIZERO,		// CI includes zero
  INF_NOEFFECT,		// Effect size (diff) too small
  INF_HIGHSUPER,	// Prob of superiority too high
};

Inference *compare_samples(Usage *usage,
			   int idx,
			   double alpha,
			   int ref_start, int ref_end,
			   int idx_start, int idx_end);

double ranked_diff_Ahat(RankedCombinedSample RCS);
double mann_whitney_w(RankedCombinedSample RCS);
double mann_whitney_u(RankedCombinedSample RCS, double *U1, double *U2);
double mann_whitney_p(RankedCombinedSample RCS, double W, double *adjustedp);
int64_t mann_whitney_U_ci(RankedCombinedSample RCS, double alpha, int64_t *high);
double median_diff_ci(RankedCombinedSample RCS,
		      double alpha,
		      int64_t *lowptr,
		      int64_t *highptr);
double median_diff_estimate(RankedCombinedSample RCS);

RankedCombinedSample rank_difference_magnitude(Usage *usage,
					       int start1, int end1,
					       int start2, int end2,
					       FieldCode fc);
RankedCombinedSample rank_difference_signed(Usage *usage,
					    int start1, int end1,
					    int start2, int end2,
					    FieldCode fc);
RankedCombinedSample rank_combined_samples(Usage *usage,
					   int start1, int end1,
					   int start2, int end2,
					   FieldCode fc);




int *sort_by_totaltime(Summary *summaries[], int start, int end);

Summary *summarize(Usage *usage, int *next);
void     free_summary(Summary *s);

double normalCDF(double z);
double inverseCDF(double alpha);
void   print_zscore_table(void);


#endif
