//  -*- Mode: C; -*-                                                       
// 
//  reports.c
// 
//  Copyright (C) Jamie A. Jennings, 2024

#include "bestguess.h"
#include "reports.h"
#include "utils.h"
#include "csv.h"
#include "graphs.h"
#include "printing.h"
#include "cli.h"		// To print hint on changing config settings
#include "optable.h"		// To print hint on changing config settings
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

static char *report_help_string = NULL;

// For printing program help.  Caller must free the returned string.
char *report_help(void) {
  if (report_help_string) return report_help_string;
  size_t bufsize = 1000;
  char *buf = malloc(bufsize);
  if (!buf) PANIC_OOM();
  int len = snprintf(buf, bufsize, "Valid report types are");
  for (ReportCode i = REPORT_NONE; i < REPORT_ERROR; i++) {
    bufsize -= len;
    len += snprintf(buf + len, bufsize, "\n  %-8s %s",
		    ReportOptionName[i], ReportOptionDesc[i]);
  }
  report_help_string = buf;
  return report_help_string;
}

void free_report_help(void) {
  free(report_help_string);
}

ReportCode interpret_report_option(const char *op) {
  for (ReportCode i = REPORT_NONE; i < REPORT_ERROR; i++)
    if (strcmp(op, ReportOptionName[i]) == 0) return i;
  return REPORT_ERROR;
}

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

static bool have_valid_ADscore(Measures *m) {
  return (!HAS(m->code, CODE_HIGHZ)
	  && !HAS(m->code, CODE_SMALLN)
	  && !HAS(m->code, CODE_LOWVARIANCE));
}

// Caller must free returned string
static const char *ADscore_repr(Measures *m) {
  char *tmp;
  if (have_valid_ADscore(m)) {
    ASPRINTF(&tmp, "%6.2f", m->ADscore);
    return tmp;
  }
  return "n/a";
}

