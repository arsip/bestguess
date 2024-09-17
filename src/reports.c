//  -*- Mode: C; -*-                                                       
// 
//  reports.c
// 
//  Copyright (C) Jamie A. Jennings, 2024

#include "bestguess.h"
#include "reports.h"
#include "utils.h"
#include "csv.h"
#include "printing.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#define SECOND(a, b, c) b,
const char *ReportOptionName[] = {XReports(SECOND)};
#undef SECOND
#define THIRD(a, b, c) c,
const char *ReportOptionDesc[] = {XReports(THIRD)};
#undef THIRD

#define BOXPLOT_NOLABELS 0
#define BOXPLOT_LABEL_ABOVE 1
#define BOXPLOT_LABEL_BELOW 2

// For printing help.  Caller must free the returned string.
char *report_options(void) {
  size_t bufsize = 1000;
  char *buf = malloc(bufsize);
  if (!buf) PANIC_OOM();
  int len = snprintf(buf, bufsize, "Valid report types are");
  for (ReportCode i = REPORT_NONE; i < REPORT_ERROR; i++) {
    bufsize -= len;
    len += snprintf(buf + len, bufsize, "\n  %-8s %s",
		    ReportOptionName[i], ReportOptionDesc[i]);
  }
  return buf;
}

ReportCode interpret_report_option(const char *op) {
  for (ReportCode i = REPORT_NONE; i < REPORT_ERROR; i++)
    if (strcmp(op, ReportOptionName[i]) == 0) return i;
  return REPORT_ERROR;
}

void announce_command(const char *cmd, int index) {
  printf("Command %d: %s\n", index + 1, *cmd ? cmd : "(empty)");
}

#define FMT "%6.1f"
#define FMTL "%-.1f"
#define FMTs "%6.2f"
#define FMTsL "%-.2f"
#define IFMT "%6" PRId64
#define IFMTL "%-6" PRId64
#define LABEL "  %15s "
#define GAP   "   "
#define UNITS 1
#define NOUNITS 0
// Scale and round to one or two decimal places
#define ROUND1(intval, divisor) \
  (round((double)((intval) * 10) / (divisor)) / 10.0)
#define ROUND2(intval, divisor) \
  (round((double)((intval) * 100) / (divisor)) / 100.0)

#define LEFT       0
#define RIGHT      1
#define TOPLINE    0
#define MIDLINE    1
#define BOTTOMLINE 2

static const char *bar(int side, int line) {
  const char *decor[6] = { "┌", "┐",
			   "│", "│",
			   "└", "┘" };
  if ((side != LEFT) && (side != RIGHT))
    return ":";
  if ((line < TOPLINE) || (line > BOTTOMLINE))
    return "!";
  return decor[line * 2 + side];
}

#define LEFTBAR(line) do {					\
    printf("%s ", bar(LEFT, line));			        \
  } while (0)

#define RIGHTBAR(line) do {					\
    printf("   %s\n", bar(RIGHT, line));			\
  } while (0)

#define PRINTMODE(field, divisor, units, line) do {		\
    printf(sec ? FMTs : FMT, ROUND1(field, divisor));		\
    printf(" %-3s", units);					\
    printf(GAP);						\
    LEFTBAR(line);						\
  } while (0)

#define PRINTTIME(field, divisor) do {				\
    if ((field) < 0) {						\
      printf("%6s", "   - ");					\
    } else {							\
      printf(sec ? FMTs : FMT, ROUND1(field, divisor));		\
    }								\
    printf(GAP);						\
  } while (0)

#define PRINTTIMENL(field, divisor, line) do {			\
    if ((field) < 0) {						\
      printf("%6s", "   - ");					\
    } else {							\
      printf(sec ? FMTs : FMT, ROUND1(field, divisor));		\
    }								\
    RIGHTBAR(line);						\
  } while (0)

