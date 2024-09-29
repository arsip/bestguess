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
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#define SECOND(a, b, c) b,
const char *ReportOptionName[] = {XReports(SECOND)};
#undef SECOND
#define THIRD(a, b, c) c,
const char *ReportOptionDesc[] = {XReports(THIRD)};
#undef THIRD

static char *report_option_string = NULL;

#define BOXPLOT_NOLABELS 0
#define BOXPLOT_LABEL_ABOVE 1
#define BOXPLOT_LABEL_BELOW 2

// For printing program help.  Caller must free the returned string.
char *report_options(void) {
  if (report_option_string) return report_option_string;
  size_t bufsize = 1000;
  char *buf = malloc(bufsize);
  if (!buf) PANIC_OOM();
  int len = snprintf(buf, bufsize, "Valid report types are");
  for (ReportCode i = REPORT_NONE; i < REPORT_ERROR; i++) {
    bufsize -= len;
    len += snprintf(buf + len, bufsize, "\n  %-8s %s",
		    ReportOptionName[i], ReportOptionDesc[i]);
  }
  report_option_string = buf;
  return report_option_string;
}

void free_report_options(void) {
  free(report_option_string);
}

ReportCode interpret_report_option(const char *op) {
  for (ReportCode i = REPORT_NONE; i < REPORT_ERROR; i++)
    if (strcmp(op, ReportOptionName[i]) == 0) return i;
  return REPORT_ERROR;
}

// 'len' is the desired length of the returned string.
void announce_command(const char *cmd, int index, const char *fmt, int len) {
  char *tmp;
  asprintf(&tmp, fmt, index + 1, *cmd ? cmd : "(empty)");
  if (len == NOLIMIT)
    printf("%s", tmp);
  else
    printf("%-*.*s", len, len, tmp);
  free(tmp);
}

// static void announce_command_short(const char *cmd, int index, int width, int precision) {
//   printf("#%d: %*.*s", index + 1, width, precision, *cmd ? cmd : "(empty)");
// }

#define FMT "%6.1f"
#define FMTL "%-.1f"
#define FMTs "%6.2f"
#define FMTsL "%-.2f"
#define IFMT "%6" PRId64
#define IFMTL "%-6" PRId64
#define LABEL "  %15s "
#define GAP   "  "

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

#define PRINTMODE(field, units, line) do {			\
    tmp = apply_units(field, units, UNITS);			\
    printf(NUMFMT, tmp);					\
    free(tmp);							\
    printf(GAP);						\
    LEFTBAR(line);						\
  } while (0)

#define PRINTTIME(field, units) do {				\
    if ((field) < 0) {						\
      printf(NUMFMT_NOUNITS, "   - ");				\
    } else {							\
      tmp = apply_units(field, units, NOUNITS);			\
      printf(NUMFMT_NOUNITS, tmp);				\
      free(tmp);						\
    }								\
    printf(GAP);						\
  } while (0)

#define PRINTTIMENL(field, units, line) do {			\
    if ((field) < 0) {						\
      printf(NUMFMT_NOUNITS, "   - ");				\
    } else {							\
      tmp = apply_units(field, units, NOUNITS);			\
      printf(NUMFMT_NOUNITS, tmp);				\
      free(tmp);						\
    }								\
    RIGHTBAR(line);						\
  } while (0)

#define PRINTCOUNT(field, units, showunits) do {		\
    if ((field) < 0)						\
      printf(showunits ? NUMFMT : NUMFMT_NOUNITS, "   - ");	\
    else {							\
      tmp = apply_units(field, units, showunits);		\
      printf(showunits ? NUMFMT : NUMFMT_NOUNITS, tmp);		\
      free(tmp);						\
    }								\
  } while (0)

