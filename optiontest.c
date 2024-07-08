//  -*- Mode: C; -*-                                                       
// 
//  optiontest.c
// 
//  COPYRIGHT (c) Jamie A. Jennings, 2024

#include "utils.h"
#include "args.h"

int main(int argc, char *argv[]) {

  if (argc) progname = argv[0];
  
  printf("%s: Printing command-line arguments\n\n", progname);

//   printf("Option  Value              Original argv[n]\n");
//   for (int i = 1; i < argc; i++) {
//     if (is_option(argv[i])) {
//       n = parse_option(argv[i], &val);
//       if ((n > 0) && !val && OptionNumVals[n] && !is_option(argv[i+1]))
// 	val = argv[++i];
//       printf("%3d     %-20s  %s\n", n, val, argv[i]); 
//     } else {
//       printf("%3s     %-20s  %s\n", "", "", argv[i]);
//     }
//   }

//   printf("Arg  Option  Value                 Original argv[n]\n");
//   int i = 1;
//   int prev = i;
//   while ((i = iter_argv(argc, argv, &n, &val, i))) {
//     printf("[%2d] %3d     %-20s  %s\n", i, n, val, argv[prev]); 
//     prev = i;
//   }

  const char *val;
  int n, i = 0;
  printf("Arg  Option  Value\n");
  while ((i = iter_argv(argc, argv, &n, &val, i))) {
    if ((n < 0) && !val)
      printf("[%2d] ERR     %-20s\n", i, argv[i]); 
    else
      printf("[%2d] %3d     %-20s\n", i, n, val); 
  }
  return 0;
}

