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

  // Total number of spans in a table is artificially limited to (on
  // average) 2 per row.
  dt->maxspans = 2 * MAXROWS;
  dt->spans = malloc(dt->maxspans * sizeof(struct Span));
  if (!dt->spans) PANIC_OOM();
  // Initialize each span to "unused" using row number of -1
  for (int i = 0; i < MAXROWS; i++) dt->spans[i].row = -1;

  // The title is allowed to be NULL
  dt->title = title ? strndup(title, width - 2) : NULL;

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

// // Takes ownership of 'buf'
// static void insert(DisplayTable *dt, int row, int col, char *buf) {
//   if (!dt) PANIC_NULL();
//   if ((row < 0) || (row >= dt->rows)) PANIC("No such row (%d) in display table", row);
//   if ((col < 0) || (col >= dt->cols)) PANIC("No such column (%d) in display table", col);
//   int idx = row * dt->cols + col;
//   if (dt->items[idx]) free(dt->items[idx]);
//   dt->items[idx] = buf;
// }

// Takes ownership of 'buf'
static void insert(DisplayTable *dt,
		   int row,
		   int start_col, int end_col,
		   char justification,
		   char *buf) {
  if (!dt) PANIC_NULL();
  if ((row < 0) || (row >= dt->rows))
    PANIC("No such row (%d) in display table", row);
  if ((start_col < 0) || (start_col >= dt->cols))
    PANIC("No such column (%d) in display table", start_col);
  if ((end_col < 0) || (end_col >= dt->cols))
    PANIC("No such column (%d) in display table", end_col);
  if (start_col > end_col)
    PANIC("Start column (%d) is after end column (%d) in display table", start_col, end_col);

  int idx = row * dt->cols + start_col;

  // The display table OWNS the items, so if the caller is replacing
  // an item, we can (must) free the old one
  if (dt->items[idx]) free(dt->items[idx]);
  dt->items[idx] = buf;

  // If this table entry is NOT a span, then we are done
  if (start_col == end_col)
    if ((justification == '\0') || (justification == dt->justifications[start_col]))
      return;

  // Find the next available entry in the span list
  int i;
  for (i = 0; i < dt->maxspans; i++) if (dt->spans[i].row == -1) break;
  if (i == dt->maxspans)
    PANIC("Too many spans in display table (limit is %d)", dt->maxspans);

  int spanwidth = 0;
  for (i = start_col; i <= end_col; i++)
    spanwidth += dt->colwidths[i] + dt->margins[i];

  dt->spans[i] = (struct Span) {.row = row,
				.start_col = start_col,
				.end_col = end_col,
				.width = spanwidth,
				.justification = justification};
  //printf("Inserted span at row %d cols (%d, %d) '%c'\n", row, start_col, end_col, justification);
}

__attribute__ ((format (printf, 4, 5)))
void display_table_set(DisplayTable *dt, int row, int col, const char *fmt, ...) {
  va_list things;
  char *buf;
  va_start(things, fmt);
  if (vasprintf(&buf, fmt, things) == -1) PANIC_OOM();
  va_end(things);
  insert(dt, row, col, col, '\0', buf);
}

// A span is needed when you want to display a single item across
// multiple columns, though it can also be used to change the
// justification in a single column.  NOTE: The 'end_col' is
// inclusive.
__attribute__ ((format (printf, 6, 7)))
void display_table_span(DisplayTable *dt,
			int row,
			int start_col, int end_col,
			char justification,
			const char *fmt, ...) {
  va_list things;
  char *buf;
  va_start(things, fmt);
  if (vasprintf(&buf, fmt, things) == -1) PANIC_OOM();
  va_end(things);
  insert(dt, row, start_col, end_col, justification, buf);
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

// Max width for the top and bottom borders of a table to properly be
// displayed is the length of BAR, currently 8 * 40 = 320 chars.
#define BAR \
  "───────────────────────────────────────"	     \
  "───────────────────────────────────────"	     \
  "───────────────────────────────────────"	     \
  "───────────────────────────────────────"	     \
  "───────────────────────────────────────"	     \
  "───────────────────────────────────────"	     \
  "───────────────────────────────────────"	     \
  "───────────────────────────────────────"

static bool all_null(char **items, int cols) {
  for (int i = 0; i < cols; i++)
    if (items[i]) return false;
  return true;
}

static void print_item (const char *item, char justif, int fwidth, int margin) {
  int nchars = utf8_length(item);
  int padding = fwidth - nchars;
  if (padding > 0) 
    switch (justif) {
      case 'r': printf("%*s", padding, ""); break;
      case 'c': printf("%*s", padding / 2, ""); break;
      default: break;
    }

  /*
    Field width is in characters.  How many bytes are used by the
    first 'fwidth' characters in the UTF-8 string 'item'?
  
    N.B. utf8_width() is a hack and doesn't know about combining chars
    or zero-width chars.
  */
  int nbytes = utf8_width(item, fwidth);
  printf("%.*s", nbytes, item);

  if (padding > 0)
    switch (justif) {
      case 'l': printf("%*s", padding, ""); break;
      case 'c': printf("%*s", (padding + 1) / 2, ""); break;
      default: break;
    }

  printf("%*s", margin, "");
}

static struct Span *span_starts(DisplayTable *dt, int row, int col) {
  for (int i = 0; i < dt->maxspans; i++)
    if ((dt->spans[i].row == row) && (dt->spans[i].start_col == col))
      return &(dt->spans[i]);
  return NULL;
}

static struct Span *span_covers(DisplayTable *dt, int row, int col) {
  for (int i = 0; i < dt->maxspans; i++)
    if ((dt->spans[i].row == row)
	&& (dt->spans[i].start_col < col)
	&& (dt->spans[i].end_col >= col))
      return &(dt->spans[i]);
  return NULL;
}

  
// Print until an entire row has NULL in every column
void display_table(DisplayTable *dt, int indent) {
  if (!dt) PANIC_NULL();
  if (indent < 0) indent = 0;
  int bytesperbar = (uint8_t) BAR[0] >> 6; // Assumes UTF-8
  int barlength = (dt->width - 2) * bytesperbar;
  
  printf("%*s%s%.*s%s\n", indent, "",
	 bar(LEFT, TOPLINE),
	 barlength, BAR,
	 bar(RIGHT, TOPLINE));

  if (dt->title) {
    printf("%*s%s", indent, "", bar(LEFT, MIDLINE));
    print_item(dt->title,
	       'c',
	       dt->width - 2,
	       0);
    printf("%s\n", bar(RIGHT, MIDLINE));
  }

  struct Span *span;
  for (int row = 0; row < dt->rows; row++) {
    if (all_null(&(dt->items[row * dt->cols]), dt->cols)) continue;
    printf("%*s%s%*s", indent, "", bar(LEFT, MIDLINE), dt->margins[0], "");
    for (int col = 0; col < dt->cols; col++) {
      if ((span = span_starts(dt, row, col))) {
	print_item(dt->items[row * dt->cols + col] ?: "",
		   span->justification,
		   span->width,
		   dt->margins[span->end_col+1]);
      } else if (!span_covers(dt, row, col)) {
	print_item(dt->items[row * dt->cols + col] ?: "",
		   dt->justifications[col],
		   dt->colwidths[col],
		   dt->margins[col+1]);
      }
    }
    printf("%s\n", bar(RIGHT, MIDLINE));
  }  

  printf("%*s%s%.*s%s\n", indent, "",
	 bar(LEFT, BOTTOMLINE),
	 barlength, BAR,
	 bar(RIGHT, BOTTOMLINE));
}