#define PRINTCOUNT(field, divisor, showunits) do {		\
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
    printf(LABEL "    Mode" GAP "  %s    Min   Median      Max   %s\n",
	   "", bar(LEFT, TOPLINE), bar(RIGHT, TOPLINE));
  else
    printf(LABEL "    Mode" GAP "  %s    Min       Q₁   Median       Q₃      Max   %s\n",
	   "", bar(LEFT, TOPLINE), bar(RIGHT, TOPLINE));


  int sec = 1;
  double div = MICROSECS;
  const char *units = "s";
  
  // Decide on which time unit to use for printing the summary
  if (s->total.max < div) {
    sec = 0;
    div = MILLISECS;
    units = "ms";
  }

  printf(LABEL, "Total CPU time");
  PRINTMODE(s->total.mode, div, units, MIDLINE);
  PRINTTIME(s->total.min, div);
  if (briefly) {
    PRINTTIME(s->total.median, div);
  } else {
    PRINTTIME(s->total.Q1, div);
    PRINTTIME(s->total.median, div);
    PRINTTIME(s->total.Q3, div);
  }
  PRINTTIMENL(s->total.max, div, MIDLINE);

  if (!briefly) {
    printf(LABEL, "User time");
    PRINTMODE(s->user.mode, div, units, MIDLINE);
    PRINTTIME(s->user.min, div);
    PRINTTIME(s->user.Q1, div);
    PRINTTIME(s->user.median, div);
    PRINTTIME(s->user.Q3, div);
    PRINTTIMENL(s->user.max, div, MIDLINE);

    printf(LABEL, "System time");
    PRINTMODE(s->system.mode, div, units, MIDLINE);
    PRINTTIME(s->system.min, div);
    PRINTTIME(s->system.Q1, div);
    PRINTTIME(s->system.median, div);
    PRINTTIME(s->system.Q3, div);
    PRINTTIMENL(s->system.max, div, MIDLINE);
  }

  printf(LABEL, "Wall clock");
  PRINTMODE(s->wall.mode, div, units, briefly ? BOTTOMLINE : MIDLINE);
  PRINTTIME(s->wall.min, div);
  if (briefly) {
    PRINTTIME(s->wall.median, div);
  } else {
    PRINTTIME(s->wall.Q1, div);
    PRINTTIME(s->wall.median, div);
    PRINTTIME(s->wall.Q3, div);
  }
  PRINTTIMENL(s->wall.max, div, briefly ? BOTTOMLINE : MIDLINE);

  if (!briefly) {
    div = MEGA;
    units = "MiB";
    // More than 3 digits left of decimal place?
    if (s->maxrss.max >= MEGA * 1000) {
      div *= 1024;
      units = "GiB";
    }
    printf(LABEL, "Max RSS");
    PRINTMODE(s->maxrss.mode, div, units, MIDLINE);
    // Misusing PRINTTIME here because it does the right thing
    PRINTTIME(s->maxrss.min, div);
    PRINTTIME(s->maxrss.Q1, div);
    PRINTTIME(s->maxrss.median, div);
    PRINTTIME(s->maxrss.Q3, div);
    PRINTTIMENL(s->maxrss.max, div, MIDLINE);

    div = 1;
    units = "ct";
    if (s->tcsw.max >= 10000) {
      div = 1000;
      units = "Kct";
    }
    if (s->tcsw.max >= MICROSECS) { // Not a time but
      div = MICROSECS;		    // MICROSECS is a
      units = "Mct";		    // convenient constant
    }
    printf(LABEL, "Context sw");
    PRINTCOUNT(s->tcsw.mode, div, UNITS);
    printf(GAP);
    LEFTBAR(BOTTOMLINE);
    PRINTCOUNT(s->tcsw.min, div, NOUNITS);
    printf(GAP);
    PRINTCOUNT(s->tcsw.Q1, div, NOUNITS);
    printf(GAP);
    PRINTCOUNT(s->tcsw.median, div, NOUNITS);
    printf(GAP);
    PRINTCOUNT(s->tcsw.Q3, div, NOUNITS);
    printf(GAP);
    PRINTCOUNT(s->tcsw.max, div, NOUNITS);
    RIGHTBAR(BOTTOMLINE);

  } // if not brief report

  fflush(stdout);
}

#define BAR "▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭"

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
#define MEDIAN(i) (summaries[i]->total.median)
#define COMMAND(i) (summaries[i]->cmd)

// Returns index into summaries
static int fastest(Summary *summaries[], int start, int end) {
  int best = start;
  int64_t fastest = MEDIAN(best);
  
  // Find the best total time, and also check for lack of data
  for (int i = start; i < end; i++) {
    if (!summaries[i]) return -1; // No data
    if (MEDIAN(i) < fastest) {
      fastest = MEDIAN(i);
      best = i;
    }
  }
  return best;
}

