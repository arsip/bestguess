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
  double  ADscore;	// ** See below **
  double  p_normal;	// p-value of ADscore
  double  skew;		// Non-parametric skew
  double  kurtosis;	// Distribution shape compared to normal
  uint8_t code;		// Info about ADscore and skew
} Measures;

// Inferential statistics comparing a sample to a reference sample
// (like the fastest performer)
typedef struct Inference {
  uint8_t indistinct;	// See below
  double  W;		// Mann-Whitney-Wilcoxon rank sum
  double  p;		// p value that η₁ ≠ η₂ (conservative)
  double  p_adj;	// p value adjusted for ties
  double  shift;	// Hodges-Lehmann estimation of median shift (μs)
  double  confidence;	// Confidence for shift, e.g. 0.955 for 95.5%
  int64_t ci_low;	// Confidence interval for shift (μs)
  int64_t ci_high;	// (μs)
  double  p_super;	// Probability of superiority
} Inference;

// Flags for 'indistinct' in Inference (results of evaluating
// inferential statistics).  When 'indistinct' is zero, we have a
// significant, sizable difference between two samples.
enum InferenceFlags {
  INF_NONSIG,		// p value not significant
  INF_CIZERO,		// CI includes zero
  INF_NOEFFECT,		// Effect size (diff) too small
  INF_HIGHSUPER,	// Prob of superiority too high
};

// ** IMPORTANT **
//
// The 'ADscore' field in Measures holds the Anderson-Darling
// normality (distance) ONLY if these flags are all FALSE:
// CODE_HIGHZ, CODE_SMALLN, CODE_LOWVARIANCE
//
// When the flag CODE_HIGHZ is true, 'ADscore' holds the highest
// Z-score we found.

// Statistical summary of a set of runs of a single command
typedef struct Summary {
  char      *cmd;		// Never NULL (can be epsilon)
  char      *shell;		// Never NULL (can be epsilon)
  int        runs;
  int        fail_count;
  Measures   user;
  Measures   system;
  Measures   total;
  Measures   maxrss;
  Measures   vcsw;
  Measures   icsw;
  Measures   tcsw;
  Measures   wall;
  Inference *infer;		// Can be NULL
} Summary;

// Bitmasks
#define HAS(byte, flagname) (((uint8_t)1)<<(flagname) & byte)
#define SET(byte, flagname) do {		\
    byte = (byte) | (((uint8_t)1)<<(flagname));	\
  } while (0)

// Flags for 'code' in Measures
enum MeasureCodes {
  CODE_HIGHZ,		  // Very high Z scores ==> No AD score 
  CODE_SMALLN,		  // Insufficient observations ==> No AD score
  CODE_LOWVARIANCE,	  // Very low variance ==> No AD score
  CODE_HIGH_SKEW,	  // High skew can explain non-normality
  CODE_HIGH_KURTOSIS,	  // High kurtosis can explain non-normality
};

// IMPORTANT: A RankedCombinedSample may contain n1+n2 elements (size
// of 'X' and 'ranks') or n1*n2 elements.
typedef struct RankedCombinedSample {
  int      n1;		// Size of sample 1
  int      n2;		// Size of sample 2
  int64_t *X;		// Ranked (sorted) differences
  double  *rank;	// Assigned ranks for X[]
} RankedCombinedSample;

// -----------------------------------------------------------------------------

// The minimum sample size needed for our inferential statistics is
// somewhere around n₁ = n₂ = 3.  Complete separation between the
// samples would be needed in order to conclude η₁ ≠ η₂ with samples
// that small.  If we require each sample to have at least 5
// observations, then complete separation should produce a p value
// around 0.016, so we could conceivably see that p value empirically.
//
// (/ 2.0 (choose 10 5)) ==>  0.00793650 ≈ 0.008

#define INFERENCE_N_THRESHOLD 5

Inference *compare_samples(Usage *usage,
			   double alpha,
			   int ref_start, int ref_end,
			   int idx_start, int idx_end);

int *sort_by_totaltime(Summary *summaries[], int start, int end);

Summary *summarize(Usage *usage, int *next);
void     free_summary(Summary *s);


#endif
