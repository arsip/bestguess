//  -*- Mode: C; -*-                                                       
// 
//  reduce.c
// 
//  Copyright (C) Jamie A. Jennings, 2024

#include "bestguess.h"
#include "utils.h"
#include "csv.h"
#include "reduce.h"

int reduce_data(void) {
  FILE *input = NULL, *output = NULL;

  printf("Relevant configuration:\n");
  printf("  Input filename:  %s\n", config.input_filename);
  printf("  Output filename: %s\n", config.output_filename);

  input = maybe_open(config.input_filename, "r");
  output = maybe_open(config.output_filename, "w");
  if (!output) output = stdout;
  if (!input) input = stdin;

  CSVrow *row;
  while ((row = read_CSVrow(input))) {
    for (int i = 0; i < CSVfields(row); i++) {
      printf("%s\n", CSVfield(row, i));
    }
    free_CSVrow(row);
  }

  if (input) fclose(input);
  if (output) fclose(output);
  return 0;
}

