//  -*- Mode: C; -*-                                                       
// 
//  printing.c
// 
//  COPYRIGHT (c) Jamie A. Jennings, 2024

#include "printing.h"
#include "utils.h"
#include <assert.h>
#include <stdarg.h> 		// __VA_ARGS__ (var args)

#define TABLEDEBUG 0

#define MAXROWS 1000

#define LEFT       0
#define RIGHT      1
#define TOPLINE    0
#define MIDLINE    1
#define MIDLINETEE 2
#define BOTTOMLINE 3

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

// -----------------------------------------------------------------------------
// Printing tables of data
// -----------------------------------------------------------------------------

// Extract one justification char per column, ignoring an optional
// leading '|' and an optional trailing '|' (which indicate borders)
static char *column_justifications(const char *justif, int cols) {
  int i = 0;
  if (justif[i] == '|') i++;
  for (int j = 0; j < cols; j++)
    if ((justif[i] == 'l') || (justif[i] == 'c') || (justif[i] == 'r')) {
      i++;
    } else {
      if (!justif[i] || (justif[i] == '|'))
	PANIC("Not enough justification characters for table with %d columns", cols);
      else 
	PANIC("Invalid justification '%c' in display table", justif[i]);
    }
  if (justif[i] == '|') i++;
  if (justif[i])
    PANIC("Too many justification characters for table with %d columns", cols);

  // C's lack of proper strings is so tedious
  char *one_per_column = malloc(cols+1);
  memcpy(one_per_column, justif + (justif[0] == '|'), cols);
  one_per_column[cols] = '\0';
  return one_per_column;
}

DisplayTable *new_display_table(int width,
				int cols,
				int colwidths[],
				int margins[],
				const char *justifications,
				bool top,
				bool bottom) {
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

  dt->justifications =
    column_justifications(justifications, cols);
  
  dt->orig_justif = justifications;

  size_t len = strlen(justifications);
  dt->leftborder = (dt->orig_justif[0] == '|');
  dt->rightborder = (dt->orig_justif[len-1] == '|');
  dt->borders = dt->leftborder + dt->rightborder;
  dt->topborder = top;
  dt->bottomborder = bottom;
  
  for (int i = 0; i < rows * cols; i++)
    dt->items[i] = NULL;

  int total = 0;
  for (int i = 0; i < cols; i++) {
    if (colwidths[i] == END)
      PANIC("Not enough colwidths (%d) for table with %d columns", i, cols);
    dt->colwidths[i] = colwidths[i];
    if (margins[i] == END)
      PANIC("Not enough margins (%d) for table with %d columns", i, cols);
    dt->margins[i] = margins[i];
    total += colwidths[i] + margins[i];
  }
  if (colwidths[cols] != END)
      PANIC("Too many colwidths for table with %d columns", cols);
  if (margins[cols] != END)
    PANIC("Too many margins for table with %d columns", cols);

  dt->rightpad = width - dt->borders - total;

  if (dt->rightpad < 0)
    PANIC("Columns, margins, and table borders total %d chars,"
	  " more than table width of %d", total + dt->borders, width);

  // Total number of spans in a table is artificially limited to (on
  // average) 2 per row.
  dt->maxspans = 2 * MAXROWS;
  dt->spans = malloc(dt->maxspans * sizeof(struct DisplaySpan));
  if (!dt->spans) PANIC_OOM();

  // Initialize each span to "unused" using row number of -1
  for (int i = 0; i < dt->maxspans; i++) dt->spans[i].row = -1;

  if (TABLEDEBUG) {
    printf("New display table: width = %d, cols = %d, top = %s, bottom = %s, borders = %d\n",
	   dt->width, dt->cols,
	   dt->topborder ? "true" : "false",
	   dt->bottomborder ? "true" : "false",
	   dt->borders);
    printf("Cols   ");
    for (int i = 0; i < dt->cols; i++)
      printf("     %2d ", i);
    printf("\nWidths ");
    for (int i = 0; i < dt->cols; i++)
      printf("(%2d) %2d ", dt->margins[i], dt->colwidths[i]);
    printf("(%2d)\n", dt->rightpad);
    printf("Justif ");
    for (int i = 0; i < dt->cols; i++)
      printf("      %c ", dt->justifications[i]);
    printf("\n");
  }
  return dt;
}

void free_display_table(DisplayTable *dt) {
  if (!dt) return;
  free(dt->colwidths);
  free(dt->margins);
  free(dt->justifications);
  for (int i = 0; i < dt->rows * dt->cols; i++) free(dt->items[i]);
  free(dt->items);
  free(dt->spans);
  free(dt);
}

// Kinds of spans

// Includes the left margin, before col 0
static bool leftspan(int start_col, int end_col, int ncols) {
  return (start_col == -1) && (end_col > start_col) && (end_col < ncols);
}

// Any span that does not include the left or right table margins,
// including a span of a single column, which can be used to
// temporarily change the justification for that one entry
static bool midspan(int start_col, int end_col, int ncols) {
  return (start_col >= 0) && (end_col >= start_col) && (end_col < ncols);
}

