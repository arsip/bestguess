//  -*- Mode: C; -*-                                                       
// 
//  reports.h
// 
//  COPYRIGHT (c) Jamie A. Jennings, 2024

#ifndef reports_h
#define reports_h

#include "bestguess.h"
#include "stats.h"
#include <stdbool.h>

void print_summary(summary *s, int n, bool briefly);
void print_graph(summary *s, Usage *usagedata);
void print_overall_summary(char *commands[],
			   int64_t modes[],
			   int start,
			   int end);

#endif
