//  -*- Mode: C; -*-                                                       
// 
//  csv.h
// 
//  Copyright (C) Jamie A. Jennings, 2024

#ifndef csv_h
#define csv_h

#include "bestguess.h"
#include "stats.h"
#include <stdio.h>

// Output file (raw data, per timed run)

void write_header(FILE *f);
void write_line(FILE *f, const char *cmd, int code, struct rusage *usage);

// Summary statistics file

void write_stat_header(FILE *f);
void write_stat_line(FILE *f, summary *s);

// Hyperfine-format file (also a summary)

void write_hf_header(FILE *f);
void write_hf_line(FILE *f, summary *s);

#endif



