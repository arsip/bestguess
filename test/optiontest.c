//  -*- Mode: C; -*-                                                       
// 
//  optiontest.c
// 
//  COPYRIGHT (c) Jamie A. Jennings, 2024

#include <stdio.h>
#include <stdlib.h>
#include "optable.h"

#define ASSERT(val) do {					\
    if (!(val)) {						\
      fprintf(stderr, "ERROR: Assertion failed %s:%d\n",	\
	      __FILE__, __LINE__);				\
      fflush(NULL);						\
      exit(-1);							\
    }								\
  } while (0)

typedef enum Options { 
  OPT_UNUSED1,			// for testing
  OPT_WARMUP,
  OPT_RUNS,
  OPT_OUTPUT,
  OPT_INPUT,
  OPT_UNUSED2,			// for testing
  OPT_SHOWOUTPUT,
  OPT_SHELL,
  OPT_VERSION,
  OPT_HELP,
  OPT_UNUSED3,			// for testing
  OPT_UNUSED4,			// for testing
  OPT_UNUSED5,			// for testing
  OPT_UNUSED6,			// for testing
  OPT_UNUSED7,			// for testing
  OPT_SNOWMAN,
  OPT_ERR_TEST			// for testing
};

static void init_options(void) {
  optable_add(OPT_WARMUP,     "w", "warmup",       1, "Number of warmup runs");
  optable_add(OPT_RUNS,       "r", "runs",         1, "Number of timed runs");
  optable_add(OPT_OUTPUT,     "o", "output",       1, "Filename for timing data (CSV)");
  optable_add(OPT_INPUT,      "i", "input",        1, "Filename for input (commands)");
  optable_add(OPT_SHOWOUTPUT, NULL, "show-output", 1, "When set, program output is shown");
  optable_add(OPT_SHELL,      "S", "shell",        1, "Shell to use to run commands (default is none)");
  optable_add(OPT_VERSION,    "v", "version",      0, "Show version");
  optable_add(OPT_HELP,       "h", "help",         0, "Show help");
  optable_add(OPT_SNOWMAN,    "⛄", "snowman",     0, "Enjoy a snowman, any time of year!");
  ASSERT(!optable_error());
}

static void print_options(void) {
  int i;
  for (i = optable_iter_start(); i >= 0; i = optable_iter_next(i)) {
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

#define EXPECT_WARNING do {			\
    printf("Expect a warning here:\n");		\
  } while (0)

int main(int argc, char *argv[]) {

  int err;

  EXPECT_WARNING;
  err = optable_add(OPT_ERR_TEST, NULL, NULL, 0, "Help! No option names!");
  ASSERT(err);
  EXPECT_WARNING;
  err = optable_add(OPT_ERR_TEST, "", NULL, 0, "Help! Empty shortname!");
  ASSERT(err);
  EXPECT_WARNING;
  err = optable_add(OPT_ERR_TEST, NULL, "", 0, "Help! Empty longname!");
  ASSERT(err);
  EXPECT_WARNING;
  err = optable_add(OPT_ERR_TEST, "", "", 0, "Help! Both names empty!");
  ASSERT(err);
  EXPECT_WARNING;
  err = optable_add(OPT_ERR_TEST, "h", "help", 2, "Help! Numvals out of range!");
  ASSERT(err);
  EXPECT_WARNING;
  err = optable_add(OPT_ERR_TEST, "h", "help", -1, "Help! Numvals out of range!");
  ASSERT(err);
  err = optable_add(OPT_HELP, "h", "help", 0, "Show help");
  ASSERT(!err);

  ASSERT(optable_error());

  // Reset everything after the tests above
  optable_free();

  printf("\nPrinting option configuration\n");
  init_options();
  print_options();
  printf("\n");

  if (argc < 2) {
    printf("\nNo command-line arguments given\n");
    goto done;
  }
  
  printf("\nParsing command-line arguments\n\n");
  const char *val;
  int n, i;
  printf("Arg  Option  Value\n");
  printf("---  ------  -----\n");
  i = optable_init(argc, argv);
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
  optable_init(argc, argv);
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

  done:
  optable_free();
  return 0;
}

