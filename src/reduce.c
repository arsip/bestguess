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

  printf("Relevant configuration:\n");
  printf("  Input filename:  %s\n", config.input_filename);
  printf("  Output filename: %s\n", config.output_filename);

  input = maybe_open(config.input_filename, "r");
  output = maybe_open(config.output_filename, "w");
  if (!output) output = stdout;
  if (!input) input = stdin;

  // TODO: Increase initial count after some testing
  struct Usage *usage = new_usage_array(1);
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
    write_line(output, usage, idx);
  }

  // TEMP: Summarize only first cmd found
  char *cmd = get_string(usage, 0, F_CMD);
  summary *s = summarize(cmd, usage);
  print_summary(s, 1, false);

  if (config.input_filename) fclose(input);
  if (config.output_filename) fclose(output);

  return 0;
}

