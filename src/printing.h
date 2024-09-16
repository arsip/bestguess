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

typedef struct DisplayTable {
  int          width;
  int          rows;
  int          cols;
  int         *colwidths;
  int         *margins;
  const char  *justifications;
  char       **items;
  const char  *title;
} DisplayTable;

DisplayTable *new_display_table(const char *title,
				int width,
				int cols,
				int *colwidths,
				int *margins,
				const char *justifications);

void free_display_table(DisplayTable *dt);

void display_table_set(DisplayTable *dt, int row, int col, const char *fmt, ...);

void display_table(DisplayTable *dt, int indent);


#endif