// Caller must free returned string
static const char *ADscore_description(Measures *m) {
  char *tmp;
  if (have_valid_ADscore(m)) {
    if (m->p_normal <= config.alpha) {
      if (m->p_normal < 0.001)
	ASPRINTF(&tmp, "p < 0.001 (signif., α = %4.2f) Not normal",
		 config.alpha);
      else
	ASPRINTF(&tmp, "p = %5.3f (signif., α = %4.2f) Not normal",
		 m->p_normal, config.alpha);
      return tmp;
    } else {
      ASPRINTF(&tmp, "p = %5.3f (non-signif., α = %4.2f) Cannot rule out normal",
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
    ASPRINTF(&tmp, "Extreme values (Z ≈ %0.1f): not normal", m->ADscore);
    return tmp;
  }
  return "(not calculated)";
}

// Caller must free returned string
static const char *skew_repr(Measures *m) {
  char *tmp;
  if (!HAS(m->code, CODE_LOWVARIANCE) && !HAS(m->code, CODE_SMALLN)) {
    ASPRINTF(&tmp, "%6.2f", m->skew);
  } else {
    return "n/a";
  }
  return tmp;
}

// Caller must free returned string
static const char *skew_description(Measures *m) {
  char *tmp;
  if (!HAS(m->code, CODE_LOWVARIANCE) && !HAS(m->code, CODE_SMALLN)) {
    ASPRINTF(&tmp, "%s",
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
    ASPRINTF(&tmp, "%6.2f", m->kurtosis);
  } else {
    return "n/a";
  }
  return tmp;
}

// Caller must free returned string
static const char *kurtosis_description(Measures *m) {
  char *tmp;
  if (!HAS(m->code, CODE_SMALLN)) {
    ASPRINTF(&tmp, "%s",
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

  DisplayTable *t = new_display_table(78,
				      3,
				      (int []){16,15,40,END},
				      (int []){2,1,1,END},
				      "|rrl|", true, true);
  int row = 0;
  display_table_fullspan(t, row, 'c', "Total CPU Time Distribution");
  row++;
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
    ASPRINTF(&tmp, FMTs " … " FMTsL, ROUND1(m->min, div), ROUND1(m->max, div));
  else
    ASPRINTF(&tmp, FMT " … " FMTL, ROUND1(m->min, div), ROUND1(m->max, div));
  display_table_set(t, row, 1, tmp);
  display_table_set(t, row, 2, sec ? "s" : "ms");
  row++;

  display_table_set(t, row, 1, (sec ? FMTs : FMT), ROUND1(range, div));
  display_table_set(t, row, 2, sec ? "s" : "ms");
  row++;

  int64_t IQR = m->Q3 - m->Q1;

  display_table_set(t, row, 0, "IQR");
  if (sec)
    ASPRINTF(&tmp, FMTs " … " FMTsL, ROUND1(m->Q1, div), ROUND1(m->Q3, div));
  else
    ASPRINTF(&tmp, FMT " … " FMTL, ROUND1(m->Q1, div), ROUND1(m->Q3, div));
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

  t = new_display_table(78,
			8,
			(int []){10,7,7,7,7,7,7,7,END},
			(int []){2,1,1,1,1,1,1,1,END},
			"|rrrrrrrr|", true, true);
  row = 0;
  display_table_fullspan(t, row, 'c', "Total CPU Time Distribution Tail");
  row++;
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

Ranking *read_input_files(int argc, char **argv) {

  if ((option.first == 0) || (option.first == argc))
    USAGE("No data files to read");
  if (option.first >= MAXDATAFILES)
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
  
  for (int i = option.first; i < argc; i++) {
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
  for (int i = option.first; i < argc; i++) fclose(input[i]);
  // Check for no data actually read from any of the files
  if (usage->next == 0) ERROR("No data read from file(s)");
  // Usage will now be owned by the 'ranking' struct 
  return rank(usage);
}


// TODO: We index into input[] starting at option.first, not 0

// TODO: The CSV reader was not coded for speed.  Could reuse input
// string by replacing commas with NULs.

// TODO: How many summaries do we want to support? MAXCMDS isn't the
// right quantity.

#define RANK_HEADER						\
  "══════ Command ═══════════════════════════ Total time "	\
  "═══ Slower by ════════════════════════════════════════"

#define DOUBLE_BAR						 \
  "════════════════════════════════════════════════════════════" \
  "════════════════════════════════════════════════════════════" \
  "════════════════════════════════════════════════════════════" \
  "════════════════════════════════════════════════════════════" \
  "════════════════════════════════════════════════════════════"

static void add_ranking(DisplayTable *t,
			int *row,
			int cmd_idx,
			bool winnerp,
			Summary *s,
			int64_t best_time) {
  const char *mark = "✗";
  //  const char *check = "✓";
  const char *cmd_fmt = "%4d: %s";
  const char *winner = "✻";

  Inference *infer = s->infer;

  double pct;
  char *tmp, *tmp2, *tmp3;
  char *shift_repr = NULL;
  char *info_line = NULL;

  if (option.explain) {
    display_table_fullspan(t, *row, 'l', "%s", RANK_HEADER);
    (*row)++;
  }

  Units *units = select_units(s->total.median, time_units);
  char *median_repr = apply_units(s->total.median, units, UNITS);
  const int cmd_width = 40;
  char *cmd = command_announcement(s->cmd, cmd_idx, cmd_fmt, cmd_width);

  // For the fastest command, 'infer' will be NULL and there is
  // only the median total run time to print.
  if (infer) {
    pct = infer->shift / (double) best_time;
    shift_repr = apply_units(infer->shift, units, UNITS);
    ASPRINTF(&info_line, "%s%-*s %10s %10s%5.1f%%",
	     winnerp ? winner : " ",
	     cmd_width, cmd, median_repr, shift_repr, pct * 100.0);
  } else {
    ASPRINTF(&info_line, "%s%-*s %10s",
	     winnerp ? winner : " ",
	     cmd_width, cmd, median_repr);
  }
  display_table_fullspan(t, *row, 'l', "%s", info_line);
  (*row)++;

  free(cmd);
  free(median_repr);
  free(shift_repr);
  free(info_line);

  // The fastest command has no comparative stats, so 'infer' will be
  // NULL and there is nothing more to print.
  //
  // And, if we are not printing explanations, there is nothing more.

  if (!option.explain || !infer) return;

  display_table_set(t, *row, 0, "");
  (*row)++;
    
  display_table_set(t, *row, 0, "Timed observations");
  display_table_set(t, *row, 1, "N = %-5d", s->runs);
  (*row)++;

  display_table_set(t, *row, 0, "Mann-Whitney");
  display_table_set(t, *row, 1, "W = %-8.0f", infer->W);
  (*row)++;

  display_table_set(t, *row, 0, "Hodges-Lehmann");
  if (HAS(infer->indistinct, INF_NOEFFECT)) {
    display_table_set(t, *row, 2, mark);
    units = select_units(infer->shift, time_units);
    tmp = apply_units(config.effect, units, UNITS);
    tmp2 = lefttrim(tmp);
    display_table_set(t, *row, 3, "effect < %s", tmp2);
    free(tmp); free(tmp2);
  }
      
  tmp = apply_units(infer->shift, units, UNITS);
  tmp2 = lefttrim(tmp);
  display_table_set(t, *row, 1, "Δ = " NUMFMT_LEFT, tmp2);
  free(tmp); free(tmp2);
  (*row)++;

  // p-values, both adjusted for ties and the unadjusted value
  // (which is more conservative than adjusted value)
  const char *pfmts[4] =
    { "p = %5.3f  (%5.3f)",
      "p = %5.3f  (< %0.3f)",
      "p < %0.3f  (%5.3f)",
      "p < %0.3f  (< %0.3f)" };
  int pselect = (infer->p < 0.001) * 2 + (infer->p_adj < 0.001);
  display_table_set(t, *row, 0, "p-value (adjusted)");
  display_table_set(t, *row, 1, pfmts[pselect],
		    (infer->p < 0.001) ? 0.001 : infer->p,
		    (infer->p_adj < 0.001) ? 0.001 : infer->p_adj);
  if (HAS(infer->indistinct, INF_NONSIG)) {
    display_table_set(t, *row, 2, mark);
    display_table_set(t, *row, 3, "non-signif. (α = %4.2f)", config.alpha);
  }
  (*row)++;

  units = select_units(infer->ci_high, time_units);
  display_table_set(t, *row, 0, "Confidence interval");
  char *ci_low = apply_units(infer->ci_low, units, NOUNITS);
  char *ci_high = apply_units(infer->ci_high, units, NOUNITS);
  ASPRINTF(&tmp, "%4.2f%%", infer->confidence * 100.0);
  tmp2 = lefttrim(ci_low);
  tmp3 = lefttrim(ci_high);
  display_table_set(t, *row, 1, "%s (%s, %s) %2s",
		    tmp, tmp2, tmp3, units->unitname);
  free(ci_low);
  free(ci_high);
  free(tmp); free(tmp2); free(tmp3);
  if (HAS(infer->indistinct, INF_CIZERO)) {
    display_table_set(t, *row, 2, mark);
    units = select_units(config.epsilon, time_units);
    tmp = apply_units(config.epsilon, units, UNITS);
    tmp2 = lefttrim(tmp);
    display_table_set(t, *row, 3, "CI ± %s contains 0", tmp2);
    free(tmp); free(tmp2);
  }
  (*row)++;

  display_table_set(t, *row, 0, "Prob. of superiority");
  display_table_set(t, *row, 1, "Â = %4.2f", infer->p_super);
  if (HAS(infer->indistinct, INF_HIGHSUPER)) {
    display_table_set(t, *row, 2, mark);
    display_table_set(t, *row, 3, "high (> %4.2f)", config.super);
  }
  (*row)++;
}


/*
  The table below is 80 cols wide, and it displays best when the
  number of observations, N, is less than 10,000.

╔══════ Command ══════════════════════════ Total time ═══ Slower by Δ ════════╗
║✓   1. rosie match -o line 'find:{[0-9]{    32.05 ms    0.03 ms     0.1%     ║
║                                                                             ║
║    Timed observations         N = 100                                       ║
║    Mann-Whitney               W = 9532                       Settings       ║
║    Hodges-Lehmann (effect)    Δ = 0.03 ms                  effect = 0.500   ║
║    p-value (adjusted)       ✗ p = 0.206  (0.206)           alpha = 0.050    ║
║    Confidence interval      ✗ 95.33% (-0.01, 0.06) ms      epsilon = 0.250  ║
║    Prob. of superiority     ✗ Â = 0.45                     super = 0.33     ║
╚═════════════════════════════════════════════════════════════════════════════╝

*/

static DisplayTable *ranking_table(void) {
  return new_display_table(80,
			   4,
			   (int []){  22, 26, 1, 23, END},
			   (int []){4,   1,  1, 1, END},
			   "llcl", false, false);
}

static void print_ranking(Ranking *rank) {
  if (!rank) PANIC_NULL();

  if (rank->count < 2) {
    printf("Only one command.  No ranking to show.\n");
    return;
  }

  Summary *s;
  int bestidx = rank->index[0];
  if (rank->summaries[bestidx]->runs < INFERENCE_N_THRESHOLD) {
    printf("Minimum observations must be at least %d for this analysis.\n",
	   INFERENCE_N_THRESHOLD);
    return;
  }

  // Take all the samples indistinguishable from the best performer
  // and group them (in rank order) in the 'same' array so they can be
  // printed together.
  int *same = malloc(rank->count * sizeof(int));
  if (!same) PANIC_NULL();

  same[0] = bestidx;
  int same_count = 1;
  for (int i = 1; i < rank->count; i++) {
    s = rank->summaries[rank->index[i]];
    if (s->infer->indistinct) {
      same[i] = rank->index[i];
      same_count++;
    } else {
      same[i] = -1;
    }
  }

  // -----------------------------------------------------------------------------

  if (same_count > 1) 
    printf("Best guess ranking: The top %d commands performed identically\n",
	   same_count);
  else
    printf("Best guess ranking:\n");

  printf("\n");

  int row = 0;
  DisplayTable *t = ranking_table();

  if (option.explain) {
    Units *units;
    char *tmp, *tmp2;
    display_table_span(t, row, -1, 0, 'l', "Inferential statistics");
    ASPRINTF(&tmp, "Settings (modify with -%s)", optable_shortname(OPT_CONFIG));
    display_table_span(t, row, 1, 3, 'r', "%s", tmp);
    free(tmp);
    row++;

    display_table_span(t, row, 0, 1, 'l', "Significance level, α");
    display_table_span(t, row, 2, 3, 'l', "    alpha    %4.2f", config.alpha);
    row++;

    display_table_span(t, row, 0, 1, 'l', "Probability of superiority");
    display_table_span(t, row, 2, 3, 'l', "    super    %4.2f", config.super);
    row++;

    display_table_span(t, row, 0, 1, 'l', "C.I. ± ε contains zero");
    units = select_units(config.epsilon, time_units);
    tmp = apply_units(config.epsilon, units, UNITS);
    tmp2 = lefttrim(tmp);
    display_table_span(t, row, 2, 3, 'l', "    epsilon  %s", tmp2);
    free(tmp); free(tmp2);
    row++;

    display_table_span(t, row, 0, 1, 'l', "Minimum effect size (H.L. median shift)");
    units = select_units(config.effect, time_units);
    tmp = apply_units(config.effect, units, UNITS);
    tmp2 = lefttrim(tmp);
    display_table_span(t, row, 2, 3, 'l', "    effect   %s", tmp2);
    free(tmp); free(tmp2);
    row++;

    display_table_set(t, row, 0, "");
    row++;

  } else {
    display_table_fullspan(t, row++, 'l', "%s", RANK_HEADER);
  }

  int64_t best_time = rank->summaries[bestidx]->total.median;

  for (int i = 0; i < rank->count; i++) {
    if (same[i] == -1) continue;
    s = rank->summaries[same[i]];
    if (option.explain && s->infer) display_table_set(t, row++, 0, "");
    add_ranking(t, &row, same[i], true, s, best_time);
  }

  if (option.explain)
    display_table_set(t, row++, 0, "");
  else
    display_table_fullspan(t, row++, 'l', "%.*s",
			   utf8_width(DOUBLE_BAR, t->width), DOUBLE_BAR);

  // Print the rest
  bool first_time = true;
  for (int i = 0; i < rank->count; i++)
    if ((same[i] == -1) && (rank->index[i] != bestidx)) {
      s = rank->summaries[rank->index[i]];
      if (option.explain && !first_time) display_table_set(t, row++, 0, "");
      add_ranking(t, &row, rank->index[i], false, s, best_time);
      first_time = false;
    }

  if (same_count < rank->count)
    display_table_fullspan(t, row++, 'l', "%.*s",
			   utf8_width(DOUBLE_BAR, t->width), DOUBLE_BAR);

  display_table(t, 2);
  fflush(stdout);
  free_display_table(t);
  free(same);
}

void write_summary_stats(Summary *s, FILE *csv_output, FILE *hf_output) {
  // If exporting CSV file of summary stats, write that line of data
  if (option.csv_filename) write_summary_line(csv_output, s);
  // If exporting in Hyperfine CSV format, write that line of data
  if (option.hf_filename) write_hf_line(hf_output, s);
}

void report_one_command(Summary *s) {
  // If raw data is going to an output file, we print a summary on the
  // terminal (else raw data goes to terminal so that it can be piped
  // to another process).
  if (!option.output_to_stdout) {
    if (option.report != REPORT_NONE)
      print_summary(s, (option.report == REPORT_BRIEF));
    if (option.report == REPORT_FULL)
      print_descriptive_stats(s);
    if ((option.report != REPORT_NONE) || (option.report == REPORT_FULL))
      printf("\n");
  }
  fflush(stdout);
}

void graph_one_command(Summary *s, Usage *usage, int start, int end) {
  if (!option.output_to_stdout) {
    if (option.graph && usage) {
      print_graph(s, usage, start, end);
    }
    if ((option.report != REPORT_NONE) || option.graph)
      printf("\n");
  }
}

// report() produces box plots and an overall ranking.  These are the
// only printed reports that use all of the data (across all
// commands).
//
// report() also performs the same CSV output and printing of command
// summaries (and bar graphs) that would happen during execution of
// the original experiment.
//
// TODO: Separate these functions and clean up the API.
//
void report(Ranking *ranking) {
  if (!ranking) USAGE("No data");

  Summary *s;
  FILE *csv_output = NULL, *hf_output = NULL;

  if (option.csv_filename) 
    csv_output = maybe_open(option.csv_filename, "w");
  if (option.hf_filename)
    hf_output = maybe_open(option.hf_filename, "w");

  if (csv_output)
    write_summary_header(csv_output);
  if (hf_output)
    write_hf_header(hf_output);

  for (int i = 0; i < ranking->count; i++) {

    s = ranking->summaries[i];

    if (option.action != actionExecute) {
      // If 'actionExecute' then a summary of each command was printed
      // after each series of timed runs.  Otherwise, we need to print
      // a summary, but only for report settings other than NONE.
      if (option.report != REPORT_NONE) {
	announce_command(s->cmd, i);
	printf("\n");
      }
      report_one_command(s);

      // GRAPH
      // If we have are not printing a report for each command, then we
      // need to announce the current command for the graph.
      if (option.graph) {
	if (option.report == REPORT_NONE) {
	  announce_command(s->cmd, i);
	  printf("\n");
	}
	graph_one_command(s,
			  ranking->usage,
			  ranking->usageidx[i],
			  ranking->usageidx[i+1]);
      }

    } // If not executing an experiment
    
    if (option.action == actionReport) {
      write_summary_stats(s, csv_output, hf_output);
    }
    
    fflush(stdout);
  }

  if (csv_output) fclose(csv_output);
  if (hf_output) fclose(hf_output);

  if (option.boxplot)
    print_boxplots(ranking->summaries, 0, ranking->count);

  print_ranking(ranking);

}
