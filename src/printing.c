//  -*- Mode: C; -*-                                                       
// 
//  printing.c
// 
//  COPYRIGHT (c) Jamie A. Jennings, 2024

#include "printing.h"
#include "utils.h"
#include <assert.h>
#include <stdarg.h> 		// __VA_ARGS__ (var args)

#define MAXROWS 100

#define LEFT       0
#define RIGHT      1
#define TOPLINE    0
#define MIDLINE    1
#define BOTTOMLINE 2

// -----------------------------------------------------------------------------
// 
// -----------------------------------------------------------------------------

DisplayTable *new_display_table(int width,
				int cols,
				int *colwidths,
				int *margins,
				const char *justifications) {
  if ((width < 2) || (cols < 1) || !colwidths || !margins || !justifications)
    PANIC("Bad arguments to display table");

  // This table does not dynamically grow (not needed)
  int rows = MAXROWS;
  
  DisplayTable *dt = malloc(sizeof(DisplayTable));
  if (!dt) PANIC_OOM();
  dt->width = width;
  dt->rows = rows;
  dt->cols = cols;
  dt->colwidths = malloc(sizeof(int) * cols);
  if (!dt->colwidths) PANIC_OOM();
  dt->margins = malloc(sizeof(int) * (cols + 1));
  if (!dt->margins) PANIC_OOM();
  dt->items = malloc(sizeof(char *) * rows * cols);
  if (!dt->items) PANIC_OOM();

  if (strlen(justifications) != (size_t) cols)
    PANIC("Display table requires one justification character per column");
  
  dt->justifications = justifications;
  
  for (int i = 0; i < rows * cols; i++)
    dt->items[i] = NULL;

  int total = 0;
  for (int i = 0; i < cols; i++) {
    dt->colwidths[i] = colwidths[i];
    dt->margins[i] = margins[i];
    total += colwidths[i] + margins[i];
  }
  dt->margins[cols] = width - 2 - total;

  if (dt->margins[cols] < 0)
    PANIC("Columns and margins total %d chars, more than table width of %d", total, width);


  return dt;
}

void free_display_table(DisplayTable *dt) {
  if (!dt) return;
  free(dt->colwidths);
  free(dt->margins);
  for (int i = 0; i < dt->rows * dt->cols; i++) free(dt->items[i]);
  free(dt->items);
  free(dt);
}

// Takes ownership of 'buf'
static void insert(DisplayTable *dt, int row, int col, char *buf) {
  if (!dt) PANIC_NULL();
  if ((row < 0) || (row >= dt->rows)) PANIC("No such row (%d) in display table", row);
  if ((col < 0) || (col >= dt->cols)) PANIC("No such column (%d) in display table", col);
  int idx = row * dt->cols + col;
  if (dt->items[idx]) free(dt->items[idx]);
  dt->items[idx] = buf;
}

__attribute__ ((format (printf, 4, 5)))
void display_table_set(DisplayTable *dt, int row, int col, const char *fmt, ...) {
  va_list things;
  char *buf;
  va_start(things, fmt);
  vasprintf(&buf, fmt, things);
  va_end(things);
  insert(dt, row, col, buf);
}

static const char *bar(int side, int line) {
  const char *decor[6] = { "┌", "┐",
			   "│", "│",
			   "└", "┘" };
  if ((side != LEFT) && (side != RIGHT))
    return ":";
  if ((line < TOPLINE) || (line > BOTTOMLINE))
    return "!";
  return decor[line * 2 + side];
}

#define BAR "────────────────────────────────────────────────────────────────────────────────"

static bool all_null(char **items, int cols) {
  for (int i = 0; i < cols; i++)
    if (items[i]) return false;
  return true;
}

// Print until an entire row has NULL in every column
void display_table(DisplayTable *dt, int indent) {
  if (!dt) PANIC_NULL();
  if (indent < 0) indent = 0;
  int bytesperbar = (uint8_t) "─"[0] >> 6; // Assumes UTF-8
  int barlength = (dt->width - indent - 2) * bytesperbar;
  char justif;
  int fwidth;
  
  printf("%*s%s%.*s%s\n", indent, "",
	 bar(LEFT, TOPLINE),
	 barlength, BAR,
	 bar(RIGHT, TOPLINE));

  for (int row = 0; row < dt->rows; row++) {
    if (all_null(&(dt->items[row * dt->cols]), dt->cols)) continue;
    printf("%*s%s%*s", indent, "", bar(LEFT, MIDLINE), dt->margins[0], "");
    for (int col = 0; col < dt->cols; col++) {
      const char *item = dt->items[row * dt->cols + col] ?: "";
      justif = dt->justifications[col];
      fwidth = (justif == 'l') ? - dt->colwidths[col] : dt->colwidths[col];
      printf("%*.*s", fwidth, dt->colwidths[col], item);
      printf("%*s", dt->margins[col+1], "");
    }
    printf("%s\n", bar(RIGHT, MIDLINE));
  }  

  printf("%*s%s%.*s%s\n", indent, "",
	 bar(LEFT, BOTTOMLINE),
	 barlength, BAR,
	 bar(RIGHT, BOTTOMLINE));
}
