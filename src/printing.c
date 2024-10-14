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

  dt->justifications =
    column_justifications(justifications, cols);
  
  dt->orig_justif = justifications;

  size_t len = strlen(justifications);
  dt->leftborder = (dt->orig_justif[0] == '|');
  dt->rightborder = (dt->orig_justif[len-1] == '|');
  dt->borders = dt->leftborder + dt->rightborder;
  
  for (int i = 0; i < rows * cols; i++)
    dt->items[i] = NULL;

  int total = 0;
  for (int i = 0; i < cols; i++) {
    dt->colwidths[i] = colwidths[i];
    dt->margins[i] = margins[i];
    total += colwidths[i] + margins[i];
  }
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
    printf("New display table: width = %d, cols = %d\n", dt->width, dt->cols);
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
  free(dt);
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
  if ((start_col < 0) || (start_col >= dt->cols))
    PANIC("No such column (%d) in display table", start_col);
  if ((end_col < 0)
      || ((start_col == 0) && (end_col > dt->cols))
      || ((start_col > 0) && (end_col >= dt->cols)))
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
  for (idx = 0; idx < dt->maxspans; idx++)
    if (dt->spans[idx].row == -1) break;
  if (idx == dt->maxspans)
    PANIC("Too many spans in display table (limit is %d)", dt->maxspans);

  int spanwidth = 0;

  // Special case: To span the entire table, we need the entire table
  // width except for the left and right borders.  The sum of the col
  // widths plus the sum of the margins before each column may total
  // up to a smaller value.  (I.e. there may be unused space after the
  // last column, before the right border.)
  if ((start_col == 0) && (end_col == dt->cols)) {
    spanwidth = dt->width - dt->borders;
  } else {
    spanwidth = dt->colwidths[start_col];
    for (int i = start_col + 1; i <= end_col; i++) 
      spanwidth += dt->margins[i] + dt->colwidths[i];
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
  const char *decor[8] = { "┌", "┐",
			   "│", "│",
			   "├", "┤",
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

void display_table_hline(DisplayTable *dt, int row) {
  display_table_span(dt, row, 0, dt->cols, '-', "");
}

void display_table_title(DisplayTable *dt, int row, char justif, const char *title) {
  display_table_span(dt, row, 0, dt->cols, justif, "%s", title);
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

  
// Print until an entire row has NULL in every column
void display_table(DisplayTable *dt, int indent) {
  if (!dt) PANIC_NULL();
  if (indent < 0) indent = 0;
  int bytesperbar = (uint8_t) BAR[0] >> 6; // Assumes UTF-8
  int barlength = (dt->width - dt->borders) * bytesperbar;
  struct DisplaySpan *span;

  if (dt->leftborder)
    printf("%*s%s", indent, "", bar(LEFT, TOPLINE));
  printf("%.*s", barlength, BAR);
  if (dt->rightborder)
    printf("%s", bar(RIGHT, TOPLINE));
  printf("\n");
  
  for (int row = 0; row < dt->rows; row++) {
    // Skip any row that contains only NULLs
    if (all_null(&(dt->items[row * dt->cols]), dt->cols)) continue;

    // Check for this special case: the span covers entire table (all
    // columns and margins)
    if ((span = span_starts(dt, row, 0))) {
      if (span->end_col == dt->cols) {
	// Yes, the special case!
	if (span->justification == '-') {
	  // The span is a horizontal line (hline)
	  printf("%*s%s", indent, "", dt->leftborder ? bar(LEFT, MIDLINETEE) : "");
	  printf("%.*s", barlength, BAR);
	  printf("%s\n", dt->rightborder ? bar(RIGHT, MIDLINETEE) : "");
	} else {
	  // The span is text
	  printf("%*s%s", indent, "", dt->leftborder ? bar(LEFT, MIDLINE) : "");
	  print_item(dt->items[row * dt->cols + 0] ?: "",
		     span->justification,
		     span->width);
	  printf("%s\n", dt->rightborder ? bar(RIGHT, MIDLINE) : "");
	}
	// Continue with the next row
	continue;
      }
    } // End of special case

    // Either we do not have a span, or the span does not cover all
    // columns and margins. The loop below handles the printing.
    printf("%*s%s", indent, "", dt->leftborder ? bar(LEFT, MIDLINE) : "");

    for (int col = 0; col < dt->cols; col++) {
      if ((span = span_starts(dt, row, col))) {
	  printf("%*s", dt->margins[col], "");
	  print_item(dt->items[row * dt->cols + col] ?: "",
		     span->justification,
		     span->width);
      } else if (!span_covers(dt, row, col)) {
	  printf("%*s", dt->margins[col], "");
	  print_item(dt->items[row * dt->cols + col] ?: "",
		     dt->justifications[col],
		     dt->colwidths[col]);
      }
    }
    printf("%*s%s\n", dt->rightpad, "", dt->rightborder ? bar(RIGHT, MIDLINE) : "");
  }  

  if (dt->leftborder)
    printf("%*s%s", indent, "", bar(LEFT, BOTTOMLINE));
  printf("%.*s", barlength, BAR);
  if (dt->rightborder)
    printf("%s", bar(RIGHT, BOTTOMLINE));
  printf("\n");
}
