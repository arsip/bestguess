//  -*- Mode: C; -*-                                                       
// 
//  reports.h
// 
//  COPYRIGHT (c) Jamie A. Jennings, 2024

#ifndef reports_h
#define reports_h

#include "bestguess.h"

void print_command_summary(Summary *s);
void print_graph(Summary *s, struct rusage *usagedata);
void print_overall_summary(const char *commands[],
			   int64_t mediantimes[],
			   int n);

#endif
