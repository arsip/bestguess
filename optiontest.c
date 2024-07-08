//  -*- Mode: C; -*-                                                       
// 
//  optiontest.c
// 
//  COPYRIGHT (c) Jamie A. Jennings, 2024

#include "utils.h"
#include "args.h"

int main(int argc, char *argv[]) {

  if (argc) progname = argv[0];
  
  int n;
  const char *val;

  printf("%s: Printing command-line arguments\n\n", progname);

  printf("Option  Value              Original argv[n]\n");
  for (int i = 1; i < argc; i++) {
    if (is_option(argv[i])) {
      n = parse_option(argv[i], &val);
      printf("%3d     %-20s  %s\n", n, val, argv[i]); 
    } else {
      printf("%3s     %-20s  %s\n", "", "", argv[i]);
    }
  }

  return 0;
}

