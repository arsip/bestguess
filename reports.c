//  -*- Mode: C; -*-                                                       
// 
//  reports.c
// 
//  Copyright (C) Jamie A. Jennings, 2024

#include "bestguess.h"
#include "utils.h"
#include "reports.h"
#include <stdio.h>
#include <string.h>

#define FMT "%6.1f"
#define FMTs "%6.3f"
#define IFMT "%6" PRId64
#define LABEL "  %-22s"
#define GAP   "   "
#define DASH  "───"

void print_command_summary(summary *s) {
  if (brief_summary)
    printf(LABEL "  Mode" GAP
	   "    Min    Median     Max   \n", "");
  else
    printf(LABEL "  Mode" GAP
	   "┌── Min ── Median ── 95th ─── 99th ──── Max ──┐\n", "");

  if (runs < 1) {
    printf("  No data (number of timed runs was %d)\n", runs);
    return;
  }

  int sec = 0;
  const char *fmt;
  double divisor;
  
  // Decide on which time unit to use for printing the summary
  if (s->total.max > (1000 * 1000)) {
    if (brief_summary)
      //          Mode         Min     Median    Max
      fmt = LABEL FMTs GAP "│" FMTs GAP FMTs GAP FMTs "   │\n";
    else
      //          Mode         Min     Median    95th     99th     Max
      fmt = LABEL FMTs GAP "│" FMTs GAP FMTs GAP FMTs GAP FMTs GAP FMTs "   │\n";
    sec = 1;
    divisor = 1000.0 * 1000.0;
  } else {
    if (brief_summary)
      //          Mode        Min   Median    Max
      fmt = LABEL FMT GAP "│" FMT GAP FMT GAP FMT  "   │\n";
    else
      //          Mode        Min    Median    95th     99th     Max
      fmt = LABEL FMT GAP "│" FMT GAP FMT  GAP FMT  GAP FMT  GAP FMT  "   │\n";
    divisor = 1000.0;
  }

  printf(fmt, sec ? "Total time (sec)" : "Total time (ms)",
	 (double)(s->total.mode / divisor),
	 (double)(s->total.min / divisor),
	 (double)(s->total.median / divisor),
	 (double)(s->total.pct95 / divisor),
	 (double)(s->total.pct99 / divisor),
	 (double)(s->total.max / divisor));

  if (!brief_summary) {
    printf(fmt, sec ? "User time (sec)" : "User time (ms)",
	   (double)(s->user.mode / divisor), 
	   (double)(s->user.min / divisor), 
	   (double)(s->user.median / divisor), 
	   (double)(s->user.pct95 / divisor), 
	   (double)(s->user.pct99 / divisor), 
	   (double)(s->user.max / divisor));

    printf(fmt, sec ? "System time (sec)" : "System time (ms)",
	   (double)(s->system.mode / divisor), 
	   (double)(s->system.min / divisor), 
	   (double)(s->system.median / divisor), 
	   (double)(s->system.pct95 / divisor), 
	   (double)(s->system.pct99 / divisor), 
	   (double)(s->system.max / divisor));

    printf(fmt, "Max RSS (MiB)", 
	   (double) s->rss.mode / (1024.0 * 1024.0),
	   (double) s->rss.min / (1024.0 * 1024.0),
	   (double) s->rss.median / (1024.0 * 1024.0),
	   (double) s->rss.pct95 / (1024.0 * 1024.0),
	   (double) s->rss.pct99 / (1024.0 * 1024.0),
	   (double) s->rss.max / (1024.0 * 1024.0));

    printf(LABEL IFMT GAP "│" IFMT GAP IFMT GAP IFMT GAP IFMT GAP IFMT "   │\n",
	   "Context switches (ct)",
	   s->tcsw.mode, s->tcsw.min, s->tcsw.median,
	   s->tcsw.pct95, s->tcsw.pct99, s->tcsw.max);

    printf(LABEL GAP "      └─────────────────────────────────────────────┘\n", "");
  }

  fflush(stdout);
}

#define BAR "▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭"

void print_graph(summary *s, struct rusage *usagedata) {
  int bars;
  int bytesperbar = (uint8_t) BAR[0] >> 6; // Assumes UTF-8
  int maxbars = strlen(BAR) / bytesperbar;
  int64_t tmax = s->total.max;
  printf("0%*smax\n", maxbars - 1, "");
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
  printf("Best guess is:\n");
  printf("  %s ran\n", *commands[best] ? commands[best] : "(empty)");
  for (int i = 0; i < n; i++) {
    if (i != best) {
      factor = (double) mediantimes[i] / (double) fastest;
      printf("  %6.2f times faster than %s\n", factor, commands[i]);
    }
  }

  fflush(stdout);
}
