//  -*- Mode: C; -*-                                                       
// 
//  stats.c  Summary statistics
// 
//  Copyright (C) Jamie A. Jennings, 2024

#include "bestguess.h"
#include "utils.h"
#include "stats.h"
#include <string.h>

/* 
   Note: For a log-normal distribution, the geometric mean is the
   median value and is a more useful measure of the "typical" value
   than is the arithmetic mean.  We may not have a log normal
   distribution, however.

   If we had to choose a single number to characterize a distribution
   of runtimes, the mode (or modes) may be the most useful.  It
   represents the most common value, after all!

   However, if we are most concerned with the long tail, then the 99th
   percentile value should be highlighted.

   Given a choice between either median or mean, the median of a
   right-skewed distribution is typically the closer of the two to the
   mode.

 */

#define MEDIAN_SELECT(accessor, indices, usagedata)			\
  ((config.runs == (config.runs/2) * 2) 				\
   ?									\
   ((accessor(&(usagedata)[indices[config.runs/2 - 1]]) +		\
     accessor(&(usagedata)[indices[config.runs/2]])) / 2)		\
   :									\
   (accessor(&usagedata[indices[config.runs/2]])))			\

static int64_t avg(int64_t a, int64_t b) {
  return (a + b) / 2;
}

#define VALUEAT(i) (accessor(&usagedata[indices[i]]))
#define WIDTH(i, j) (VALUEAT(j) - VALUEAT(i))

// "Half sample" technique for mode estimation.  The base cases are
// when number of samples, 𝑛, is 1, 2, or 3.  When 𝑛 > 3, there is a
// recursive case that computes the mode of ℎ = 𝑛/2 samples.  To
// choose which 𝑛/2 samples, we examine every interval [i, i+ℎ) to
// find the one with the smallest width.  That sequence of samples is
// the argument to the recursive call.
//
// See https://arxiv.org/abs/math/0505419
//
// Note that the data must be sorted, and for that we use 'indices'.
//
static int64_t estimate_mode(struct rusage *usagedata,
			     int64_t (accessor)(struct rusage *),
			     int *indices) {
  // n = number of samples being examined
  // h = size of "half sample" (floor of n/2)
  // idx = start index of samples being examined
  // limit = end index one beyond last sample being examined
  int idx, limit, n, h;
  int64_t wmin = 0;
  idx = 0;
  n = config.runs;
  
 tailcall:
  if (n == 1) {
    return VALUEAT(idx);
  } else if (n == 2) {
    return avg(VALUEAT(idx), VALUEAT(idx+1));
  } else if (n == 3) {
    // Break a tie in favor of lower value
    if (WIDTH(idx, idx+1) <= WIDTH(idx+1, idx+2))
      return avg(VALUEAT(idx), VALUEAT(idx+1));
    else
      return avg(VALUEAT(idx+1), VALUEAT(idx+2));
  }
  h = n / 2;	     // floor
  wmin = WIDTH(idx, idx+h);
  // Search for half sample with smallest width
  limit = idx+h;
  for (int i = idx+1; i < limit; i++) {
    // Break ties in favor of lower value
    if (WIDTH(i, i+h) < wmin) {
      wmin = WIDTH(i, i+h);
      idx = i;
    }
  }
  n = h+1;
  goto tailcall;
}

// Returns -1 when there are insufficient samples
static int64_t percentile(int pct,
			  struct rusage *usagedata,
			  int64_t (accessor)(struct rusage *),
			  int *indices) {
  if (pct < 90) bail("Error: refuse to calculate percentiles less than 90");
  if (pct > 99) bail("Error: unable to calculate percentiles greater than 99");
  // Number of samples needed for a percentile calculation
  int samples = 100 / (100 - pct);
  if (config.runs < samples) return -1;
  int idx = config.runs - (config.runs / samples);
  return accessor(&usagedata[indices[idx]]);
}

// Produce a statistical summary (stored in 's') of usagedata over all runs
static void measure(struct rusage *usagedata,
		    int64_t (accessor)(struct rusage *),
		    comparator compare,
		    measures *meas) {
  // Note: 'meas' is filled with zeros on initial allocation
  if (config.runs < 1) return;
  int *indices = malloc(config.runs * sizeof(int));
  if (!indices) bail("Out of memory");
  for (int i = 0; i < config.runs; i++) indices[i] = i;
  sort(indices, config.runs, sizeof(int), usagedata, compare);
  meas->median = MEDIAN_SELECT(accessor, indices, usagedata);
  meas->min = accessor(&usagedata[indices[0]]);
  meas->max = accessor(&usagedata[indices[config.runs - 1]]);
  meas->mode = estimate_mode(usagedata, accessor, indices);
  meas->pct95 = percentile(95, usagedata, accessor, indices);
  meas->pct99 = percentile(99, usagedata, accessor, indices);
  free(indices);
  return;
}

static summary *new_summary(void) {
  summary *s = calloc(1, sizeof(summary));
  if (!s) bail("Out of memory");
  return s;
}

void free_summary(summary *s) {
  free(s->cmd);
  free(s->shell);
  free(s);
}

summary *summarize(char *cmd, int fail_count, struct rusage *usagedata) {

  summary *s = new_summary();

  s->cmd = strdup(cmd);
  s->shell = config.shell ? strdup(config.shell) : NULL;
  s->runs = config.runs;
  s->fail_count = fail_count;

  if (config.runs < 1) {
    // No data to process, and 's' contains all zeros in the numeric
    // fields except for number of runs as set above
    return s;
  }

  measure(usagedata, totaltime, compare_totaltime, &s->total);
  measure(usagedata, usertime, compare_usertime, &s->user);
  measure(usagedata, systemtime, compare_systemtime, &s->system);
  measure(usagedata, rss, compare_rss, &s->rss);
  measure(usagedata, vcsw, compare_vcsw, &s->vcsw);
  measure(usagedata, icsw, compare_icsw, &s->icsw);
  measure(usagedata, tcsw, compare_tcsw, &s->tcsw);

  return s;
}


