//  -*- Mode: C; -*-                                                       
// 
//  graphs.h
// 
//  Copyright (C) Jamie A. Jennings, 2024

#ifndef graphs_h
#define graphs_h

#include "bestguess.h"
#include "stats.h"

void print_graph(Summary *s, Usage *usagedata, int start, int end);
void print_boxplots(Summary *s[], int start, int end);

void maybe_boxplots(Ranking *ranking);
void maybe_graph(Summary *s, Usage *usage, int start, int end);

#endif



