//  -*- Mode: C; -*-                                                       
// 
//  optiontest.c
// 
//  COPYRIGHT (c) Jamie A. Jennings, 2024

#include "utils.h"
#include "optable.h"

int main(int argc, char *argv[]) {

  if (argc) progname = argv[0];
  printf("%s: Printing command-line arguments\n\n", progname);

  const char *val;
  int n, i = 0;
  printf("Arg  Option  Value\n");
  printf("---  ------  -----\n");
  while ((i = optable_iter(argc, argv, &n, &val, i))) {
    if (n < 0) {
      if (val)
	printf("[%2d]         %-20s\n", i, argv[i]); 
      else
	printf("[%2d] ERR     %-20s\n", i, argv[i]);
    }
    else
      printf("[%2d] %3d     %-20s\n", i, n, val); 
  }

  // -------------------------------------------------------

  printf("\nRecognized command-line arguments:\n\n");
  
  printf("Option           Value\n");
  printf("---------------  -------------\n");
  i = 0;
  while ((i = optable_iter(argc, argv, &n, &val, i))) {
    if (n == -1) {
      if (val) continue;	// ordinary argument
      printf("Error: invalid option/switch %s\n", argv[i]);
      continue;
    }
    switch (n) {
      case OPT_WARMUP:
	printf("%15s  %s\n", "warmup", val);
	break;
      case OPT_RUNS:
	printf("%15s  %s\n", "runs", val);
	break;
      case OPT_OUTPUT:
	printf("%15s  %s\n", "output", val);
	break;
      case OPT_INPUT:
	printf("%15s  %s\n", "input", val);
	break;
      case OPT_SHOWOUTPUT:
	printf("%15s  %s\n", "show-output", val);
	break;
      case OPT_SHELL:
	printf("%15s  %s\n", "shell", val);
	break;
      case OPT_VERSION:
	printf("%15s  %s\n", "version", val);
	break;
      case OPT_HELP:
	printf("%15s  %s\n", "help", val);
	break;
      default:
	printf("Error: Invalid option index %d\n", n);
    }
    if (val) {
      if (!*val) printf("Warning: option value is empty\n");
      if (!OptionNumVals[n]) printf("Error: option does not take a value\n");
    } else {
      if (OptionNumVals[n]) printf("Error: option requires a value\n");
    }
  }
  return 0;
}

