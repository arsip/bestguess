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

void report(Ranking *ranking);

Ranking *read_input_files(int argc, char **argv);

void print_summary(Summary *s, bool briefly);
void print_overall_summary(Summary *summaries[], int start, int end);
void print_distribution_stats(Summary *s);
void print_tail_stats(Summary *s);

void per_command_output(Summary *s, Usage *usage, int start, int end);

#endif
