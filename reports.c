//  -*- Mode: C; -*-                                                       
// 
//  reports.c
// 
//  Copyright (C) Jamie A. Jennings, 2024

#include "bestguess.h"
#include "reports.h"
#include <stdio.h>
#include <string.h>

#define FMT "%6.1f %-3s"
#define FMTs "%6.3f %-3s"
#define IFMT "%6" PRId64
#define LABEL "  %-16s"

void print_command_summary(summary *s) {
  printf("                      Mode       Median               Range\n");

  if (runs < 1) {
    printf("    No data (number of timed runs was %d)\n", runs);
    return;
  }

  const char *units, *timefmt;
  double divisor;
  
  // Decide on which time unit to use for printing the summary
  if (s->total.max > (1000 * 1000)) {
    timefmt = LABEL FMTs "  " FMTs "     " FMTs " - " FMTs "\n",
    units = "s";
    divisor = 1000.0 * 1000.0;
  } else {
    timefmt = LABEL FMT "  " FMT "     " FMT " - " FMT "\n",
    units = "ms";
    divisor = 1000.0;
  }

  printf(timefmt, "Total time",
	 (double)(s->total.mode / divisor), units,
	 (double)(s->total.median / divisor), units,
	 (double)(s->total.min / divisor), units,
	 (double)(s->total.max / divisor), units);

  if (!brief_summary) {
    printf(timefmt, "User time",
	   (double)(s->user.mode / divisor), units,
	   (double)(s->user.median / divisor), units,
	   (double)(s->user.min / divisor), units,
	   (double)(s->user.max / divisor), units);

    printf(timefmt, "System time",
	   (double)(s->system.mode / divisor), units,
	   (double)(s->system.median / divisor), units,
	   (double)(s->system.min / divisor), units,
	   (double)(s->system.max / divisor), units);

    printf(timefmt, "Max RSS",
	   (double) s->rss.mode / (1024.0 * 1024.0), "MiB",
	   (double) s->rss.median / (1024.0 * 1024.0), "MiB",
	   (double) s->rss.min / (1024.0 * 1024.0), "MiB",
	   (double) s->rss.max / (1024.0 * 1024.0), "MiB");

    printf(LABEL IFMT " count" IFMT " cnt     " IFMT " cnt - " IFMT " cnt\n",
	   "Context switches",
	   s->tcsw.mode, s->tcsw.median, s->tcsw.min, s->tcsw.max);
  }

  fflush(stdout);
}

#define BAR "▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭"

void print_graph(summary *s, struct rusage *usagedata) {
  int bars;
  int bytesperbar = (uint8_t) BAR[0] >> 6; // Assumes UTF-8
  int maxbars = strlen(BAR) / bytesperbar;
  int64_t tmax = s->total.max;
  printf("0%*smax\n", maxbars - 3, "");
  for (int i = 0; i < runs; i++) {
    bars = (int) (totaltime(&usagedata[i]) * maxbars / tmax);
    if (bars <= maxbars)
      printf("|%.*s\n", bars * bytesperbar, BAR);
    else
      printf("|time exceeds plot size: %" PRId64 " us\n",
	     totaltime(&usagedata[i]));
  }
  fflush(stdout);
}

void print_overall_summary(const char *commands[],
			   int64_t mediantimes[],
			   int n) {

  if ((n < 2) || (runs < 1)) return;

  int best = 0;
  int64_t fastest = mediantimes[best];
  double factor;
  
  for (int i = 1; i < n; i++)
    if (mediantimes[i] < fastest) {
      fastest = mediantimes[i];
      best = i;
    }
  printf("Best guess is that...\n");
  printf("  %s ran\n", *commands[best] ? commands[best] : "(empty)");
  for (int i = 0; i < n; i++) {
    if (i != best) {
      factor = (double) mediantimes[i] / (double) fastest;
      printf("  %6.2f times faster than %s\n", factor, commands[i]);
    }
  }

  fflush(stdout);
}
