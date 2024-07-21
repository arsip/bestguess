//  -*- Mode: C; -*-                                                       
// 
//  reduce.c
// 
//  Copyright (C) Jamie A. Jennings, 2024

#include "common.h"
#include "utils.h"
#include "reduce.h"

int reduce_data(void) {
  FILE *input = NULL, *output = NULL;
  char buf[MAXCSVLEN];

  printf("Not fully implemented yet\n");

  input = maybe_open(input_filename, "r");
  output = maybe_open(output_filename, "w");
  if (!output) output = stdout;
  if (!input) input = stdin;

  char *line = NULL;
  while ((line = fgets(buf, MAXCSVLEN, input)))
    printf("%s", line);

  if (input) fclose(input);
  if (output) fclose(output);
  return 0;
}

