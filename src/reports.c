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
#define FMTs "%6.2f"
#define IFMT "%6" PRId64
#define LABEL "  %14s "
#define GAP   "   "
#define UNITS 1
#define NOUNITS 0

#define LEFTBAR do {					\
    printf("%s", config.brief_summary ? "  " : "│ ");		\
  } while (0)

#define RIGHTBAR do {					\
    printf("%s", config.brief_summary ? "\n" : "   │\n");	\
  } while (0)

#define PRINT_(fmt_s, fmt_ms, field) do {				\
    if ((field) < 0) 							\
      printf("%6s", "   - ");						\
    else 								\
      printf(sec ? fmt_s : fmt_ms, field);				\
  } while (0)

#define PRINTMODE(field) do {					\
    PRINT_(FMTs, FMT, (double)((field) / divisor));		\
    printf(" %-3s", units);					\
    printf(GAP);						\
    LEFTBAR;							\
  } while (0)

#define PRINTTIME(field) do {					\
    PRINT_(FMTs, FMT, (double)((field) / divisor));		\
    printf(GAP);						\
  } while (0)

#define PRINTTIMENL(field) do {					\
    PRINT_(FMTs, FMT, (double)((field) / divisor));		\
    RIGHTBAR;							\
  } while (0)

#define PRINTCOUNT(field, showunits) do {			\
    if ((field) < 0)						\
      printf("%6s", "   - ");					\
    else if (divisor == 1) {					\
      printf("%6" PRId64, (field));				\
    } else {							\
      printf(FMTs, (double)((field) / divisor));		\
    }								\
    if (showunits) printf(" %-3s", units);			\
  } while (0)

void print_command_summary(summary *s) {
  if (config.runs < 1) {
    printf("\n  No data (number of timed runs was %d)\n", config.runs);
    return;
  }

  if (config.brief_summary)
    printf(LABEL "    Mode" GAP "       Min    Median     Max\n", "");
  else
    printf(LABEL "    Mode" GAP "  ┌    Min    Median    95th     99th      Max   ┐\n", "");


  int sec = 1;
  double divisor = MICROSECS;
  const char *units = "s";
  
  // Decide on which time unit to use for printing the summary
  if (s->total.max < divisor) {
    sec = 0;
    divisor = 1000.0;
    units = "ms";
  }

  printf(LABEL, "Total CPU time");
  PRINTMODE(s->total.mode);
  PRINTTIME(s->total.min);
  PRINTTIME(s->total.median);

  if (!config.brief_summary) {
    PRINTTIME(s->total.pct95);
    PRINTTIME(s->total.pct99);
  }
  PRINTTIMENL(s->total.max);

  if (!config.brief_summary) {
    printf(LABEL, "User time");
    PRINTMODE(s->user.mode);
    PRINTTIME(s->user.min);
    PRINTTIME(s->user.median);

    PRINTTIME(s->user.pct95);
    PRINTTIME(s->user.pct99);
    PRINTTIMENL(s->user.max);

    printf(LABEL, "System time");
    PRINTMODE(s->system.mode);
    PRINTTIME(s->system.min);
    PRINTTIME(s->system.median);

    PRINTTIME(s->system.pct95);
    PRINTTIME(s->system.pct99);
    PRINTTIMENL(s->system.max);
  }

  printf(LABEL, "Wall clock");
  PRINTMODE(s->wall.mode);
  PRINTTIME(s->wall.min);
  PRINTTIME(s->wall.median);

  if (!config.brief_summary) {
    PRINTTIME(s->wall.pct95);
    PRINTTIME(s->wall.pct99);
  }
  PRINTTIMENL(s->wall.max);

  if (!config.brief_summary) {
    divisor = MEGA;
    units = "MiB";
    // More than 3 digits left of decimal place?
    if (s->maxrss.max >= MEGA * 1000) {
      divisor *= 1024;
      units = "GiB";
    }
    printf(LABEL, "Max RSS");
    PRINTMODE(s->maxrss.mode);
    PRINTTIME(s->maxrss.min);
    PRINTTIME(s->maxrss.median);

    // Misusing PRINTTIME because it does the right thing
    PRINTTIME(s->maxrss.pct95);
    PRINTTIME(s->maxrss.pct99);
    PRINTTIMENL(s->maxrss.max);

    divisor = 1;
    units = "ct";
    if (s->tcsw.max >= 10000) {
      divisor = 1000;
      units = "Kct";
    }
    if (s->tcsw.max >= MICROSECS) { // Not a time
      divisor = MICROSECS;	    // but MICROSECS
      units = "Mct";		    // is convenient
    }
    printf(LABEL, "Context sw");
    PRINTCOUNT(s->tcsw.mode, UNITS);
    printf(GAP);
    printf("└ ");
    PRINTCOUNT(s->tcsw.min, NOUNITS);
    printf(GAP);
    PRINTCOUNT(s->tcsw.median, NOUNITS);
    printf(GAP);
	   
    PRINTCOUNT(s->tcsw.pct95, NOUNITS);
    printf(GAP);
    PRINTCOUNT(s->tcsw.pct99, NOUNITS);
    printf(GAP);
    PRINTCOUNT(s->tcsw.max, NOUNITS);
    printf("   ┘\n");

  } // if not brief_summary

  fflush(stdout);
}

#define BAR "▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭"

void print_graph(summary *s, Usage *usagedata) {
  int bars;
  int bytesperbar = (uint8_t) BAR[0] >> 6; // Assumes UTF-8
  int maxbars = strlen(BAR) / bytesperbar;
  int64_t tmax = s->total.max;
  printf("0%*smax\n", maxbars - 1, "");
  for (int i = 0; i < config.runs; i++) {
    bars = (int) (totaltime(&usagedata[i]) * maxbars / tmax);
    if (bars <= maxbars)
      printf("│%.*s\n", bars * bytesperbar, BAR);
    else
      printf("│time exceeds plot size: %" PRId64 " us\n",
	     totaltime(&usagedata[i]));
  }
  fflush(stdout);
}

void print_overall_summary(char *commands[],
			   int64_t modes[],
			   int start,
			   int end) {

  if (config.runs < 1) return;
  if (config.input_filename && ((end - start) < 2)) {
    printf("Command group contains only one command\n");
    return;
  }

  int best = start;
  int64_t fastest = modes[best];
  double factor;
  
  for (int i = start; i < end; i++)
    if (modes[i] < fastest) {
      fastest = modes[i];
      best = i;
    }

  if ((end - start) > 1) {
    printf("Best guess is:\n");
    printf("  %s ran\n", *commands[best] ? commands[best] : "(empty)");
    for (int i = start; i < end; i++) {
      if (i != best) {
	factor = (double) modes[i] / (double) fastest;
	printf("  %6.2f times faster than %s\n",
	       factor, *commands[i] ? commands[i] : "(empty)");
      }
    }
  }

  fflush(stdout);
}