// Includes the right margin, after the last col, tbl->cols or 'ncols' here
static bool rightspan(int start_col, int end_col, int ncols) {
  return (start_col >= 0) && (end_col > start_col) && (end_col == ncols);
}

// Are we looking at a single column, i.e. not a span?
static bool nospan(int start_col, int end_col, int ncols) {
  return (start_col >= 0) && (end_col == start_col) && (end_col < ncols);
}

// Spans the entire table
static bool fullspan(int start_col, int end_col, int ncols) {
  return (start_col == -1) && (end_col == ncols);
}

//
// NOTE: Takes ownership of 'buf'
//
// Normal insert: start_col == end_col
// DisplaySpan (inclusive range of cols): start_col < end_col
// DisplaySpan (entire table): start_col == 0, end_col == dt->cols
// 
static void insert(DisplayTable *dt,
		   int row,
		   int start_col, int end_col,
		   char justification,
		   char *buf) {
  if (!dt) PANIC_NULL();
  if ((row < 0) || (row >= dt->rows))
    PANIC("No such row (%d) in display table", row);
  // There are 4 types of spans (and "no span")
  if (!nospan(start_col, end_col, dt->cols) &&
      !fullspan(start_col, end_col, dt->cols) &&
      !leftspan(start_col, end_col, dt->cols) &&
      !midspan(start_col, end_col, dt->cols) &&
      !rightspan(start_col, end_col, dt->cols))      
    PANIC("Invalid span (%d, %d) in display table with %d columns",
	  start_col, end_col, dt->cols);

  // For leftspans, the item to display is stored at (row, 0)
  int idx = row * dt->cols + ((start_col == -1) ? 0 : start_col);

  // The display table OWNS the items (strings), so if the caller is
  // replacing an item, we can (must) free the old one
  if (dt->items[idx]) free(dt->items[idx]);
  dt->items[idx] = buf;

  // If this table entry does NOT span multiple columns, AND we are
  // not creating a one-column span in order to change the
  // justification, then we are done.  A justification code of NUL
  // means "no change from the default".
  if (nospan(start_col, end_col, dt->cols) &&
      ((justification == '\0') || (justification == dt->justifications[start_col])))
    return;

  // Find the next available entry in the span list
  for (idx = 0; idx < dt->maxspans; idx++)
    if (dt->spans[idx].row == -1) break;
  if (idx == dt->maxspans)
    PANIC("Too many spans in display table (limit is %d)", dt->maxspans);

  int spanwidth = 0;

  // Add the column widths, except for last column of the span
  for (int i = (start_col == -1) ? 0 : start_col; i < end_col; i++) 
    spanwidth += dt->colwidths[i];
  // Plus the width of the left margin, if this is a leftspan
  if (start_col == -1)
    spanwidth += dt->margins[0];
  // Plus the margins within the span
  for (int i = (start_col == -1) ? 1 : start_col + 1; i < end_col; i++) 
    spanwidth += dt->margins[i];
  // Plus the margin and width of the last column of the span, unless
  // the end_col is dt->cols
  if (end_col < dt->cols)
    spanwidth += dt->margins[end_col] + dt->colwidths[end_col];
  else
    spanwidth += dt->rightpad;

  if (TABLEDEBUG) {
    if (fullspan(start_col, end_col, dt->cols)) {
      if (spanwidth != dt->width - dt->borders)
	PANIC("Table debugging: calculated span width (%d) should be %d",
	      spanwidth, dt->width - dt->borders);
    }
  }

  dt->spans[idx] = (struct DisplaySpan) {.row = row,
					 .start_col = start_col,
					 .end_col = end_col,
					 .width = spanwidth,
					 .justification = justification};
  if (TABLEDEBUG) {
    int utfwidth = utf8_width(buf, spanwidth);
    printf("Inserted span at row %d, cols (%d, %d), "
	   "width %d, justif '%c', strlen %zu, utf8 width %d\n",
	   row, start_col, end_col,
	   spanwidth, justification, strlen(buf), utfwidth);
    printf("%s\n", buf);
  }
}

__attribute__ ((format (printf, 4, 5)))
void display_table_set(DisplayTable *dt, int row, int col, const char *fmt, ...) {
  va_list things;
  char *buf;
  if (!fmt || !*fmt)
    PANIC("Null or empty format string passed to display_table_set()");
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
  if (!fmt || !*fmt)
    PANIC("Null or empty format string passed to display_table_span()");
  va_start(things, fmt);
  if (vasprintf(&buf, fmt, things) == -1) PANIC_OOM();
  va_end(things);
  insert(dt, row, start_col, end_col, justification, buf);
}

__attribute__ ((format (printf, 4, 5)))
void display_table_fullspan(DisplayTable *dt,
			    int row,
			    char justification,
			    const char *fmt, ...) {
  va_list things;
  char *buf;
  if (!fmt || !*fmt)
    PANIC("Null or empty format string passed to display_table_fullspan()");
  va_start(things, fmt);
  if (vasprintf(&buf, fmt, things) == -1) PANIC_OOM();
  va_end(things);
  insert(dt, row, -1, dt->cols, justification, buf);
}

