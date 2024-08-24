//  -*- Mode: C; -*-                                                       
// 
//  reports.h
// 
//  COPYRIGHT (c) Jamie A. Jennings, 2024

#ifndef reports_h
#define reports_h

#include "bestguess.h"
#include "stats.h"

// How many input files of data are we willing to read?
#define MAXDATAFILES 400

#define REPORT_FULL    0
#define REPORT_SUMMARY 1

void report(int argc, char **argv);

void announce_command(const char *cmd, int number);
void print_summary(Summary *s, bool briefly);
void print_graph(Summary *s, Usage *usagedata, int start, int end);
void print_overall_summary(Summary *summaries[], int start, int end);
void print_descriptive_stats(Measures *m, int n);

#define BOXPLOT_NOLABELS 0
#define BOXPLOT_LABEL_ABOVE 1
#define BOXPLOT_LABEL_BELOW 2
void print_boxplot(Measures *m, int64_t axismin, int64_t axismax, int width);
void print_boxplot_scale(int axismin, int axismax, int width, int labelplacement);

// For debugging
void print_ticks(Measures *m, int width);


#endif
