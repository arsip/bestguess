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

// Error: missing field in first "record"
static const char *example4 =
  "  ||  o       0 || i input       1 ||		";

// Error: numvals not 0 or 1
static const char *example5 =
  "o  output     0 || i input       2";

// Error: last record is incomplete
static const char *example6 =
  "o  output     0 || i";


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

static void print_options(void) {
  int count = optable_count();
  for (int i = 0; i < count; i++) {
    printf("[%2d] %6s  %20s  %1d\n",
	   i,
	   optable_shortname(i),
	   optable_longname(i),
	   optable_numvals(i));
  }
}

#define RUNTEST(config) do {					\
    printf("\nExample '" #config "' is:\n'%s'\n\n", (config));	\
    err = optable_init((config), argc, argv);			\
    if (err) \
      printf("ERROR in configuration\n");			\
    else {							\
      print_options();						\
      optable_free();						\
    }								\
  } while (0)

#define ASSERT(val) do {			\
    if (!val) bail("assertion failed");		\
  } while(0)

int main(int argc, char *argv[]) {

  int err;
  if (argc) progname = argv[0];

  printf("%s: Printing option configuration\n", progname);

  RUNTEST(example1); ASSERT(!err);
  RUNTEST(example2); ASSERT(!err);
  RUNTEST(example3); ASSERT(!err);
  RUNTEST(example4); ASSERT(err);
  RUNTEST(example5); ASSERT(err);
  RUNTEST(example6); ASSERT(err);

  printf("\n%s: Parsing command-line arguments\n\n", progname);
  err = optable_init(option_config, argc, argv);
  if (err) printf("ERROR returned by init\n");
  print_options();
  printf("\n");

  const char *val;
  int n, i = 0;
  printf("Arg  Option  Value\n");
  printf("---  ------  -----\n");
  while ((i = optable_next(&n, &val, i))) {
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
  while ((i = optable_next(&n, &val, i))) {
    if (n < 0) {
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
      if (!optable_numvals(n))
	printf("Error: option does not take a value\n");
    } else {
      if (optable_numvals(n))
	printf("Error: option requires a value\n");
    }
  }
  optable_free();
  return 0;
}

