//  -*- Mode: C; -*-                                                       
// 
//  reports.c
// 
//  Copyright (C) Jamie A. Jennings, 2024

#include "bestguess.h"
#include "utils.h"
#include "reports.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

void announce_command(const char *cmd, int number) {
  printf("Command %d: %s\n", number, *cmd ? cmd : "(empty)");
}

#define FMT "%6.1f"
#define FMTs "%6.2f"
#define IFMT "%6" PRId64
#define LABEL "  %14s "
#define GAP   "   "
#define UNITS 1
#define NOUNITS 0
// Round to one decimal place
#define ROUND1(intval, divisor) \
  (round((double)((intval) * 10) / (divisor)) / 10.0)

#define LEFTBAR(briefly) do {			\
    printf("%s", briefly ? "  " : "│ ");	\
  } while (0)

#define RIGHTBAR(briefly) do {			\
    printf("%s", briefly ? "\n" : "   │\n");	\
  } while (0)

#define PRINT_(fmt_s, fmt_ms, field) do {				\
    if ((field) < 0) 							\
      printf("%6s", "   - ");						\
    else 								\
      printf(sec ? fmt_s : fmt_ms, field);				\
  } while (0)

#define PRINTMODE(field, briefly) do {				\
    PRINT_(FMTs, FMT, ROUND1(field, divisor));			\
    printf(" %-3s", units);					\
    printf(GAP);						\
    LEFTBAR(briefly);						\
  } while (0)

#define PRINTTIME(field) do {					\
    PRINT_(FMTs, FMT, ROUND1(field, divisor));			\
    printf(GAP);						\
  } while (0)

#define PRINTTIMENL(field, briefly) do {			\
    PRINT_(FMTs, FMT, ROUND1(field, divisor));			\
    RIGHTBAR(briefly);						\
  } while (0)

#define PRINTCOUNT(field, showunits) do {			\
    if ((field) < 0)						\
      printf("%6s", "   - ");					\
    else if (divisor == 1) {					\
      printf("%6" PRId64, (field));				\
    } else {							\
      printf(FMTs, ROUND1(field, divisor));			\
    }								\
    if (showunits) printf(" %-3s", units);			\
  } while (0)

