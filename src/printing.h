//  -*- Mode: C; -*-                                                       
// 
//  printing.h
// 
//  COPYRIGHT (c) Jamie A. Jennings, 2024

#ifndef printing_h
#define printing_h

#include "bestguess.h"
#define INT64FMT "%" PRId64

// -----------------------------------------------------------------------------
// 
// -----------------------------------------------------------------------------

// Rows and columns are numbered starting at 0.

typedef struct DisplayTable {
  int          width;		// Includes borders (one left, one right)
  int          rows;		// Capacity (max possible rows)
  int          cols;		// Default number of columns per row
  int         *colwidths;	// Width for each column
  int         *margins;		// Margin before column i is in margin[i]
  const char  *justifications;	// c/r/l for column i is in justifications[i]
  char       **items;		// Array of columns x rows items to display
  int          maxspans;	// Max number of spans in entire table
  struct Span {
    int row;
    int start_col;
    int end_col;
    int width;
    char justification;
  }           *spans;		// List of spans, unordered
  const char  *title;		// Optional title, can be NULL
} DisplayTable;

DisplayTable *new_display_table(const char *title,
				int width,
				int cols,
				int *colwidths,
				int *margins,
				const char *justifications);

void free_display_table(DisplayTable *dt);

void display_table_set(DisplayTable *dt,
		       int row,
		       int col,
		       const char *fmt, ...);

void display_table_span(DisplayTable *dt,
			int row,
			int start_col, int end_col,
			char justification,
			const char *fmt, ...);

void display_table(DisplayTable *dt, int indent);


#endif
