//  -*- Mode: C; -*-                                                       
// 
//  stats.h
// 
//  COPYRIGHT (c) Jamie A. Jennings, 2024

#ifndef stats_h
#define stats_h

#include "bestguess.h"

int64_t find_median(struct rusage *usagedata, int field);
Summary *summarize(char *cmd, struct rusage *usagedata);


#endif
