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

typedef struct DisplaySpan {
  int row;
  int start_col;
  int end_col;
  int width;
  char justification;
} DisplaySpan;

typedef struct DisplayTable {
  int          width;		// Includes borders (one left, one right)
  int          rows;		// Capacity (max possible rows)
  int          cols;		// Default number of columns per row
  int         *colwidths;	// Width for each column
  int         *margins;		// Margin BEFORE column i is margin[i]
  char        *justifications;	// c/r/l for column i is justifications[i]
  const char  *orig_justif;	// May have vertical border indicators
  int          borders;		// Number of table border chars
  bool         leftborder;	// True if table has a left border
  bool         rightborder;	// True if table has a right border
  int          rightpad;	// Padding after last data col before border
  char       **items;		// Array of columns x rows items to display
  int          maxspans;	// Max number of spans in entire table
  DisplaySpan *spans;		// List of spans, unordered
} DisplayTable;

DisplayTable *new_display_table(int width,
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

void display_table_title(DisplayTable *dt, int row, char justif, const char *title);

void display_table_hline(DisplayTable *dt, int row);

void display_table(DisplayTable *dt, int indent);


#endif
