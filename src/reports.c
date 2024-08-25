//  -*- Mode: C; -*-                                                       
// 
//  reports.c
// 
//  Copyright (C) Jamie A. Jennings, 2024

#include "bestguess.h"
#include "reports.h"
#include "utils.h"
#include "csv.h"
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

ReportCode interpret_report_option(const char *op) {
  for (ReportCode i = REPORT_NONE; i < REPORT_ERROR; i++)
    if (strcmp(op, ReportOptionName[i]) == 0) return i;
  return REPORT_ERROR;
}

// Caller must the returned string
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

void announce_command(const char *cmd, int index) {
  printf("Command %d: %s\n", index + 1, *cmd ? cmd : "(empty)");
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

#define PRINTMODE(field, briefly) do {				\
    printf(sec ? FMTs : FMT, ROUND1(field, divisor));		\
    printf(" %-3s", units);					\
    printf(GAP);						\
    LEFTBAR(briefly);						\
  } while (0)

#define PRINTTIME(field) do {					\
    if ((field) < 0) {						\
      printf("%6s", "   - ");					\
    } else {							\
      printf(sec ? FMTs : FMT, ROUND1(field, divisor));		\
    }								\
    printf(GAP);						\
  } while (0)

#define PRINTTIMENL(field, briefly) do {			\
    if ((field) < 0) {						\
      printf("%6s", "   - ");					\
    } else {							\
      printf(sec ? FMTs : FMT, ROUND1(field, divisor));		\
    }								\
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

  } // if not brief report

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
    if (config.input_filename && (config.action == actionExecute)) {
      printf("Command group contains only one command\n");
    }
    return;
  }

  if (!summaries[start]) return;

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

// static void print_boxplot_command(const char *cmd, int index, int width) {
//   printf("  %*d: %.*s\n",
// 	 LABELWIDTH, index + 1,
// 	 width - LABELWIDTH - 3, *cmd ? cmd : "(empty)");
// }

static void print_boxplot_labels(int axismin, int axismax, int width) {
  //
  // Print tick mark labels for x-axis
  //
  width = width - (LABELWIDTH - 1);
  int ticks = 1 + width / TICKSPACING;
  double incr = (double) (TICKSPACING * (axismax - axismin)) / (double) width;
  const char *fmt = (axismax >= 10) ? "%*.0f%*s" : "%*.1f%*s";
  for (int i = 0; i < ticks; i++) {
    printf(fmt,
	   LABELWIDTH, (double) axismin + (i * incr),
	   TICKSPACING - LABELWIDTH, "");
  }
  printf("\n");
}