// ASSUMES 'start' and 'end' are valid indexes into 'summaries'
void print_overall_summary(Summary *summaries[], int start, int end) {
  if (!summaries) PANIC_NULL();

  // Need at least two commands in order to have a comparison
  if ((end - start) < 2) {
    // Groups are a feature of commands read from a file (not from
    // the command line)
    if (config.input_filename && (config.action == actionExecute)) {
      printf("Group contains only one command\n");
    }
    return;
  }

  if (!summaries[start]) return;

  int best = fastest(summaries, start, end);

  if ((end - start) > 1) {
    printf("Best guess is:\n");
    printf("  %s ran\n", *COMMAND(best) ? COMMAND(best) : "(empty)");
    for (int i = start; i < end; i++) {
      if (i != best) {
	double factor = (double) MODE(i) / (double) MEDIAN(best);
	printf("  %6.2f times faster than %s\n",
	       factor, *COMMAND(i) ? COMMAND(i) : "(empty)");
      }
    }
  }

  fflush(stdout);
}

// -----------------------------------------------------------------------------
// Box plots
// -----------------------------------------------------------------------------

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

static void print_boxplot_labels(int scale_min, int scale_max, int width) {
  //
  // Print tick mark labels for x-axis
  //
  int ticks = 1 + width / TICKSPACING;
  double incr = (double) (TICKSPACING * (scale_max - scale_min)) / (double) width;
  const char *fmt = (scale_max >= 10) ? "%*.0f%*s" : "%*.1f%*s";
  for (int i = 0; i < ticks; i++) {
    printf(fmt,
	   LABELWIDTH, (double) scale_min + (i * incr),
	   TICKSPACING - LABELWIDTH, "");
  }
  printf("\n");
}

// Tick marks are labeled with as integers with LABELWIDTH digits.
// E.g. when LABELWIDTH is 4, the highest tick mark label is 9999.
// The argument 'width' is the total terminal (or desired) width,
// beyond which we will not print.
static void print_boxplot_scale(int scale_min,
				int scale_max,
				int width,
				int labelplacement) {
  if (width < WIDTHMIN) {
    printf("Requested width (%d) too narrow for plot\n", width);
    return;
  }
  width = width - LABELWIDTH;
  if (labelplacement == BOXPLOT_LABEL_ABOVE)
    print_boxplot_labels(scale_min, scale_max, width);
  //
  // Print axis graphic
  //
  printf("%*s├", (LABELWIDTH - 1), "");
  int currpos = 0;
  int bytesperunit = (uint8_t) AXISLINE[0] >> 6; // Assumes UTF-8
  assert((strlen(AXISLINE) / bytesperunit) > TICKSPACING);

  int ticks = width / TICKSPACING;
  int even = (ticks * TICKSPACING == width);
  for (int i = 0; i < ticks; i++) {
    printf("%.*s%s", (TICKSPACING - 1) * bytesperunit, AXISLINE,
	   even && (i == ticks-1) ? "┤" : "┼");
    currpos += TICKSPACING;
  }
  printf("%.*s", (width - currpos) * bytesperunit, AXISLINE);
  printf("\n");

  if (labelplacement == BOXPLOT_LABEL_BELOW)
    print_boxplot_labels(scale_min, scale_max, width);

}

#define SCALE(val) (round(((double) (val) * scale)))

