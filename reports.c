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
#define BARFMT "%s"

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
    printf(brief_summary ? "\n" : ENDLINE);			\
  } while (0)

#define PRINTINT(field) do {					\
    PRINT_(IFMT, IFMT, field);					\
    printf(GAP);						\
  } while (0)

#define PRINTINTNL(field) do {					\
    PRINT_(IFMT, IFMT, field);					\
    printf(brief_summary ? "\n" : ENDLINE);			\
  } while (0)

#define MAYBEBAR do {				\
    printf("%s", brief_summary ? " " : "│");	\
  } while (0)

void print_command_summary(summary *s) {
  if (runs < 1) {
    printf("\n  No data (number of timed runs was %d)\n", runs);
    return;
  }

  if (brief_summary)
    printf(LABEL "  Mode" GAP "    Min    Median     Max\n", "");
  else
    printf(LABEL "  Mode" GAP
	   "┌── Min ── Median ── 95th ─── 99th ──── Max ──┐\n", "");


  int sec = 1;
  double divisor = 1000 * 1000;
  
  // Decide on which time unit to use for printing the summary
  if (s->total.max <= divisor) {
    sec = 0;
    divisor = 1000.0;
  }

  printf(LABEL, sec ? "Total time (sec)" : "Total time (ms)");
  PRINTTIME(s->total.mode); MAYBEBAR;
  PRINTTIME(s->total.min);
  PRINTTIME(s->total.median);

  if (!brief_summary) {
    PRINTTIME(s->total.pct95);
    PRINTTIME(s->total.pct99);
  }
  PRINTTIMENL(s->total.max);

  if (!brief_summary) {
    printf(LABEL, sec ? "User time (sec)" : "User time (ms)");
    PRINTTIME(s->user.mode); MAYBEBAR;
    PRINTTIME(s->user.min);
    PRINTTIME(s->user.median);

    PRINTTIME(s->user.pct95);
    PRINTTIME(s->user.pct99);
    PRINTTIMENL(s->user.max);

    printf(LABEL, sec ? "System time (sec)" : "System time (ms)");
    PRINTTIME(s->system.mode); MAYBEBAR;
    PRINTTIME(s->system.min);
    PRINTTIME(s->system.median);

    PRINTTIME(s->system.pct95);
    PRINTTIME(s->system.pct99);
    PRINTTIMENL(s->system.max);

    divisor = 1024.0 * 1024.0;
    printf(LABEL, "Max RSS (MiB)");
    PRINTTIME(s->rss.mode); MAYBEBAR;
    PRINTTIME(s->rss.min);
    PRINTTIME(s->rss.median);

    // Misusing PRINTTIME because it does the right thing
    PRINTTIME(s->rss.pct95);
    PRINTTIME(s->rss.pct99);
    PRINTTIMENL(s->rss.max);

    printf(LABEL, "Context switches (ct)");
    PRINTINT(s->tcsw.mode); MAYBEBAR;
    PRINTINT(s->tcsw.min);
    PRINTINT(s->tcsw.median);
	   
    PRINTINT(s->tcsw.pct95);
    PRINTINT(s->tcsw.pct99);
    PRINTINTNL(s->tcsw.max);

    printf(LABEL GAP "      └─────────────────────────────────────────────┘\n", "");
  } // if not brief_summary

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
			   int64_t modes[],
			   int n) {

  if ((n < 2) || (runs < 1)) return;

  int best = 0;
  int64_t fastest = modes[best];
  double factor;
  
  for (int i = 1; i < n; i++)
    if (modes[i] < fastest) {
      fastest = modes[i];
      best = i;
    }

  printf("Best guess is:\n");
  printf("  %s ran\n", *commands[best] ? commands[best] : "(empty)");
  for (int i = 0; i < n; i++) {
    if (i != best) {
      factor = (double) modes[i] / (double) fastest;
      printf("  %6.2f times faster than %s\n", factor, commands[i]);
    }
  }

  fflush(stdout);
}
