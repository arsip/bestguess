//  -*- Mode: C; -*-                                                       
// 
//  stats.c
// 
//  Copyright (C) Jamie A. Jennings, 2024

//#include <math.h>
#include "common.h"
#include "utils.h"
#include "stats.h"

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

#define MEDIAN_SELECT(accessor, indices, usagedata)	\
  ((runs == (runs/2) * 2) 				\
   ?							\
   ((accessor(&(usagedata)[indices[runs/2 - 1]]) +	\
     accessor(&(usagedata)[indices[runs/2]])) / 2)	\
   :							\
   (accessor(&usagedata[indices[runs/2]])))		\

static int64_t really_find_median(struct rusage *usagedata,
				  int64_t (accessor)(struct rusage *),
				  comparator compare) {
  if (runs < 1) return 0;
  int64_t result;
  int *indices = malloc(runs * sizeof(int));
  if (!indices) bail("Out of memory");
  for (int i = 0; i < runs; i++) indices[i] = i;
  sort(indices, runs, sizeof(int), usagedata, compare);
  result = MEDIAN_SELECT(accessor, indices, usagedata);
  free(indices);
  return result;
}

int64_t find_median(struct rusage *usagedata, int field) {
  switch (field) {
    case F_RSS:
      return really_find_median(usagedata, rss, compare_rss);
    case F_USER:
      return really_find_median(usagedata, usertime, compare_usertime);
    case F_SYSTEM:
      return really_find_median(usagedata, systemtime, compare_systemtime);
    case F_TOTAL:
      return really_find_median(usagedata, totaltime, compare_totaltime);
    case F_TCSW:
      return really_find_median(usagedata, tcsw, compare_tcsw);
    default:
      bail("ERROR: Unsupported field code (find median)");
      return 0;			// To silence a warning
  }
}

Summary *summarize(char *cmd, struct rusage *usagedata) {

  Summary *s = new_Summary();

  s->cmd = escape(cmd);
  s->runs = runs;

  if (runs < 1) {
    // No data to process, and 's' contains all zeros in the numeric
    // fields except for number of runs
    return s;
  }

  int64_t umin = INT64_MAX,   umax = 0;
  int64_t smin = INT64_MAX,   smax = 0;
  int64_t tmin = INT64_MAX,   tmax = 0;
  int64_t rssmin = INT64_MAX, rssmax = 0;
  int64_t cswmin = INT64_MAX, cswmax = 0;

  // Find mins and maxes
  for (int i = 0; i < runs; i++) {
    if (usertime(&usagedata[i]) < umin) umin = usertime(&usagedata[i]);
    if (systemtime(&usagedata[i]) < smin) smin = systemtime(&usagedata[i]);
    if (totaltime(&usagedata[i]) < tmin) tmin = totaltime(&usagedata[i]);
    if (rss(&usagedata[i]) < rssmin) rssmin = rss(&usagedata[i]);
    if (tcsw(&usagedata[i]) < cswmin) cswmin = tcsw(&usagedata[i]);

    if (usertime(&usagedata[i]) > umax) umax = usertime(&usagedata[i]);
    if (systemtime(&usagedata[i]) > smax) smax = systemtime(&usagedata[i]);
    if (totaltime(&usagedata[i]) > tmax) tmax = totaltime(&usagedata[i]);
    if (rss(&usagedata[i]) > rssmax) rssmax = rss(&usagedata[i]);
    if (tcsw(&usagedata[i]) > cswmax) cswmax = tcsw(&usagedata[i]);
  }
  
  s->tmin = tmin;
  s->tmax = tmax;
  s->umin = umin;
  s->umax = umax;
  s->smin = smin;
  s->smax = smax;
  s->rssmin = rssmin;
  s->rssmax = rssmax;
  s->cswmin = cswmin;
  s->cswmax = cswmax;

  // Find medians
  s->user = find_median(usagedata, F_USER);
  s->system = find_median(usagedata, F_SYSTEM);
  s->total = find_median(usagedata, F_TOTAL);
  s->rss = find_median(usagedata, F_RSS);
  s->csw = find_median(usagedata, F_TCSW);

  return s;
}