// The 'index' arg is the command index, 0..N-1 for N commands
static void print_boxplot(int index, 
			  Measures *m,
			  int64_t axismin,
			  int64_t axismax,
			  int width) {
  if (!m) PANIC_NULL();
  if (width < WIDTHMIN) {
    printf("Width %d too narrow for plot\n", width);
    return;
  }
  if ((m->min < axismin) || (m->max > axismax)) {
    printf("Measurement min/max outside of axis min/max values\n");
    return;
  }
  if (axismin >= axismax) PANIC("axis min/max equal or out of order");
  int indent = LABELWIDTH - 1;
  width = width - indent - 1;
  double scale = (double) width / (double) (axismax - axismin);

  int minpos = SCALE(m->min - axismin);
  int boxleft = SCALE(m->Q1 - axismin);
  int median = SCALE(m->median - axismin);
  int boxright = SCALE(m->Q3 - axismin);
  int maxpos = SCALE(m->max - axismin);
  int boxwidth = boxright - boxleft;

  // Will we show the median as its own character?
  int show_median = 1;
  if ((median == boxleft) || (median == boxright))
    show_median = 0;

  if (DEBUG)
    printf("minpos = %d, boxleft = %d, median = %d, "
	   "boxright = %d, maxpos = %d (boxwidth = %d)\n",
	   minpos, boxleft, median, boxright, maxpos, boxwidth);

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
  printf("%-*d", indent, index+1);
  if (minpos == maxpos) {
    printf("%*s", minpos, "");
    printf("┼\n");
  } else {
    printf("%*s", minpos, "");
    if (boxleft > minpos) printf("├"); // "╾"
    for (int j = 1; j < boxleft - minpos; j++) printf("┄");
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
      for (int j = 1; j < median - boxleft; j++) printf(" ");
      if (show_median) printf("│");
      for (int j = 1; j < boxright - median; j++) printf(" ");
      if (median == boxright) printf("╟");
      else printf("├");
    }
    for (int j = 1; j < (maxpos - boxright); j++) printf("┄");
    if ((maxpos - boxright) > 0) printf("┤"); // "╼"
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
#define LBLFMT "%-14s"

static const char *ADscore_repr(Measures *m) {
  char *tmp;
  if (HAS_NONE(m->code)) {
    assert(m->ADscore > 0);
    asprintf(&tmp, "%6.2f", m->ADscore);
    return tmp;
  }
  return ". ";
}

static const char *ADscore_description(Measures *m) {
  char *tmp;
  if (HAS_NONE(m->code)) {
    if (m->p_normal <= 0.05) {
      asprintf(&tmp, "p = %0.3f (NOT normal)", m->p_normal);
      return tmp;
    } else {
      asprintf(&tmp, "p = %0.3f (cannot rule out normal)", m->p_normal);
      return tmp;
    }
  }
  if (HAS(m->code, CODE_LOWVARIANCE))
    return "Very low variance suggests NOT normal";
  if (HAS(m->code, CODE_SMALLN))
    return "Too few data points to measure";
  if (HAS(m->code, CODE_HIGHZ))
    // Approx. 1 observation in a sample of 15,787 will
    // trigger this inability to calculate the ADscore.
    // https://en.wikipedia.org/wiki/68–95–99.7_rule
    return "Extreme values suggest NOT normal";
  return "(not calculated)";
}

static const char *skew_repr(Measures *m) {
  char *tmp;
  if (!HAS(m->code, CODE_LOWVARIANCE) && !HAS(m->code, CODE_SMALLN)) {
    asprintf(&tmp, "%6.2f", m->skew);
  } else {
    return ". ";
  }
  return tmp;
}

static const char *skew_description(Measures *m) {
  char *tmp;
  if (!HAS(m->code, CODE_LOWVARIANCE) && !HAS(m->code, CODE_SMALLN)) {
    asprintf(&tmp, "%s (magnitude %s %0.2f)",
	     (fabs(m->skew) > SKEWSIGNIFICANCE) ? "SIGNIFICANT" : "Insignificant",
	     (fabs(m->skew) > SKEWSIGNIFICANCE) ? ">" : "≤",
	     SKEWSIGNIFICANCE);
    return tmp;
  }
  if (HAS(m->code, CODE_LOWVARIANCE)) {
    return "Variance too low to measure";
  } else if (HAS(m->code, CODE_HIGHZ)) {
    return("Variance too high to measure");
  } else if (HAS(m->code, CODE_SMALLN)) {
    return("Too few data points to measure");
  } else {
    return "(not calculated)";
  }
}

void print_descriptive_stats(Summary *s) {
  if (!s || (s->runs == 0)) return; // No data

  char *tmp = NULL;
  Measures *m = &(s->total);
  int N = s->runs;
  
  int sec = 1;
  double div = MICROSECS;
  
  // Decide on which time unit to use for printing
  if (s->total.max < div) {
    sec = 0;
    div = MILLISECS;
  }

  DisplayTable *t = new_display_table("Total CPU Time",
				      78,
				      3,
				      (int []){16,15,40},
				      (int []){2,1,1},
				      "rrl");
  int row = 0;
  display_table_set(t, row, 0, "");
  row++;

  display_table_set(t, row, 0, "N (observations)");
  display_table_set(t, row, 1, "%6d", N);
  display_table_set(t, row, 2, "ct");
  row++;

  display_table_set(t, row, 0, "Median");
  display_table_set(t, row, 1, (sec ? FMTs : FMT), ROUND1(m->median, div));
  display_table_set(t, row, 2, sec ? "s" : "ms");
  row++;

  int64_t range = m->max - m->min;
  display_table_set(t, row, 0, "Range");
  if (sec)
    asprintf(&tmp, FMTs " … " FMTsL, ROUND1(m->min, div), ROUND1(m->max, div));
  else
    asprintf(&tmp, FMT " … " FMTL, ROUND1(m->min, div), ROUND1(m->max, div));
  display_table_set(t, row, 1, tmp);
  display_table_set(t, row, 2, sec ? "s" : "ms");
  row++;

  display_table_set(t, row, 1, (sec ? FMTs : FMT), ROUND1(range, div));
  display_table_set(t, row, 2, sec ? "s" : "ms");
  row++;

  int64_t IQR = m->Q3 - m->Q1;

  display_table_set(t, row, 0, "IQR");
  if (sec)
    asprintf(&tmp, FMTs " … " FMTsL, ROUND1(m->Q1, div), ROUND1(m->Q3, div));
  else
    asprintf(&tmp, FMT " … " FMTL, ROUND1(m->Q1, div), ROUND1(m->Q3, div));
  display_table_set(t, row, 1, tmp);
  display_table_set(t, row, 2, sec ? "s" : "ms");
  row++;

  display_table_set(t, row, 1, (sec ? FMTs : FMT), ROUND1(IQR, div));
  if (range > 0) 
    display_table_set(t, row, 2, "%-2s (%.1f%% of range)",
		      sec ? "s" : "ms", MS(IQR) * 100.0/ MS(range));
  else
    display_table_set(t, row, 2, sec ? "s" : "ms");
  row++;

  display_table_set(t, row, 0, "");
  row++;
  
  display_table_set(t, row, 0, "Skew");
  display_table_set(t, row, 1, skew_repr(m));
  display_table_set(t, row, 2, skew_description(m));
  row++;

  display_table_set(t, row, 0, "AD normality");
  display_table_set(t, row, 1, ADscore_repr(m));
  display_table_set(t, row, 2, ADscore_description(m));
  row++;

  display_table(t, 2);
  free_display_table(t);

  t = new_display_table("Total CPU Time",
			78,
			8,
			(int []){10,7,7,7,7,7,7,7},
			(int []){2,1,1,1,1,1,1,1}, "rrrrrrrr");
  row = 0;
  display_table_set(t, row, 0, "");
  row++;

  display_table_set(t, row, 0, "Tail shape");
  display_table_set(t, row, 1, "Q₀ ");
  display_table_set(t, row, 2, "Q₁ ");
  display_table_set(t, row, 3, "Q₂ ");
  display_table_set(t, row, 4, "Q₃ ");
  display_table_set(t, row, 5, "95 ");
  display_table_set(t, row, 6, "99 ");
  display_table_set(t, row, 7, "Q₄ ");
  row++;

  display_table_set(t, row, 0, sec ? "(sec)" : "(ms)");
  display_table_set(t, row, 1, sec ? FMTs : FMT, ROUND1(m->min, div));
  display_table_set(t, row, 2, sec ? FMTs : FMT, ROUND1(m->Q1, div));
  display_table_set(t, row, 3, sec ? FMTs : FMT, ROUND1(m->median, div));
  display_table_set(t, row, 4, sec ? FMTs : FMT, ROUND1(m->Q3, div));
  if (m->pct95 < 0)
    display_table_set(t, row, 5,  ". ");
  else 
    display_table_set(t, row, 5, sec ? FMTs : FMT, ROUND1(m->pct95, div));
  if (m->pct99 < 0)
    display_table_set(t, row, 6,  ". ");
  else 
    display_table_set(t, row, 6, sec ? FMTs : FMT, ROUND1(m->pct99, div));
  display_table_set(t, row, 7, sec ? FMTs : FMT, ROUND1(m->max, div));

  display_table(t, 2);
  free_display_table(t);

}

// -----------------------------------------------------------------------------
// Read raw data from CSV files
// -----------------------------------------------------------------------------

// The arg to 'new_usage_array()' is just the initial allocation --
// the array grows dynamically.
#define ESTIMATED_DATA_POINTS 500

Usage *read_input_files(int argc, char **argv) {

  if ((config.first == 0) || (config.first == argc))
    USAGE("No data files to read");
  if (config.first >= MAXDATAFILES)
    USAGE("Too many data files");

  FILE *input[MAXDATAFILES] = {NULL};
  struct Usage *usage = new_usage_array(ESTIMATED_DATA_POINTS);
  size_t buflen = MAXCSVLEN;
  char *buf = malloc(buflen);
  if (!buf) PANIC_OOM();
  char *str;
  int errfield;
  int lineno;
  CSVrow *row;
  int64_t value;
  
  for (int i = config.first; i < argc; i++) {
    input[i] = (strcmp(argv[i], "-") == 0) ? stdin : maybe_open(argv[i], "r");
    if (!input[i]) PANIC_NULL();
    // Skip CSV header
    errfield = read_CSVrow(input[i], &row, buf, buflen);
    if (errfield)
      csv_error(argv[i], lineno, "data", errfield, buf, buflen);
    // Read all the rows
    lineno = 1;
    while (!(errfield = read_CSVrow(input[i], &row, buf, buflen))) {
      lineno++;
      int idx = usage_next(usage);
      str = CSVfield(row, F_CMD);
      if (!str) csv_error(argv[i], lineno, "string", F_CMD+1, buf, buflen);
      str = unescape_csv(str);
      set_string(usage, idx, F_CMD, str);
      free(str);
      str = CSVfield(row, F_SHELL);
      if (!str) csv_error(argv[i], lineno, "string", F_SHELL+1, buf, buflen);
      str = unescape_csv(str);
      set_string(usage, idx, F_SHELL, str);
      free(str);
      // Set all the numeric fields that are measured directly
      for (int fc = F_CODE; fc < F_TOTAL; fc++) {
	str = CSVfield(row, fc);
	if (str && try_strtoint64(str, &value))
	  set_int64(usage, idx, fc, value);
	else
	  csv_error(argv[i], lineno, "integer", fc+1, buf, buflen);
      }
      // Set the fields we calculate for the user
      set_int64(usage, idx, F_TOTAL,
		get_int64(usage, idx, F_USER) + get_int64(usage, idx, F_SYSTEM));
      set_int64(usage, idx, F_TCSW,
		get_int64(usage, idx, F_ICSW) + get_int64(usage, idx, F_VCSW));
      free_CSVrow(row);
    }
    // Check for error reading this particular file (EOF is ok)
    if (errfield > 0)
      csv_error(argv[i], lineno, "data", errfield, buf, buflen);
  }
  free(buf);
  for (int i = config.first; i < argc; i++) fclose(input[i]);
  // Check for no data actually read from any of the files
  if (usage->next == 0) ERROR("No data read.  Exiting...");
  return usage;
}

void print_boxplots(Summary *summaries[], int start, int end) {
  if (!summaries) PANIC_NULL();

  // Check for no data
  if (((end - start) < 1) || (!summaries[start])) {
    printf("  No data for box plot\n");
    return;
  }

  // Establish the axis labels for min/max time
  int64_t axismin = INT64_MAX;
  int64_t axismax = INT64_MIN;
  for (int i = start; i < end; i++) {
    Measures *m = &(summaries[i]->total);
    axismin = min64(m->min, axismin);
    axismax = max64(m->max, axismax);
  }
  int width = config.width;

  // Must ensure that axis min/max have some separation
  if ((axismax - axismin) < width)
    axismax = axismin + width;

  int scale_min = round((double) axismin / 1000.0);
  int scale_max = round((double) axismax / 1000.0);

  print_boxplot_scale(scale_min, scale_max, width, BOXPLOT_LABEL_ABOVE);
  for (int i = start; i < end; i++) {
    print_boxplot(i, &(summaries[i]->total), axismin, axismax, width);
  }
  print_boxplot_scale(scale_min, scale_max, width, BOXPLOT_LABEL_BELOW);
  printf("\n");
  printf("Box plot legend:\n");
  for (int i = start; i < end; i++) {
    printf("  ");
    announce_command(summaries[i]->cmd, i);
  }
  printf("\n");
}

// ASSUMES 'start' and 'end' are valid indexes into 'summaries'
void print_overall_inferences(Summary *summaries[], int start, int end) {
  if (!summaries) PANIC_NULL();

  // Need at least two commands in order to have a comparison
  if ((end - start) < 2) {
    // Groups are a feature of commands read from a file (not from
    // the command line)
    if (config.input_filename && (config.action == actionExecute)) {
      printf("Group contains only one command\n");
    }
    return;
  }

  if (!summaries[start]) return; // No data

  int best = fastest(summaries, start, end);

  printf("Best guess is:\n");
  printf("  %s ran\n", *COMMAND(best) ? COMMAND(best) : "(empty)");
  for (int i = start; i < end; i++) {
    if (i != best) {
      double factor = (double) MODE(i) / (double) MEDIAN(best);
      printf("  %6.2f times faster than %s\n",
	     factor, *COMMAND(i) ? COMMAND(i) : "(empty)");
    }
  }
  fflush(stdout);
}

// TODO: We index into input[] starting at config.first, not 0

// TODO: The CSV reader was not coded for speed.  Could reuse input
// string by replacing commas with NULs.

// TODO: How many summaries do we want to support? MAXCMDS isn't the
// right quantity.

void report(Usage *usage) {

  FILE *csv_output = NULL, *hf_output = NULL;
  if (config.csv_filename) 
    csv_output = maybe_open(config.csv_filename, "w");
  if (config.hf_filename)
    hf_output = maybe_open(config.hf_filename, "w");

  if (csv_output)
    write_summary_header(csv_output);
  if (hf_output)
    write_hf_header(hf_output);

  Summary *s[MAXCMDS];
  int usageidx[MAXCMDS] = {0};
  int count = 0;
  int *next = &(usageidx[1]);
  while ((s[count] = summarize(usage, next))) {
    if (csv_output) 
      write_summary_line(csv_output, s[count]);
    if (hf_output)
      write_hf_line(hf_output, s[count]);

    if (config.report != REPORT_NONE) {
      announce_command(s[count]->cmd, count);
      print_summary(s[count], (config.report == REPORT_BRIEF));
      printf("\n");
    }
    if (config.graph) {
      if (config.report == REPORT_NONE)
	announce_command(s[count]->cmd, count);
      print_graph(s[count], usage, *(next - 1), *next);
      printf("\n");
    }
    if (config.report == REPORT_FULL) {
      print_descriptive_stats(s[count]);
      printf("\n");
    }
    // TODO: Handle "too many commands" better
    next++;
    *next = *(next - 1);
    if (++count == MAXCMDS) USAGE("too many commands");
  }
  if (csv_output) fclose(csv_output);
  if (hf_output) fclose(hf_output);

  if (config.boxplot)
    print_boxplots(s, 0, count);

  print_overall_summary(s, 0, count);
  printf("\n");

  // TEMP: Experimental new features below
  printf("==================================================================\n");

  int best = fastest(s, 0, count);
  Summary *summary1 = s[best];
  Summary *summary2;

  printf("Command                      N   Median    p    p_adj      Diff  Confidence Interval             Â     rbs\n");
  printf("%-25.25s %4d %8" PRId64 "\n",
	 summary1->cmd, summary1->runs, summary1->total.median);

  for (int i = 0; i < count; i++) {
    if (i == best) continue;
    summary2 = s[i];
    RankedCombinedSample RCS =
      rank_difference_magnitude(usage,
				usageidx[best], usageidx[best+1],
				usageidx[i], usageidx[i+1],
				F_TOTAL);

    printf("%-25.25s %4d %8" PRId64,
	   summary2->cmd, summary2->runs, summary2->total.median);

    double W = mann_whitney_w(RCS);
    //printf("Mann-Whitney W (rank sum) = %.0f\n", W);

    // TODO: Should compare dispersions (e.g. IQR) because if they
    // differ a lot, then the W test may not be showing a displacement
    // of the median.

    double alpha = 0.05;

    double adjustedp;
    double p = mann_whitney_p(RCS, W, &adjustedp);
    //printf("Hypothesis: median 1 ≠ median 2\n");
    bool psignificant = p <= (alpha + 0.00005);
    bool adjpsignificant = adjustedp <= (alpha + 0.00005);

    printf("  ");

    //printf("  p                   ");
    if (p < 0.001) printf("<.001");
    else printf("%5.3f", p);
    //printf(" (%ssignificant)", psignificant ? "" : "not ");

    printf("  ");

    //printf("  p adjusted for ties ");
    if (adjustedp < 0.001) printf("<.001");
    else printf("%5.3f", adjustedp);
    //printf(" (%ssignificant)", adjpsignificant ? "" : "not ");

    printf("  ");

    int64_t low, high;
    RankedCombinedSample RCS3 =
      rank_difference_signed(usage,
			     usageidx[i], usageidx[i+1],
			     usageidx[best], usageidx[best+1],
			     F_TOTAL);

    double diff = median_diff_estimate(RCS3);
    //printf("Median difference (Hodges–Lehmann estimate) = %.0f\n", diff);
    printf("%8.0f", diff);

    double confidence = median_diff_ci(RCS3, alpha, &low, &high);
    //printf("  %.2f%% confidence interval (%.0f, %.0f)\n",
    //    confidence * 100.0, (double) low, (double) high);
    char *tmp;
    asprintf(&tmp, "  %4.2f%% (%.0f, %.0f)",
	     confidence * 100.0, (double) low, (double) high);
    printf("%-30.30s", tmp);
    free(tmp);

#if 0
    if ((llabs(low) <= 0.005) || (llabs(high) <= 0.005)) {
      if (psignificant || adjpsignificant)
	printf(" Confidence interval barely includes zero and p is is significant\n");
    } else if ((low <= 0.0) && (high >= 0.0)) {
      if (!psignificant || !adjpsignificant)
	printf(" Confidence interval includes zero and p is not significant");
    } else {
      if (psignificant || adjpsignificant)
	printf(" Confidence interval does not include zero and p is significant");
    }
#endif
    
    double U1 = W - (double) (RCS.n1 * (RCS.n1 + 1)) / 2.0;
    double Ahat = 1.0 - ranked_diff_Ahat(RCS);
//     printf("Â estimates probability of superiority, i.e. that\n"
// 	   "a randomly-chosen observation from the first sample\n"
// 	   "will have a shorter run time than a randomly-chosen\n"
// 	   "observation from the second sample.\n");
//     printf("  Â = %8.3f\n", Ahat);
//     if (fabs(Ahat - 0.5) < 0.100) 
//       printf("  Interpretation: large overlap in distributions, i.e.\n"
// 	     "  roughly equal probability of a random X > a random Y\n");
//     else 
//       printf("  Interpretation: %s chance that a run of \n"
// 	     "  command 1 takes less time than a run of command 2\n",
// 	     (Ahat > 0.5) ? "high" : "low");

    printf("  ");
    printf("%.3f", Ahat);
    
//     printf("A rank biserial correlation of +1 (-1) indicates positive\n"
// 	   "(negative) correlation between the samples, with 0 indicating\n"
// 	   "no correlation at all.\n");
//     printf("  Rank biserial correlation r = %8.3f\n",
// 	   (2.0 * U1 / (double) (RCS.n1 * RCS.n2)) - 1.0);
    printf("  ");
    printf("%0.3f\n", (2.0 * U1 / (double) (RCS.n1 * RCS.n2)) - 1.0);


    printf("\n");

//     printf("\ncdf(1-alpha/2) = cdf(%f) = %8.3f\n",
// 	   1.0 - (alpha / 2.0), inverseCDF(1.0 - (alpha / 2.0)));

//     printf("\ncdf(alpha/2) = cdf(%f) = %8.3f\n",
// 	   (alpha / 2.0), inverseCDF(alpha / 2.0));
    
//     int NQ = runs * 0.5;
//     double alpha = 0.10;
//     int plusminusrank = ci_rank(runs, alpha, 0.5);
//     printf("alpha = %.2f: NQ = %d ± %d == (rank %d, rank %d) ==> (%" PRId64 ", %" PRId64 ")\n",
// 	   alpha,
// 	   NQ, plusminusrank,
// 	   NQ - plusminusrank,
// 	   NQ + plusminusrank,
// 	   RCS.X[RCS.index[NQ - plusminusrank - 1]],
// 	   RCS.X[RCS.index[NQ + plusminusrank - 1]]);


//     alpha = 0.01;
//     low = mann_whitney_ci(RCS, alpha, &high);
//     printf("alpha = %.2f: (%8.3f, %8.3f)\n", alpha, diff - low, diff + high);

    free(RCS.X);
    free(RCS.rank);
    free(RCS.index);
  }


  for (int i = 0; i < count; i++) free_summary(s[i]);
  return;
}