void print_summary(Summary *s, bool briefly) {
  if (!s) {
    printf("  No data\n");
    return;
  }

  if (briefly)
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
  PRINTMODE(s->total.mode, briefly);
  PRINTTIME(s->total.min);
  PRINTTIME(s->total.median);

  if (!briefly) {
    PRINTTIME(s->total.pct95);
    PRINTTIME(s->total.pct99);
  }
  PRINTTIMENL(s->total.max, briefly);

  if (!briefly) {
    printf(LABEL, "User time");
    PRINTMODE(s->user.mode, briefly);
    PRINTTIME(s->user.min);
    PRINTTIME(s->user.median);

    PRINTTIME(s->user.pct95);
    PRINTTIME(s->user.pct99);
    PRINTTIMENL(s->user.max, briefly);

    printf(LABEL, "System time");
    PRINTMODE(s->system.mode, briefly);
    PRINTTIME(s->system.min);
    PRINTTIME(s->system.median);

    PRINTTIME(s->system.pct95);
    PRINTTIME(s->system.pct99);
    PRINTTIMENL(s->system.max, briefly);
  }

  printf(LABEL, "Wall clock");
  PRINTMODE(s->wall.mode, briefly);
  PRINTTIME(s->wall.min);
  PRINTTIME(s->wall.median);

  if (!briefly) {
    PRINTTIME(s->wall.pct95);
    PRINTTIME(s->wall.pct99);
  }
  PRINTTIMENL(s->wall.max, briefly);

  if (!briefly) {
    divisor = MEGA;
    units = "MiB";
    // More than 3 digits left of decimal place?
    if (s->maxrss.max >= MEGA * 1000) {
      divisor *= 1024;
      units = "GiB";
    }
    printf(LABEL, "Max RSS");
    PRINTMODE(s->maxrss.mode, briefly);
    PRINTTIME(s->maxrss.min);
    PRINTTIME(s->maxrss.median);

    // Misusing PRINTTIME because it does the right thing
    PRINTTIME(s->maxrss.pct95);
    PRINTTIME(s->maxrss.pct99);
    PRINTTIMENL(s->maxrss.max, briefly);

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

void print_graph(Summary *s, Usage *usage, int start, int end) {
  // No data if 's' or 'usage' are NULL
  if (!s || !usage) return;
  // Error if start or end is out of range
  if ((start < 0) || (start >= usage->next) || (end < 0) || (end > usage->next))
    PANIC("usage data indices out of bounds");
  int bars;
  int bytesperbar = (uint8_t) BAR[0] >> 6; // Assumes UTF-8
  int maxbars = strlen(BAR) / bytesperbar;
  int64_t tmax = s->total.max;
  printf("0%*smax\n", maxbars - 1, "");
  for (int i = start; i < end; i++) {
    bars = (int) (get_int64(usage, i, F_TOTAL) * maxbars / tmax);
    if (bars <= maxbars)
      printf("│%.*s\n", bars * bytesperbar, BAR);
    else
      printf("│time exceeds plot size: %" PRId64 " us\n",
	     get_int64(usage, i, F_TOTAL));
  }
  fflush(stdout);
}

#define MODE(i) (summaries[i]->total.mode)
#define COMMAND(i) (summaries[i]->cmd)

void print_overall_summary(Summary *summaries[], int start, int end) {
  if (!summaries) PANIC_NULL();

  // Need at least two commands in order to have a comparison
  if ((end - start) < 2) {
    // Command groups are a feature of commands read from a file
    if (config.input_filename && !config.reducing_mode) {
      printf("Command group contains only one command\n");
    }
    return;
  }

  if (!summaries[start]) return; // No data

  int best = start;
  int64_t fastest = MODE(best);
  double factor;
  
  // Find the best total time, and also check for lack of data
  for (int i = start; i < end; i++) {
    if (!summaries[i]) return;	// No data
    if (MODE(i) < fastest) {
      fastest = MODE(i);
      best = i;
    }
  }

  if ((end - start) > 1) {
    printf("Best guess is:\n");
    printf("  %s ran\n", *COMMAND(best) ? COMMAND(best) : "(empty)");
    for (int i = start; i < end; i++) {
      if (i != best) {
	factor = (double) MODE(i) / (double) fastest;
	printf("  %6.2f times faster than %s\n",
	       factor, *COMMAND(best) ? COMMAND(i) : "(empty)");
      }
    }
  }

  fflush(stdout);
}

// Boxplot convention:
//
//  Q0  Q1  Q2  Q3      Q4
//      ┌───┬────┐
//  ├╌╌╌┤   │    ├╌╌╌╌╌╌╌╌┤
//      └───┴────┘

// These macros control how the boxplot looks, and are somewhat arbitrary.
#define WIDTHMIN 40
#define TICKSPACING 10
#define LABELWIDTH 4
#define AXISLINE "────────────────────────────────────────────────────────────"

static void print_boxplot_labels(int axismin, int axismax, int width) {
  //
  // Print tick mark labels for x-axis
  //
  width = width - (LABELWIDTH - 1);
  int ticks = 1 + width / TICKSPACING;
  double incr = round((double) (TICKSPACING * (axismax - axismin)) / (double) width);
  for (int i = 0; i < ticks; i++) {
    printf("%*.0f%*s",
	   LABELWIDTH, round((double) axismin + (i * incr)),
	   TICKSPACING - LABELWIDTH, "");
  }
  printf("\n");
}

// Tick marks are labeled with as integers with LABELWIDTH digits.
// E.g. when LABELWIDTH is 4, the highest tick mark label is 9999.
// The argument 'width' is the total terminal (or desired) width,
// beyond which we will not print.
void print_boxplot_scale(int axismin, int axismax, int width, int labelplacement) {
  if (width < WIDTHMIN) {
    printf("Requested width (%d) too narrow for plot\n", width);
    return;
  }
  if (labelplacement == BOXPLOT_LABEL_ABOVE)
    print_boxplot_labels(axismin, axismax, width);
  //
  // Print axis graphic
  //
  printf("%*s├", (LABELWIDTH - 1), "");
  int currpos = LABELWIDTH;
  int bytesperunit = (uint8_t) AXISLINE[0] >> 6; // Assumes UTF-8
  assert((strlen(AXISLINE) / bytesperunit) > TICKSPACING);

  int ticks = 1 + width / TICKSPACING;
  // Already printed first visual tick mark, will print last one later
  for (int i = 0; i < (ticks - 2); i++) {
    printf("%.*s┼", (TICKSPACING - 1) * bytesperunit, AXISLINE);
    currpos += TICKSPACING;
  }

  printf("%.*s", (TICKSPACING - 1) * bytesperunit, AXISLINE);
  currpos += TICKSPACING;
  if (currpos < width) 
    printf("┼%.*s", (width - currpos) * bytesperunit, AXISLINE);
  else
    printf("┤");
  printf("\n");

  if (labelplacement == BOXPLOT_LABEL_BELOW)
    print_boxplot_labels(axismin, axismax, width);

}

//
// To facilitate debugging how boxplots are printed:
//
// Call print_ticks() with the POSITIONS (e.g. 0..79 for a terminal
// width of 80) of each quartile value.
//
__attribute__((unused))
void print_ticks(Measures *m, int width) {
  // IMPORTANT: We are truncating int64's down to ints here!  This
  // will only work, obvi, if the values are small, which is what is
  // needed for printing.
  int Q0 = m->min;
  int Q1 = m->Q1;
  int Q2 = m->median;
  int Q3 = m->Q3;
  int Q4 = m->max;
  int indent = LABELWIDTH - 1;
  width -= indent;
  printf("%*s", indent, "");
  for (int i = 0; i < (width / 10); i++) printf("%d         ", i);
  if ((width % 10) == 1) printf("%d         ", width/10);
  printf("\n%*s", indent, "");
  for (int i = 0; i < (width / 10); i++) printf("0123456789");
  printf("%.*s", width % 10, "0123456789");
  printf("\n%*s", indent, "");
  if (Q1 > Q0) printf("%*s", Q0+1, "<");
  printf("%*s", Q1 - Q0 - (Q1 == Q2), (Q1 == Q2) ? "" : "|");
  printf("%*s", Q2 - Q1, "X");
  printf("%*s", Q3 - Q2, (Q2 == Q3) ? "" : "|");
  if (Q4 > Q3) printf("%*s", Q4 - Q3, ">");
  printf("\n");
}


#define SCALE(val) (round(((double) (val) * scale)))

void print_boxplot(Measures *m, int64_t axismin, int64_t axismax, int width) {
  if (!m) PANIC_NULL();
  if (width < WIDTHMIN) {
    printf("Width %d too narrow for plot\n", width);
    return;
  }
  if ((m->min < axismin) || (m->max > axismax)) {
    printf("Measurement min/max outside of axis min/max values");
    return;
  }
  int indent = LABELWIDTH - 1;
  width -= indent;
  double scale = (double) (width - 1) / (double) (axismax - axismin);

  int minpos = SCALE(m->min - axismin);
  int boxleft = SCALE(m->Q1 - axismin);
  int median = SCALE(m->median - axismin);
  int boxright = SCALE(m->Q3 - axismin);
  int maxpos = SCALE(m->max - axismin);
  int boxwidth = boxright - boxleft;
//   printf("BOXPLOT scale = %4.3f  min/Q1/med/Q3/max: %.2f %.2f %.2f %.2f %.2f\n",
// 	 scale,
// 	 scale * (double)(m->min - axismin),
// 	 scale * (double)(m->Q1 - axismin),
// 	 scale * (double)(m->median - axismin),
// 	 scale * (double)(m->Q3 - axismin),
// 	 scale * (double)(m->max - axismin));

  // Will we show the median as its own character?
  int show_median = 1;
  if ((median == boxleft) || (median == boxright))
    show_median = 0;

  // Top line
  printf("%*s", indent + boxleft, "");
  if (boxwidth > 0) {
    if (median == boxleft) printf("╓");
    else printf("┌");
    for (int j = 1; j < median - boxleft ; j++) printf("─");
    if (show_median) printf("┬");
    for (int j = 1; j < boxright - median; j++) printf("─");
    if (median == boxright) printf("╖");
    else printf("┐");
  }
  printf("\n");
  // Middle line
  if (minpos == maxpos) {
    printf("%*s\n", indent + minpos, "┼");
  } else {
    printf("%*s", indent + minpos, "");
    if (boxleft > minpos) printf("├");
    for (int j = 1; j < boxleft - minpos; j++) printf("╌");
    if (boxwidth == 0) {
      printf("┼");
    } else if (boxwidth == 1) {
      if (median == boxleft) printf("╢");
      else printf("┤");
      if (median == boxright) printf("╟");
      else printf("├");
    } else {
      if (median == boxleft) printf("╢");
      else printf("┤");
    }
    for (int j = 1; j < median - boxleft; j++) printf(" ");
    if (show_median) printf("│");
    for (int j = 1; j < boxright - median; j++) printf(" ");

    if (median == boxright) printf("╟");
    else printf("├");

    for (int j = 1; j < (maxpos - boxright); j++) printf("╌");
    if ((maxpos - boxright) > 0) printf("┤");
    printf("\n");
  }
  // Bottom line
  printf("%*s", indent + boxleft, "");
  if (boxwidth > 0) {
    if (median == boxleft) printf("╙");
    else printf("└");
    for (int j = 1; j < median - boxleft; j++) printf("─");
    if (show_median) printf("┴");
    for (int j = 1; j < boxright - median; j++) printf("─");
    if (median == boxright) printf("╜");
    else printf("┘");
  }
  printf("\n");
}

#define MS(nanoseconds) (ROUND1(nanoseconds, 1000.0))
#define MSFMT "%8.1fms"
#define COLSEP "  "
#define SKEWSIGNIFICANCE 0.20
#define INDENT "  "

void print_distribution_report(Measures *m, int n) {
  printf(INDENT "About the distribution:\n\n");

  printf(INDENT "     Min         Q₁         Median       Q₃          Max   \n");
  printf(INDENT MSFMT COLSEP MSFMT COLSEP MSFMT COLSEP MSFMT COLSEP MSFMT "\n",
	 MS(m->min),
	 MS(m->Q1),
	 MS(m->median),
	 MS(m->Q3),
	 MS(m->max));

  printf("\n");
  printf(INDENT "N (data points) = %d\n", n);
  int64_t IQR = m->Q3 - m->Q1;
  int64_t range = m->max - m->min;
  printf(INDENT "Inter-quartile range = %0.2f (%0.2f%% of total range)\n",
	 MS(IQR), (MS(IQR) * 100.0/ MS(range)));

  // How far below and above the median
  int64_t IQ1delta = m->median - m->Q1;
  int64_t IQ3delta = m->Q3 - m->median;
  printf(INDENT "Relative to the median, the IQR is [-%0.1fms, +%0.1fms]\n",
	 MS(IQ1delta), MS(IQ3delta));

  if (m->code == 0) {
    if (m->ADscore > 0)
      printf(INDENT "Distribution %s normal (AD score %4.2f) with p = %6.4f\n",
	     (m->p_normal <= 0.10) ? "is NOT" : "could be",
	     m->ADscore,
	     m->p_normal);
    else
      printf(INDENT "Unable to test for normality %s\n",
	     (m->est_stddev == 0) ? "(variance too low)" : "(too few data points)");

  }
  if (!HAS(m->code, CODE_LOWVARIANCE) && !HAS(m->code, CODE_SMALLN))
    printf(INDENT "Non-parametric skew of %4.2f is %s (magnitude is %s %0.2f)\n",
	   m->skew,
	   (fabs(m->skew) > SKEWSIGNIFICANCE) ? "SIGNIFICANT" : "insignificant",
	   (fabs(m->skew) > SKEWSIGNIFICANCE) ? "above" : "at/below",
	   SKEWSIGNIFICANCE);

  if (HAS(m->code, CODE_LOWVARIANCE))
    printf(INDENT
	   "Low variance blocked AD test and skew calculation\n");
  if (HAS(m->code, CODE_HIGHZ))
    printf(INDENT
	   "High variance blocked AD test (normal distribution unlikely)\n");
  if (HAS(m->code, CODE_SMALLN))
    printf(INDENT
	   "Too few data points for AD or skew calculation\n");
    
      
  printf("\n");
}
