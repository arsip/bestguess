//  -*- Mode: C; -*-                                                       
// 
//  reduce.c
// 
//  Copyright (C) Jamie A. Jennings, 2024

#include "bestguess.h"
#include "utils.h"
#include "csv.h"
#include "reports.h"		// TEMP!
#include "reduce.h"
#include "math.h"

int reduce_data(void) {
  FILE *input = NULL, *output = NULL;

  // TEMP:
  // print_zscore_table();


  // Just a way to test our assertions:
  const int n = 10000;
  for (int i=0; i < n; i++) {
    double z = (((double) i) - ((double) n / 2.0)) / 500.0;
    if ((zscore(z) < 0.0) || (zscore(z) > 1.0))
      printf("**** ALERT! Z-score for %6.3f is %8.5f\n", z, zscore(z));
  }

//   printf("Relevant configuration:\n");
//   printf("  Input filename:  %s\n", config.input_filename);
//   printf("  Output filename: %s\n", config.output_filename);

  input = maybe_open(config.input_filename, "r");
  output = maybe_open(config.output_filename, "w");
  if (!output) output = stdout;
  if (!input) input = stdin;

  // TODO: Increase initial count after some testing
  struct Usage *usage = new_usage_array(100);
  CSVrow *row;
  // Headers
  row = read_CSVrow(input);
  // Rows
  while ((row = read_CSVrow(input))) {
    int idx = usage_next(usage);
    set_string(usage, idx, F_CMD, CSVfield(row, F_CMD));
    set_string(usage, idx, F_SHELL, CSVfield(row, F_SHELL));
    for (int fc = F_CODE; fc < F_TOTAL; fc++) {
      set_int64(usage, idx, fc, strtoint64(CSVfield(row, fc)));
    }
    set_int64(usage, idx, F_TOTAL, get_int64(usage, idx, F_USER) + get_int64(usage, idx, F_SYSTEM));
    set_int64(usage, idx, F_TCSW, get_int64(usage, idx, F_ICSW) + get_int64(usage, idx, F_VCSW));
    free_CSVrow(row);
    //write_line(output, usage, idx);
  }

  Summary *s[MAXCMDS];
  int next = 0;
  int prev = 0;
  int count = 0;
  while ((s[count] = summarize(usage, &next))) {
    announce_command(get_string(usage, prev, F_CMD), count+1);
    print_summary(s[count], false);
    if (config.show_graph) print_graph(s[count], usage, prev, next);
    printf("\n");

    printf("Have %d data points in range %" PRId64 "..%" PRId64 "\n",
	   s[count]->runs, s[count]->total.min, s[count]->total.max);

    int64_t IQR = s[count]->total.Q3 - s[count]->total.Q1;
    printf("Inter-quartile range = %" PRId64 " (%6.4f%% of full range)\n",
	   IQR, ((double) IQR / (double) (s[count]->total.max - s[count]->total.min)) * 100.0);

    if (s[count]->total.ADscore > 0) {
      if (s[count]->total.p_normal <= 0.10)
	printf("Distribution is NOT normal ");
      else
	printf("Distribution could be normal ");
      printf("(ADscore %6.4f) with p = %6.4f\n",
	     s[count]->total.ADscore, s[count]->total.p_normal);
    } else {
      printf("Unable to test for normality\n");
    }

    // Standard deviation has same units as the data, so we can
    // express it as a percentage of the mean.
    //
    // TODO: What is a good value for this warning?
    double stddev_ratio = s[count]->total.est_stddev / s[count]->total.est_mean;
    if (stddev_ratio < 0.001) {
      printf("Warning: Low estimated variance ratio %3.2f%%\n", stddev_ratio * 100.0);
    }

    if (s[count]->total.skew > 0) {
      if (fabs(s[count]->total.skew) > 0.2)
	printf("Non-parametric skew is significant at %4.2f\n", s[count]->total.skew);
      else
	printf("Non-parametric skew not significant at %4.2f\n", s[count]->total.skew);
    } else {
      printf("No skew (very low variance)\n");
    }
      
    printf("\n");
    prev = next;
    if (++count == MAXCMDS) USAGE("too many commands");
  }

  print_overall_summary(s, 0, count);

//   printf("\n");
//   write_summary_header(output);
//   for (int i = 0; i < count; i++)
//     write_summary_line(output, s[i]);

  for (int i = 0; i < count; i++) free_summary(s[i]);
  free_usage_array(usage);

  if (config.input_filename) fclose(input);
  if (config.output_filename) fclose(output);

  return 0;
}