// Tick marks are labeled with as integers with LABELWIDTH digits.
// E.g. when LABELWIDTH is 4, the highest tick mark label is 9999.
// The argument 'width' is the total terminal (or desired) width,
// beyond which we will not print.
static void print_boxplot_scale(int axismin,
				int axismax,
				int width,
				int labelplacement) {
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

  int ticks = width / TICKSPACING;
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
static void print_ticks(Measures *m, int width) {
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

static void print_boxplot(int index, // Which command is this? 
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
    printf("Measurement min/max outside of axis min/max values");
    return;
  }
  if (axismin >= axismax) PANIC("axis min/max equal or out of order");
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
  printf("%-*d", indent, index+1);
  if (minpos == maxpos) {
    printf("%*s", minpos, "");
    printf("┼\n");
  } else {
    printf("%*s", minpos, "");
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

// TODO: Scale the values if too many ms?
void print_descriptive_stats(Summary *s) {
  if (!s || (s->runs == 0)) return; // No data
  Measures *m = &(s->total);
  int n = s->runs;
  
  printf(INDENT "About the distribution of total CPU time:\n\n");

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
  printf(INDENT "Inter-quartile range = %0.2fms (%0.2f%% of total range)\n",
	 MS(IQR), (MS(IQR) * 100.0/ MS(range)));

  // How far below and above the median
  int64_t IQ1delta = m->median - m->Q1;
  int64_t IQ3delta = m->Q3 - m->median;
  printf(INDENT "Relative to the median, the IQR is [-%0.1fms, +%0.1fms]\n",
	 MS(IQ1delta), MS(IQ3delta));

  if (m->code == 0) {
    if (m->ADscore > 0)
      printf(INDENT "Distribution %s normal (AD score %4.2f) with p = %6.4f\n",
	     (m->p_normal <= 0.05) ? "is NOT" : "could be",
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
  CSVrow *row;
  int64_t value;
  
  for (int i = config.first; i < argc; i++) {
    input[i] = (strcmp(argv[i], "-") == 0) ? stdin : maybe_open(argv[i], "r");
    if (!input[i]) PANIC_NULL();
    // Skip CSV header
    row = read_CSVrow(input[i], buf, buflen);
    if (!row) ERROR("No data read from file %s", argv[i]);
    // Read all the rows
    while ((row = read_CSVrow(input[i], buf, buflen))) {
      int idx = usage_next(usage);
      set_string(usage, idx, F_CMD, CSVfield(row, F_CMD));
      set_string(usage, idx, F_SHELL, CSVfield(row, F_SHELL));
      // Set all the numeric fields that are measured directly
      for (int fc = F_CODE; fc < F_TOTAL; fc++) {
	if (try_strtoint64(CSVfield(row, fc), &value))
	  set_int64(usage, idx, fc, value);
	else
	  USAGE("CSV read error looking for integer in field %d\n"
		"Line: %s\n", fc+1, buf);
      }
      // Set the fields we calculate for the user
      set_int64(usage, idx, F_TOTAL,
		get_int64(usage, idx, F_USER) + get_int64(usage, idx, F_SYSTEM));
      set_int64(usage, idx, F_TCSW,
		get_int64(usage, idx, F_ICSW) + get_int64(usage, idx, F_VCSW));
      free_CSVrow(row);
    }
  }
  free(buf);
  for (int i = config.first; i < argc; i++) fclose(input[i]);
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
    //print_boxplot_command(summaries[i]->cmd, i, width);
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
  int next = 0;
  int count = 0;
  int prev = 0;
  while ((s[count] = summarize(usage, &next))) {
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
      print_graph(s[count], usage, prev, next);
      printf("\n");
    }
    if (config.report == REPORT_FULL) {
      print_descriptive_stats(s[count]);
      printf("\n");
    }
    prev = next;
    if (++count == MAXCMDS) USAGE("too many commands");
  }
  if (csv_output) fclose(csv_output);
  if (hf_output) fclose(hf_output);

  if (config.boxplot)
    print_boxplots(s, 0, count);

  print_overall_summary(s, 0, count);

  for (int i = 0; i < count; i++) free_summary(s[i]);
  printf("\n");

#if 0
  Measures m;
  m.min = 5;
  m.Q1 = 10;
  m.median = 13;
  m.Q3 = 16;
  m.max = 20;

  printf("min/Q1/med/Q3/max: %d %d %d %d %d\n", (int) m.min, (int) m.Q1, (int) m.median, (int) m.Q3, (int) m.max);
  print_boxplot_scale(0, width-3, width, BOXPLOT_LABEL_ABOVE);
  //  print_ticks(m.min, m.Q1, m.median, m.Q3, m.max, width);
  print_boxplot(&m, 0, width-3, width);
  print_boxplot_scale(0, width-3, width, BOXPLOT_LABEL_BELOW);

  m.min = 3;
  m.Q1 = 5;
  m.median = 10;
  m.Q3 = 13;
  m.max = 20;
  printf("min/Q1/med/Q3/max: %d %d %d %d %d\n", (int) m.min, (int) m.Q1, (int) m.median, (int) m.Q3, (int) m.max);
  //  print_ticks(m.min, m.Q1, m.median, m.Q3, m.max, width);
  print_boxplot(&m, 0, width-3, width);
  print_boxplot_scale(0, width-3, width, BOXPLOT_LABEL_BELOW);

  m.min = 3;
  m.Q1 = 5;
  m.median = 5;
  m.Q3 = 13;
  m.max = 25;
  printf("min/Q1/med/Q3/max: %d %d %d %d %d\n", (int) m.min, (int) m.Q1, (int) m.median, (int) m.Q3, (int) m.max);
  //  print_ticks(m.min, m.Q1, m.median, m.Q3, m.max, width);
  print_boxplot(&m, 0, width-3, width);
  print_boxplot_scale(0, width-3, width, BOXPLOT_LABEL_BELOW);

  m.min = 3;
  m.Q1 = 5;
  m.median = 13;
  m.Q3 = 13;
  m.max = 25;
  printf("min/Q1/med/Q3/max: %d %d %d %d %d\n", (int) m.min, (int) m.Q1, (int) m.median, (int) m.Q3, (int) m.max);
  //  print_ticks(m.min, m.Q1, m.median, m.Q3, m.max, width);
  print_boxplot(&m, 0, width-3, width);
  print_boxplot_scale(0, width-3, width, BOXPLOT_LABEL_BELOW);

  m.min = 0;
  m.Q1 = 5;
  m.median = 13;
  m.Q3 = 13;
  m.max = 25;
  printf("min/Q1/med/Q3/max: %d %d %d %d %d\n", (int) m.min, (int) m.Q1, (int) m.median, (int) m.Q3, (int) m.max);
  //  print_ticks(m.min, m.Q1, m.median, m.Q3, m.max, width);
  print_boxplot(&m, 0, width-3, width);
  print_boxplot_scale(0, width-3, width, BOXPLOT_LABEL_BELOW);
  
  m.min = 0;
  m.Q1 = 5;
  m.median = 13;
  m.Q3 = 13;
  m.max = 30;
  printf("min/Q1/med/Q3/max: %d %d %d %d %d\n", (int) m.min, (int) m.Q1, (int) m.median, (int) m.Q3, (int) m.max);
  //  print_ticks(m.min, m.Q1, m.median, m.Q3, m.max, width);
  print_boxplot(&m, 0, width-3, width);
  print_boxplot_scale(0, width-3, width, BOXPLOT_LABEL_BELOW);
  
  m.min = 0;
  m.Q1 = 5;
  m.median = 13;
  m.Q3 = 13;
  m.max = 80;
  printf("min/Q1/med/Q3/max: %d %d %d %d %d\n", (int) m.min, (int) m.Q1, (int) m.median, (int) m.Q3, (int) m.max);
  //  print_ticks(m.min, m.Q1, m.median, m.Q3, m.max, width);
  print_boxplot(&m, 0, width-3, width);
  print_boxplot_scale(0, width-3, width, BOXPLOT_LABEL_BELOW);
#endif
  
  return;
}
