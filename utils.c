//  -*- Mode: C; -*-                                                       
// 
//  utils.c
// 
//  COPYRIGHT (c) Jamie A. Jennings, 2024

#include "utils.h"

void bail(const char *msg) {
  warn(progname, msg);
  exit(-1);
}

