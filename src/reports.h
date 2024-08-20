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

void announce_command(const char *cmd, int number);
void print_summary(Summary *s, bool briefly);
void print_graph(Summary *s, Usage *usagedata, int start, int end);
void print_overall_summary(Summary *summaries[], int start, int end);
void print_boxplot(Measures *m, int64_t axismin, int64_t axismax, int width);


#endif
