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

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

// -----------------------------------------------------------------------------
// 
// -----------------------------------------------------------------------------

DisplayTable *new_display_table(const char *title,
				int width,
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
    PANIC("Columns, margins, and table borders total %d chars,"
	  " more than table width of %d", total + 2, width);

  dt->title = title;		// Allowed to be NULL

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
  int barlength = (dt->width - 2) * bytesperbar;
  char justif;
  const char *item;
  int fwidth, padding, nchars, nbytes;
  
  printf("%*s%s%.*s%s\n", indent, "",
	 bar(LEFT, TOPLINE),
	 barlength, BAR,
	 bar(RIGHT, TOPLINE));

  if (dt->title) {
    fwidth = dt->width - 2;
    nchars = utf8_length(dt->title);
    padding = fwidth - nchars;
    printf("%*s%s", indent, "", bar(LEFT, MIDLINE));
    if (padding > 0)
      printf("%*s", padding / 2, "");
    nbytes = utf8_width(dt->title, fwidth);
    printf("%.*s", nbytes, dt->title);
    if (padding > 0)
      printf("%*s", (padding + 1) / 2, "");
    printf("%s\n", bar(RIGHT, MIDLINE));
  }

  for (int row = 0; row < dt->rows; row++) {
    if (all_null(&(dt->items[row * dt->cols]), dt->cols)) continue;
    printf("%*s%s%*s", indent, "", bar(LEFT, MIDLINE), dt->margins[0], "");
    for (int col = 0; col < dt->cols; col++) {
      item = dt->items[row * dt->cols + col] ?: "";
      justif = dt->justifications[col];
      fwidth = dt->colwidths[col];
      nchars = utf8_length(item);
      padding = fwidth - nchars;
      if ((justif == 'r') && (padding > 0))
	printf("%*s", padding, "");
      // Field width is in characters.  How many bytes are used by the
      // first 'fwidth' characters in the UTF-8 string 'item'?
      nbytes = utf8_width(item, fwidth);
      printf("%.*s", nbytes, item);
      if ((justif == 'l') && (padding > 0))
	printf("%*s", padding, "");
      printf("%*s", dt->margins[col+1], "");
    }
    printf("%s\n", bar(RIGHT, MIDLINE));
  }  

  printf("%*s%s%.*s%s\n", indent, "",
	 bar(LEFT, BOTTOMLINE),
	 barlength, BAR,
	 bar(RIGHT, BOTTOMLINE));
}
