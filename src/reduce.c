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
    if ((normalCDF(z) < 0.0) || (normalCDF(z) > 1.0))
      printf("**** ALERT! CDF value for z-score of %6.3f is %8.5f\n",
	     z, normalCDF(z));
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
  int count = 0;
  int prev = 0;
  while ((s[count] = summarize(usage, &next))) {
    announce_command(get_string(usage, prev, F_CMD), count+1);
    print_summary(s[count], false);
    if (config.show_graph) print_graph(s[count], usage, prev, next);
    printf("\n");
    print_distribution_report(&(s[count]->total), s[count]->runs);
    prev = next;
    if (++count == MAXCMDS) USAGE("too many commands");
  }
  next = 0;
  count = 0;
  int64_t axismin = INT64_MAX;
  int64_t axismax = INT64_MIN;
  while ((s[count] = summarize(usage, &next))) {
    Measures *m = &(s[count]->total);
    axismin = min64(m->min, axismin);
    axismax = max64(m->max, axismax);
    count++;
  }
  int width = 84;
  int scale_min = round((double) axismin / 1000.0);
  int scale_max = round((double) axismax / 1000.0);
  print_boxplot_scale(scale_min, scale_max, width, BOXPLOT_LABEL_ABOVE);
  next = 0;
  count = 0;
  while ((s[count] = summarize(usage, &next))) {
    print_boxplot(&(s[count]->total), axismin, axismax, width);
    count++;
  }
  print_boxplot_scale(scale_min, scale_max, width, BOXPLOT_LABEL_BELOW);
  printf("\n");
  
  print_overall_summary(s, 0, count);

//   printf("\n");
//   write_summary_header(output);
//   for (int i = 0; i < count; i++)
//     write_summary_line(output, s[i]);

  for (int i = 0; i < count; i++) free_summary(s[i]);
  free_usage_array(usage);
  printf("\n");

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


  if (config.input_filename) fclose(input);
  if (config.output_filename) fclose(output);

  return 0;
}

