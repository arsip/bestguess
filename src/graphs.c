//  -*- Mode: C; -*-                                                       
// 
//  graphs.c
// 
//  Copyright (C) Jamie A. Jennings, 2024

#include "bestguess.h"
#include "graphs.h"
#include "utils.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

// -----------------------------------------------------------------------------
// Bar graphs of individual command execution
// -----------------------------------------------------------------------------

// This is a cheap and coarse feature, but a useful one.  We can tell
// at a glance whether warmups are needed, and can estimate how many.
// We can also see whether performance is steady or oscillating, and
// whether the variance is high or low.

#define BAR "▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭"

void print_graph(Summary *s, Usage *usage, int start, int end) {
  // No data if 's' or 'usage' are NULL
  if (!s || !usage) return;
  // Error if start or end is out of range
  if ((start < 0) || (start >= usage->next) || (end < 0) || (end > usage->next))
    PANIC("Usage data indices out of bounds");
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

void maybe_graph(Summary *s, Usage *usage, int start, int end) {
  if (option.graph && usage) {
    print_graph(s, usage, start, end);
    printf("\n");
    fflush(stdout);
  }
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

#define BOXPLOT_NOLABELS 0
#define BOXPLOT_LABEL_ABOVE 1
#define BOXPLOT_LABEL_BELOW 2

#define MODE(i) (summaries[i]->total.mode)
#define MEDIAN(i) (summaries[i]->total.median)
#define COMMAND(i) (summaries[i]->cmd)

// These macros control how the boxplot looks, and are somewhat arbitrary.
#define WIDTHMIN 40
#define TICKSPACING 10
#define LABELWIDTH 4
#define AXISLINE "────────────────────────────────────────────────────────────"

//
// Print tick mark labels (time values) for x-axis
//
// Tick marks are labeled with as integers with LABELWIDTH digits.
// E.g. when LABELWIDTH is 4, the highest tick mark label is 9999.
// The argument 'width' is the total terminal (or desired) width,
// beyond which we will not print.
//
static void print_boxplot_labels(int scale_min, int scale_max, int width) {
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

//
// The scale is the x-axis, and consists of two lines: the top line
// has the x-axis labels (time values), and the bottom line is a
// horizontal line with vertical tick marks.
// 
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
  if (axismin >= axismax) PANIC("Axis min/max equal or out of order");
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
    printf("%*d:", indent-1, index+1);
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

void print_boxplots(Summary *summaries[], int start, int end) {
  if (!summaries) PANIC_NULL();

  // Check for no data
  if (((end - start) < 1) || (!summaries[start])) {
    printf("No data for box plot\n");
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
    // Maybe limit announcement length to width - 2 ?
    char *tmp = command_announcement(summaries[i]->cmd, i, "%d: %s", NOLIMIT);
    printf("%s\n", tmp);
    free(tmp);
  }
  puts("");
  fflush(stdout);
}

void maybe_boxplots(Ranking *ranking) {
  if (option.boxplot)
    print_boxplots(ranking->summaries, 0, ranking->count);
}