void print_summary(Summary *s, bool briefly) {
  if (!s) {
    printf("  No data\n");
    return;
  }

  if (briefly)
    printf(LABEL "    Mode" GAP "  %s     Min   Median      Max   %s\n",
	   "", bar(LEFT, TOPLINE), bar(RIGHT, TOPLINE));
  else
    printf(LABEL "    Mode" GAP "  %s     Min      Q₁    Median      Q₃       Max   %s\n",
	   "", bar(LEFT, TOPLINE), bar(RIGHT, TOPLINE));


  char *tmp;
  Units *units = select_units(s->total.max, time_units);

  printf(LABEL, "Total CPU time");
  PRINTMODE(s->total.mode, units, MIDLINE);
  PRINTTIME(s->total.min, units);
  if (briefly) {
    PRINTTIME(s->total.median, units);
  } else {
    PRINTTIME(s->total.Q1, units);
    PRINTTIME(s->total.median, units);
    PRINTTIME(s->total.Q3, units);
  }
  PRINTTIMENL(s->total.max, units, MIDLINE);

  if (!briefly) {
    printf(LABEL, "User time");
    PRINTMODE(s->user.mode, units, MIDLINE);
    PRINTTIME(s->user.min, units);
    PRINTTIME(s->user.Q1, units);
    PRINTTIME(s->user.median, units);
    PRINTTIME(s->user.Q3, units);
    PRINTTIMENL(s->user.max, units, MIDLINE);

    printf(LABEL, "System time");
    PRINTMODE(s->system.mode, units, MIDLINE);
    PRINTTIME(s->system.min, units);
    PRINTTIME(s->system.Q1, units);
    PRINTTIME(s->system.median, units);
    PRINTTIME(s->system.Q3, units);
    PRINTTIMENL(s->system.max, units, MIDLINE);
  }

  printf(LABEL, "Wall clock");
  PRINTMODE(s->wall.mode, units, briefly ? BOTTOMLINE : MIDLINE);
  PRINTTIME(s->wall.min, units);
  if (briefly) {
    PRINTTIME(s->wall.median, units);
  } else {
    PRINTTIME(s->wall.Q1, units);
    PRINTTIME(s->wall.median, units);
    PRINTTIME(s->wall.Q3, units);
  }
  PRINTTIMENL(s->wall.max, units, briefly ? BOTTOMLINE : MIDLINE);

  if (!briefly) {
    units = select_units(s->maxrss.max, space_units);
    printf(LABEL, "Max RSS");
    PRINTMODE(s->maxrss.mode, units, MIDLINE);
    // Misusing PRINTTIME here because it does the right thing
    PRINTTIME(s->maxrss.min, units);
    PRINTTIME(s->maxrss.Q1, units);
    PRINTTIME(s->maxrss.median, units);
    PRINTTIME(s->maxrss.Q3, units);
    PRINTTIMENL(s->maxrss.max, units, MIDLINE);

    units = select_units(s->tcsw.max, count_units);
    printf(LABEL, "Context sw");
    PRINTCOUNT(s->tcsw.mode, units, UNITS);
    printf(GAP);
    LEFTBAR(BOTTOMLINE);
    PRINTCOUNT(s->tcsw.min, units, NOUNITS);
    printf(GAP);
    PRINTCOUNT(s->tcsw.Q1, units, NOUNITS);
    printf(GAP);
    PRINTCOUNT(s->tcsw.median, units, NOUNITS);
    printf(GAP);
    PRINTCOUNT(s->tcsw.Q3, units, NOUNITS);
    printf(GAP);
    PRINTCOUNT(s->tcsw.max, units, NOUNITS);
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

#if 0
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
#endif

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
  if ((end - start) <= 1) return;
  
  int *index = sort_by_totaltime(summaries, start, end);
  int bestidx = index[0];

  printf("Best guess is:\n");
  printf("  ");
  announce_command(COMMAND(bestidx), bestidx, "#%d: %s", NOLIMIT);
  printf(" ran\n");
  for (int i = 1; i < (end - start); i++) {
    double factor = (double) MEDIAN(index[i]) / (double) MEDIAN(bestidx);
    printf("  %6.2f times faster than ", factor);
    announce_command(COMMAND(index[i]), index[i], "#%d: %s", NOLIMIT);
    puts("");
  }

  free(index);
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
  if (index+1 < 100)
    printf("#%-*d", indent-1, index+1);
  else
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

// These are judgement calls: thresholds that are used only to produce
// an informative (not authoritative) statement in a report.
//
// https://www.ncbi.nlm.nih.gov/pmc/articles/PMC3591587/
// 
// Above article cites the paper below for considering skew values of
// magnitude 2.0 or larger and kurtosis magnitude larger than 4.0 to
// indicate substantial departure from normal:
//
// West SG, Finch JF, Curran PJ. Structural equation models with
// nonnormal variables: problems and remedies. In: Hoyle RH,
// editor. Structural equation modeling: Concepts, issues and
// applications. Newbery Park, CA: Sage; 1995. pp. 56–75.

#define ROUND1(intval, divisor) \
  (round((double)((intval) * 10) / (divisor)) / 10.0)
#define MS(nanoseconds) (ROUND1(nanoseconds, 1000.0))
#define MSFMT "%8.1fms"
#define COLSEP "  "
#define INDENT "  "
#define LBLFMT "%-14s"

static bool have_valid_ADscore(Measures *m) {
  return (!HAS(m->code, CODE_HIGHZ)
	  && !HAS(m->code, CODE_SMALLN)
	  && !HAS(m->code, CODE_LOWVARIANCE));
}

// Caller must free returned string
static const char *ADscore_repr(Measures *m) {
  char *tmp;
  if (have_valid_ADscore(m)) {
    asprintf(&tmp, "%6.2f", m->ADscore);
    return tmp;
  }
  return ". ";
}

// Caller must free returned string
static const char *ADscore_description(Measures *m) {
  char *tmp;
  if (have_valid_ADscore(m)) {
    if (m->p_normal <= config.alpha) {
      if (m->p_normal < 0.001)
	asprintf(&tmp, "p < 0.001 (signif., α = %4.2f) Not normal",
		 config.alpha);
      else
	asprintf(&tmp, "p = %5.3f (signif., α = %4.2f) Not normal",
		 m->p_normal, config.alpha);
      return tmp;
    } else {
      asprintf(&tmp, "p = %5.3f (non-signif., α = %4.2f) Cannot rule out normal",
	       m->p_normal, config.alpha);
      return tmp;
    }
  }
  if (HAS(m->code, CODE_LOWVARIANCE))
    return "Very low variance suggests NOT normal";
  if (HAS(m->code, CODE_SMALLN))
    return "Too few data points to measure";
  if (HAS(m->code, CODE_HIGHZ)) {
    // Approx. 1 observation in a sample of 390 BILLION will trigger
    // this situation if the sample really is normally distributed.
    // https://en.wikipedia.org/wiki/68–95–99.7_rule
    asprintf(&tmp, "Extreme values (Z ≈ %0.1f): not normal", m->ADscore);
    return tmp;
  }
  return "(not calculated)";
}

// Caller must free returned string
static const char *skew_repr(Measures *m) {
  char *tmp;
  if (!HAS(m->code, CODE_LOWVARIANCE) && !HAS(m->code, CODE_SMALLN)) {
    asprintf(&tmp, "%6.2f", m->skew);
  } else {
    return ". ";
  }
  return tmp;
}

// Caller must free returned string
static const char *skew_description(Measures *m) {
  char *tmp;
  if (!HAS(m->code, CODE_LOWVARIANCE) && !HAS(m->code, CODE_SMALLN)) {
    asprintf(&tmp, "%s",
	     HAS(m->code, CODE_HIGH_SKEW)
	     ? "Substantial deviation from normal"
	     : "Non-significant");
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

// Caller must free returned string
static const char *kurtosis_repr(Measures *m) {
  char *tmp;
  if (!HAS(m->code, CODE_SMALLN)) {
    asprintf(&tmp, "%6.2f", m->kurtosis);
  } else {
    return ". ";
  }
  return tmp;
}

// Caller must free returned string
static const char *kurtosis_description(Measures *m) {
  char *tmp;
  if (!HAS(m->code, CODE_SMALLN)) {
    asprintf(&tmp, "%s",
	     HAS(m->code, CODE_HIGH_SKEW)
	     ? "Substantial deviation from normal"
	     : "Non-significant");
    return tmp;
  } else {
    return("Too few data points to measure");
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
  
  display_table_set(t, row, 0, "AD normality");
  display_table_set(t, row, 1, ADscore_repr(m));
  display_table_set(t, row, 2, ADscore_description(m));
  row++;

  display_table_set(t, row, 0, "Skew");
  display_table_set(t, row, 1, skew_repr(m));
  display_table_set(t, row, 2, skew_description(m));
  row++;

  display_table_set(t, row, 0, "Excess kurtosis");
  display_table_set(t, row, 1, kurtosis_repr(m));
  display_table_set(t, row, 2, kurtosis_description(m));
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
  CSVrow *row;
  int64_t value;
  int errfield;
  int lineno = 1;
  
  for (int i = config.first; i < argc; i++) {
    input[i] = (strcmp(argv[i], "-") == 0) ? stdin : maybe_open(argv[i], "r");
    if (!input[i]) PANIC_NULL();
    // Skip CSV header
    errfield = read_CSVrow(input[i], &row, buf, buflen);
    free_CSVrow(row);
    if (errfield)
      csv_error(argv[i], lineno, "data", errfield, buf, buflen);
    // Read all the rows
    lineno = 1;
    while (!(errfield = read_CSVrow(input[i], &row, buf, buflen))) {
      lineno++;
      int idx = usage_next(usage);
      str = CSVfield(row, F_CMD);
      if (!str)
	csv_error(argv[i], lineno, "string", F_CMD+1, buf, buflen);
      str = unescape_csv(str);
      set_string(usage, idx, F_CMD, str);
      free(str);
      str = CSVfield(row, F_SHELL);
      if (!str)
	csv_error(argv[i], lineno, "string", F_SHELL+1, buf, buflen);
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
      csv_error(argv[i], lineno + 1, "data", errfield, buf, buflen);
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
    announce_command(summaries[i]->cmd, i, "#%d: %s", NOLIMIT); // width - 2
    puts("");
  }
  puts("");
}

// TODO: We index into input[] starting at config.first, not 0

// TODO: The CSV reader was not coded for speed.  Could reuse input
// string by replacing commas with NULs.

// TODO: How many summaries do we want to support? MAXCMDS isn't the
// right quantity.

static void print_sample(Summary *s, int num, double best_median) {
  Units *units;
  char *tmp, *tmp2, *tmp3;

  announce_command(s->cmd, num, "#%d: %s", 20);
  units = select_units(s->total.max, time_units);
  tmp = apply_units(s->total.median, units, UNITS);
  printf(" %4d " NUMFMT, s->runs, tmp);
  free(tmp);

  Inference *stat = s->infer;
  if (!stat) {
    // We are printing the best performer (no inferential stats)
    printf("\n");
    return;
  }

  printf("  ");
  printf("%8.0f", stat->W);

  printf("  ");
  if (stat->p < 0.001) printf("<.001");
  else printf("%5.3f", stat->p);

  printf("  ");
  // Adjusted for ties
  if (stat->p_adj < 0.001) printf("<.001");
  else printf("%5.3f", stat->p_adj);

  printf("  ");

  tmp = apply_units(stat->shift, units, UNITS);
  printf(NUMFMT, tmp);
  free(tmp);

  if (best_median != -1.0) {
    double pct_shift = 100.0 * stat->shift / best_median;
    printf(" (%3.f%%)", pct_shift);
  } else {
    printf("       ");
  }
  
  printf("  %4.2f%% ", stat->confidence * 100.0);
  tmp = apply_units(stat->ci_low, units, NOUNITS);
  tmp2 = apply_units(stat->ci_high, units, NOUNITS);
  asprintf(&tmp3, "(%s, %s) %2s", lefttrim(tmp), lefttrim(tmp2), units->unitname);
  printf("%-20s", tmp3);
  free(tmp);
  free(tmp2);
  free(tmp3);
    
  printf("  ");
  printf("%.3f", stat->p_super);
    
  printf("  ");

  if (HAS(stat->indistinct, INF_NONSIG)) printf("p");
  if (HAS(stat->indistinct, INF_CIZERO)) printf("0");
  if (HAS(stat->indistinct, INF_NOEFFECT)) printf("Δ");
  if (HAS(stat->indistinct, INF_HIGHSUPER)) printf("↑");

  printf("\n");
}

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
      printf("\n\n");
      print_summary(s[count], (config.report == REPORT_BRIEF));
      printf("\n");
    }
    if (config.graph) {
      if (config.report == REPORT_NONE) {
	announce_command(s[count]->cmd, count, "Command #%d: %s", NOLIMIT);
	printf("\n");
      }
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

  // Number of samples
  int N = count;

  if (N < 2) {
    printf("Only one command.  Not printing experimental new info.\n");
    goto done1;
  }

  // Rank the samples and identify the best performer
  int *index = sort_by_totaltime(s, 0, N);
  int bestidx = index[0];

  if (s[bestidx]->runs < INFERENCE_N_THRESHOLD) {
    printf("Minimum observations must be at least %d for this analysis.\n",
	   INFERENCE_N_THRESHOLD);
    goto done2;
  }

  // Compare each sample to the best performer
  for (int i = 0; i < N; i++) {
    if (index[i] == bestidx) continue;
    s[index[i]]->infer =
      compare_samples(usage,
		      config.alpha,
		      usageidx[bestidx], usageidx[bestidx+1],
		      usageidx[index[i]], usageidx[index[i]+1]);
  }

  // Take all the samples indistinguishable from the best performer
  // and group them (in rank order) in 'same' so they can be printed
  // together.
  int *same = malloc(N * sizeof(int));
  if (!same) PANIC_NULL();
  same[0] = bestidx;
  int same_count = 1;
  for (int i = 1; i < N; i++) 
    if (s[index[i]]->infer->indistinct) {
      same[i] = index[i];
      index[i] = -1;
      same_count++;
    } else {
      same[i] = -1;
    }

  printf("Command                 N     Median      W      p    p_adj      Shift           Confidence Interval         Â \n");

  // Print the 'same' array of samples
  for (int i = 0; i < N; i++)
    if (same[i] != -1) 
      print_sample(s[same[i]], same[i], -1);

  printf("------\n");

  // Print the rest
  for (int i = 0; i < N; i++)
    if ((index[i] != -1) && (index[i] != bestidx))
      print_sample(s[index[i]], index[i], s[bestidx]->total.median);
  
  char *tmp;
  printf("\n");

  /* Maybe we should draw a table like this?

    Best guess ranking:
    ╔═════ Command ══════════════════════════ Total time ══ Slower by Δ ═══╗
    ║✓  4. rosie match -o line 'find:{[0-9]{    32.03 ms                   ║
    ║                                                                      ║
    ║   1. rosie match -o line 'find:{[0-9]{    32.55 ms    0.50 ms   +2%  ║
    ║   3. rosie match -o line 'find:{[0-9]{    32.73 ms    0.72 ms   +2%  ║
    ║   2. rosie match -o line 'find:[0-9]{5    35.66 ms    3.61 ms  +11%  ║
    ╚══════════════════════════════════════════════════════════════════════╝

  */     

  const char *fill = "═";
  const int b = strlen(fill);

  const int cmd_nonfillchars = 9;
  const int cmd_width = 38;
  const int cmd_len = cmd_nonfillchars + (cmd_width - cmd_nonfillchars) * b;
  const char *cmd_header = "═════ Command ═════════════════════════════════════════════";

  const int time_width = 16;
  const int time_nonfillchars = 12;
  const int time_len = time_nonfillchars + (time_width - time_nonfillchars) * b;
  const char *time_header = "══ Total time ══";
  const int delta_width = 18;
  const int delta_nonfillchars = 12;
  const int delta_len = (delta_nonfillchars
			 + (delta_width - 1 - delta_nonfillchars) * b
			 + strlen("Δ"));
  const char *delta_header = " Slower by Δ ═════════════════";
			
  const char *winner = "✓";
  const char *cmd_fmt = "%3d. %s";

  if (same_count > 1) 
    printf("Best guess ranking: (the top %d commands performed identically)\n",
	   same_count);
  else
    printf("Best guess ranking:\n");

  printf("\n");
  printf("%*.*s%*.*s", cmd_len, cmd_len, cmd_header, time_len, time_len, time_header);
  if (same_count < N)
    printf("%*.*s\n", delta_len, delta_len, delta_header);
  else
    printf("\n");
  
  for (int i = 0; i < N; i++)
    if (same[i] != -1) {
      printf("%s", winner);
      Summary *ss = s[same[i]];
      Units *units = select_units(ss->total.max, time_units);
      announce_command(ss->cmd, same[i], cmd_fmt, cmd_width);
      units = select_units(ss->total.max, time_units);
      tmp = apply_units(ss->total.median, units, UNITS);
      printf("  %s\n", tmp);
      free(tmp);
    }

  // Print the rest
  for (int i = 0; i < N; i++)
    if ((index[i] != -1) && (index[i] != bestidx)) {
      printf(" ");
      Summary *ss = s[index[i]];
      Units *units = select_units(ss->total.max, time_units);
      announce_command(ss->cmd, index[i], cmd_fmt, cmd_width);
      units = select_units(ss->total.max, time_units);
      tmp = apply_units(ss->total.median, units, UNITS);
      printf("  %s ", tmp);
      free(tmp);
      tmp = apply_units(ss->infer->shift, units, UNITS);
      printf(NUMFMT, tmp);
      free(tmp);
      double factor = (double) ss->infer->shift / s[bestidx]->total.median;
      printf(" %7.2fx", factor);
      printf("\n");
    }

  printf("%.*s\n", (cmd_width + time_width + delta_width) * b,
	 "═══════════════════════════════════════════════════════════" 
	 "═══════════════════════════════════════════════════════════");


  free(same);
 done2:
  free(index);
 done1:
  for (int i = 0; i < N; i++) free_summary(s[i]);
  return;
}
