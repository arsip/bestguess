//  -*- Mode: C; -*-                                                       
// 
//  reports.h
// 
//  COPYRIGHT (c) Jamie A. Jennings, 2024

#ifndef reports_h
#define reports_h

#include "bestguess.h"
#include "stats.h"

void print_command_summary(summary *s);
void print_graph(summary *s, struct rusage *usagedata);
void print_overall_summary(char *commands[],
			   int64_t modes[],
			   int start,
			   int end);

#endif