static const char *bar(int side, int line) {
  const char *decor[8] = { "╭", "╮",
			   "│", "│",
			   "├", "┤",
			   "╰", "╯" };
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

void display_table_hline(DisplayTable *dt, int row) {
  display_table_span(dt, row, 0, dt->cols, '-', "%s", "");
}

void display_table_blankline(DisplayTable *dt, int row) {
  display_table_span(dt, row, -1, dt->cols, 'l', "%s", "");
}

static bool all_null(char **items, int cols) {
  for (int i = 0; i < cols; i++)
    if (items[i]) return false;
  return true;
}

static void print_item(const char *item, char justif, int fwidth) {
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
}

static struct DisplaySpan *span_starts(DisplayTable *dt, int row, int col) {
  for (int i = 0; i < dt->maxspans; i++)
    if ((dt->spans[i].row == row) && (dt->spans[i].start_col == col))
      return &(dt->spans[i]);
  return NULL;
}

static struct DisplaySpan *span_covers(DisplayTable *dt, int row, int col) {
  for (int i = 0; i < dt->maxspans; i++)
    if ((dt->spans[i].row == row)
	&& (dt->spans[i].start_col < col)
	&& (dt->spans[i].end_col >= col))
      return &(dt->spans[i]);
  return NULL;
}

static void print_row(DisplayTable *dt, int row) {
  struct DisplaySpan *span;
  bool leftpad = true;		// Pad the left side with dt->margins[0]
  bool rightpad = true;		// Pad the right side with dt->rightpad

  // Is the first item to display a span that includes the left margin?
  if ((span = span_starts(dt, row, -1))) {
    // Is it a full span of the entire table?
    if (fullspan(span->start_col, span->end_col, dt->cols)) {
      // Yes, span of entire table.  But what kind?
      if (span->justification == '-') {
	// The span is a horizontal line (hline)
	int bytesperbar = (uint8_t) BAR[0] >> 6; // Assumes UTF-8
	int barlength = (dt->width - dt->borders) * bytesperbar;
	printf("%s", dt->leftborder ? bar(LEFT, MIDLINETEE) : "");
	printf("%.*s", barlength, BAR);
	printf("%s\n", dt->rightborder ? bar(RIGHT, MIDLINETEE) : "");
      } else {
	// The span is text
	printf("%s", dt->leftborder ? bar(LEFT, MIDLINE) : "");
	print_item(dt->items[row * dt->cols + 0] ?: "",
		   span->justification,
		   span->width);
	printf("%s\n", dt->rightborder ? bar(RIGHT, MIDLINE) : "");
      }
      // Done with this row, which is a fullspan
      return;
    }
    // Not a full span, but a leftspan
    leftpad = false;
  }

  printf("%s", dt->leftborder ? bar(LEFT, MIDLINE) : "");
  if (leftpad) printf("%*s", dt->margins[0], "");

  for (int col = -1; col < dt->cols; col++) {
    if ((span = span_starts(dt, row, col))) {
      if (col > 0) printf("%*s", dt->margins[col], "");
      print_item(dt->items[row * dt->cols + ((col == -1) ? 0 : col)] ?: "",
		 span->justification,
		 span->width);
      if (rightspan(span->start_col, span->end_col, dt->cols))
	rightpad = false;
    } else if ((col > -1) && !span_covers(dt, row, col)) {
      if (col > 0) printf("%*s", dt->margins[col], "");
      print_item(dt->items[row * dt->cols + col] ?: "",
		 dt->justifications[col],
		 dt->colwidths[col]);
    }
  }
  if (rightpad)
    printf("%*s", dt->rightpad, "");
  printf("%s\n", dt->rightborder ? bar(RIGHT, MIDLINE) : "");
}

// Print until an entire row has NULL in every column
void display_table(DisplayTable *dt, int indent) {
  if (!dt) PANIC_NULL();
  if (indent < 0) indent = 0;
  int bytesperbar = (uint8_t) BAR[0] >> 6; // Assumes UTF-8
  int barlength = (dt->width - dt->borders) * bytesperbar;

  if (dt->topborder) {
    printf("%*s", indent, "");
    if (dt->leftborder)
      printf("%s", bar(LEFT, TOPLINE));
    printf("%.*s", barlength, BAR);
    if (dt->rightborder)
      printf("%s", bar(RIGHT, TOPLINE));
    printf("\n");
  }
  
  for (int row = 0; row < dt->rows; row++) {
    // Skip any row that contains only NULLs
    if (all_null(&(dt->items[row * dt->cols]), dt->cols)) continue;
    printf("%*s", indent, "");
    print_row(dt, row);
  }

  if (dt->bottomborder) {
    printf("%*s", indent, "");
    if (dt->leftborder)
      printf("%s", bar(LEFT, BOTTOMLINE));
    printf("%.*s", barlength, BAR);
    if (dt->rightborder)
      printf("%s", bar(RIGHT, BOTTOMLINE));
    printf("\n");
  }
}
