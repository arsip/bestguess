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

typedef struct CSVrow {
  char **fields;
  int    next;
  int    capacity;
} CSVrow;

// Generic utilities

// Read a CSV row from 'f' using 'buf' for storage
CSVrow *read_CSVrow(FILE *f, char *buf, size_t buflen);
// Number of fields in 'row', N
int CSVfields(CSVrow *row);
// Get the contents of field i (numbered 0..N-1) of 'row'
char *CSVfield(CSVrow *row, int i);
// Free the row and all its strings
void free_CSVrow(CSVrow *);

// Output file (raw data, per timed run)

void write_header(FILE *f);
void write_line(FILE *f, Usage *usage, int idx);

// Summary statistics file

void write_summary_line(FILE *f, Summary *s);
void write_summary_header(FILE *f);

// Hyperfine-format file

void write_hf_header(FILE *f);
void write_hf_line(FILE *f, Summary *s);

#endif



