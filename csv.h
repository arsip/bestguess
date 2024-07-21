//  -*- Mode: C; -*-                                                       
// 
//  csv.h
// 
//  Copyright (C) Jamie A. Jennings, 2024

#ifndef csv_h
#define csv_h

#include <stdio.h>
#include "common.h"
#include "bestguess.h"

// Output file (raw data, per timed run)

void write_header(FILE *f);
void write_line(FILE *f, const char *cmd, int code, struct rusage *usage);

// Hyperfine-format file

void write_hf_header(FILE *f);
void write_hf_line(FILE *f, Summary *s);

#endif



