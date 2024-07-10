//  -*- Mode: C; -*-                                                       
// 
//  optiontest.c
// 
//  COPYRIGHT (c) Jamie A. Jennings, 2024

#include "utils.h"
#include "optable.h"

static const char *example1 =
  "|o output      1|"
  "|i input       1|"
  "|- show-output 1|"
  "|v -           0|"
  "|h help        0|";

static const char *example2 =
  "o output      0 | i input       1";

static const char *example3 =
  "  ||  o output      0 || i input       1 ||		";

#define ConfigList(X)			\
  X(OPT_WARMUP,     "w warmup      1")	\
  X(OPT_RUNS,       "r runs        1")	\
  X(OPT_OUTPUT,     "o output      1")	\
  X(OPT_INPUT,      "i input       1")	\
  X(OPT_SHOWOUTPUT, "- show-output 1")	\
  X(OPT_SHELL,      "S shell       1")	\
  X(OPT_VERSION,    "v version     0")	\
  X(OPT_HELP,       "h help        0")  \
  X(OPT_SNOWMAN,    "⛄ snowman    0")

#define X(a, b) a,
typedef enum XOptions { ConfigList(X) };
#undef X
#define X(a, b) b "|"
static const char *option_config = ConfigList(X);
#undef X

static void print_options(optable_options *opts) {
  if (!opts) {
    printf("null options table\n");
    return;
  }
  for (int i = 0; i < opts->count; i++) {
    printf("[%2d] %6s  %20s  %1d\n",
	   i,
	   optable_shortname(opts, i),
	   optable_longname(opts, i),
	   optable_numvals(opts, i));
  }
}


int main(int argc, char *argv[]) {

  optable_options *opts;
  if (argc) progname = argv[0];

  printf("%s: Printing option configuration\n", progname);

  printf("\nExample config 1 is:\n'%s'\n\n", example1);
  opts = optable_init(example1);
  print_options(opts);
  optable_free(opts);

  printf("\nExample config 2 is:\n'%s'\n\n", example2);
  opts = optable_init(example2);
  print_options(opts);
  optable_free(opts);

  printf("\nExample config 3 is:\n'%s'\n\n", example3);
  opts = optable_init(example3);
  print_options(opts);
  optable_free(opts);

  printf("\n%s: Parsing command-line arguments\n\n", progname);
  opts = optable_init(option_config);
  print_options(opts);
  printf("\n");

  const char *val;
  int n, i = 0;
  printf("Arg  Option  Value\n");
  printf("---  ------  -----\n");
  while ((i = optable_iter(opts, argc, argv, &n, &val, i))) {
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
  while ((i = optable_iter(opts, argc, argv, &n, &val, i))) {
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
      case OPT_SNOWMAN:
	printf("%15s  %s\n", "snowman", val);
	break;
      default:
	printf("Error: Invalid option index %d\n", n);
    }
    if (val) {
      if (!*val)
	printf("Warning: option value is empty\n");
      if (!optable_numvals(opts, n))
	printf("Error: option does not take a value\n");
    } else {
      if (optable_numvals(opts, n))
	printf("Error: option requires a value\n");
    }
  }
  optable_free(opts);
  return 0;
}

