//  -*- Mode: C; -*-                                                       
// 
//  reduce.c
// 
//  Copyright (C) Jamie A. Jennings, 2024

#include "bestguess.h"
#include "utils.h"
#include "csv.h"
#include "reduce.h"
#include "string.h"

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

  struct Usage usage;
  CSVrow *row;
  // Headers
  row = read_CSVrow(input);
  for (int i = 0; i < CSVfields(row); i++) {
    printf("%s\n", CSVfield(row, i));
  }
  // Rows
  while ((row = read_CSVrow(input))) {
    set_usage_string(&usage,
		     F_CMD,
		     strndup(CSVfield(row, F_CMD), MAXCMDLEN));
    set_usage_string(&usage,
		     F_SHELL,
		     strndup(CSVfield(row, F_SHELL), MAXCMDLEN));
    for (int fc = F_CODE; fc < F_TOTAL; fc++) {
      set_usage_int64(&usage, fc, strtoint64(CSVfield(row, fc)));
    }
    set_usage_int64(&usage, F_TOTAL,
		    get_usage_int64(&usage, F_USER) +
		    get_usage_int64(&usage, F_SYSTEM));
    set_usage_int64(&usage, F_TCSW,
		    get_usage_int64(&usage, F_ICSW) +
		    get_usage_int64(&usage, F_VCSW));
    free_CSVrow(row);
    write_line(output, &usage);
  }

  


  if (config.input_filename) fclose(input);
  if (config.output_filename) fclose(output);

  return 0;
}

