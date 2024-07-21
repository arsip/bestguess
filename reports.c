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

void print_command_summary(summary *s) {
  if (!brief_summary)
    printf("                      Median               Range\n");

  if (runs < 1) {
    printf("    No data (number of timed runs was %d)\n", runs);
    return;
  }

  const char *units, *timefmt;
  double divisor;
  
  // Decide on which time unit to use for printing the summary
  if (s->tmax > (1000 * 1000)) {
    timefmt = "%s" FMTs "     " FMTs " - " FMTs "\n",
    units = "s";
    divisor = 1000.0 * 1000.0;
  } else {
    timefmt = "%s" FMT "     " FMT " - " FMT "\n",
    units = "ms";
    divisor = 1000.0;
  }

  const char *timelabel;
  if (brief_summary)
    timelabel = "  Median time      ";
  else
    timelabel = "  Total time       ";

  printf(timefmt, timelabel,
	 (double)(s->total / divisor), units,
	 (double)(s->tmin / divisor), units,
	 (double)(s->tmax / divisor), units);

  if (!brief_summary) {
    printf("  User time        "  FMT "     " FMT " - " FMT "\n",
	   (double)(s->umin / divisor), units,
	   (double)(s->umax / divisor), units,
	   (double)(s->user / divisor), units);

    printf("  System time      " FMT "     " FMT " - " FMT "\n",
	   (double)(s->smin / divisor), units,
	   (double)(s->smax / divisor), units,
	   (double)(s->system / divisor), units);

    printf("  Max RSS          " FMT "     " FMT " - " FMT "\n",
	   (double) s->rssmin / (1024.0 * 1024.0), "MiB",
	   (double) s->rssmax / (1024.0 * 1024.0), "MiB",
	   (double) s->rss / (1024.0 * 1024.0), "MiB");

    printf("  Context switches " IFMT " %-8s" IFMT " cnt - " IFMT " cnt\n",
	   s->csw, "count", s->cswmin, s->cswmax);
  }

  fflush(stdout);
}

#define BAR "▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭"

void print_graph(summary *s, struct rusage *usagedata) {
  int bars;
  int bytesperbar = (uint8_t) BAR[0] >> 6; // Assumes UTF-8
  int maxbars = strlen(BAR) / bytesperbar;
  int64_t tmax = s->tmax;
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
  printf("Best guess is\n");
  printf("  %s ran\n", *commands[best] ? commands[best] : "(empty)");
  for (int i = 0; i < n; i++) {
    if (i != best) {
      factor = (double) mediantimes[i] / (double) fastest;
      printf("  %6.2f times faster than %s\n", factor, commands[i]);
    }
  }

  fflush(stdout);
}
