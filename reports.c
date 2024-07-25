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
#define ENDLINE "   │\n"

#define PRINT_(fmt_s, fmt_ms, field) do {				\
    if ((field) < 0)							\
      printf("%6s", "   - ");						\
    else								\
      printf(sec ? fmt_s : fmt_ms, field);				\
  } while (0)

#define PRINTTIME(field) do {					\
    PRINT_(FMTs, FMT, (double)((field) / divisor));		\
    printf(GAP);						\
  } while (0)

#define PRINTTIMENL(field) do {					\
    PRINT_(FMTs, FMT, (double)((field) / divisor));		\
    printf(ENDLINE);						\
  } while (0)

#define PRINTINT(field) do {					\
    PRINT_(IFMT, IFMT, field);					\
    printf(GAP);						\
  } while (0)

#define PRINTINTNL(field) do {					\
    PRINT_(IFMT, IFMT, field);					\
    printf(ENDLINE);						\
  } while (0)

void print_command_summary(summary *s) {
  if (brief_summary)
    printf(LABEL "  Mode" GAP
	   "│   Min    Median     Max" ENDLINE, "");
  else
    printf(LABEL "  Mode" GAP
	   "┌── Min ── Median ── 95th ─── 99th ──── Max ──┐\n", "");

  if (runs < 1) {
    printf("  No data (number of timed runs was %d)\n", runs);
    return;
  }

  int sec = 0;
  const char *fmt1;
  double divisor;
  
  // Decide on which time unit to use for printing the summary
  if (s->total.max > (1000 * 1000)) {
    //           Mode         Min     Median  
    fmt1 = LABEL FMTs GAP "│" FMTs GAP FMTs GAP;
    //         95th     99th     Max
    //fmt2full = FMTs GAP FMTs GAP FMTs ENDLINE;
    //         Max
    //fmt2brief = FMTs ENDLINE;
    sec = 1;
    divisor = 1000.0 * 1000.0;
    } else {
    //           Mode        Min   Median 
    fmt1 = LABEL FMT GAP "│" FMT GAP FMT GAP;
    //         95th    99th    Max
    //fmt2full = FMT GAP FMT GAP FMT ENDLINE;
    //          Max
    //fmt2brief = FMT ENDLINE;
    divisor = 1000.0;
  } // Should we print seconds or milliseconds?

  printf(fmt1, sec ? "Total time (sec)" : "Total time (ms)",
	 (double)(s->total.mode / divisor),
	 (double)(s->total.min / divisor),
	 (double)(s->total.median / divisor));

  if (!brief_summary) {
    PRINTTIME(s->total.pct95);
    PRINTTIME(s->total.pct99);
  }
  PRINTTIMENL(s->total.max);

  printf(fmt1, sec ? "User time (sec)" : "User time (ms)",
	 (double)(s->user.mode / divisor), 
	 (double)(s->user.min / divisor), 
	 (double)(s->user.median / divisor));

  if (!brief_summary) {
    PRINTTIME(s->user.pct95);
    PRINTTIME(s->user.pct99);
  }
  PRINTTIMENL(s->user.max);

  printf(fmt1, sec ? "System time (sec)" : "System time (ms)",
	 (double)(s->system.mode / divisor), 
	 (double)(s->system.min / divisor), 
	 (double)(s->system.median / divisor));

  if (!brief_summary) {
    PRINTTIME(s->system.pct95);
    PRINTTIME(s->system.pct99);
  }
  PRINTTIMENL(s->system.max);

  divisor = 1024.0 * 1024.0;
  printf(fmt1, "Max RSS (MiB)", 
	 (double) s->rss.mode / divisor,
	 (double) s->rss.min / divisor,
	 (double) s->rss.median / divisor);

  // Misusing PRINTTIME because it does the right thing
  if (!brief_summary) {
    PRINTTIME(s->rss.pct95);
    PRINTTIME(s->rss.pct99);
  }
  PRINTTIMENL(s->rss.max);

  printf(LABEL IFMT GAP "│" IFMT GAP IFMT GAP,
	 "Context switches (ct)",
	 s->tcsw.mode, s->tcsw.min, s->tcsw.median);
	   
  if (!brief_summary) {
    PRINTINT(s->tcsw.pct95);
    PRINTINT(s->tcsw.pct99);
  }
  PRINTINTNL(s->tcsw.max);

  if (!brief_summary)
    printf(LABEL GAP "      └─────────────────────────────────────────────┘\n", "");

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
