//  -*- Mode: C; -*-                                                       
// 
//  csv.c
// 
//  Copyright (C) Jamie A. Jennings, 2024

#include "csv.h"
#include "utils.h"
#include <limits.h>
#include <assert.h>

int CSVfields(CSVrow *row) {
  if (!row) PANIC_NULL();
  return row->next;
}

// Returns NULL on error, so that caller can report the error with
// more context than we have here.  Returned value is a pointer into
// shared CSVrow structure, so caller must not free it.
char *CSVfield(CSVrow *row, int i) {
  if (!row) PANIC_NULL();
  if ((i < 0) || (i >= row->next)) return NULL;
  return row->fields[i];
}

// Tuning parameter: We allocate memory to hold MIN_COLS in each row,
// but dynamically expand the number of columns as needed.
#define MIN_COLS 50

static CSVrow *new_row(void) {
  CSVrow *row = malloc(sizeof(CSVrow));
  if (!row) PANIC_OOM();
  row->fields = malloc(sizeof(char *) * MIN_COLS);
  if (!row->fields) PANIC_OOM();
  row->next = 0;
  row->capacity = MIN_COLS;
  return row;
}

static void CSV_add(CSVrow *row, char *start, char *end) {
  if (row->next == row->capacity) {
    if (row->capacity > INT_MAX / 2) 
      PANIC("Too many columns in CSV row %d", row->capacity);
    int newcap = 2 * row->capacity;
    char **tmp = realloc(row->fields, sizeof(char *) * newcap);
    if (!tmp) PANIC_OOM();
    row->fields = tmp;
    row->capacity = newcap;
  }
  assert(row->capacity > row->next);
  char *tmp = strndup(start, (end - start));
  if (!tmp) PANIC_OOM();
  row->fields[row->next] = tmp;
  row->next++;
}

static bool quotep(char *p) {
  return *p && (*p == '"');
}

static bool newlinep(char *p) {
  return *p && (*p == '\n');
}

static bool commap(char *p) {
  return *p && (*p == ',');
}

// Returns the number of the field that failed to parse.  Fields are
// numbered starting with 1, so 0 means no errors.
static int parse_CSVrow(char *line, CSVrow *row) {
  if (!line) return 1;
  if (!line[0]) return 1;
  char *start, *p = line;
  while (*p != '\0') {
    start = p;
    if (quotep(p)) {
      // Quoted field
      while (p++, !newlinep(p)) {
	if (((*p == '\\') || quotep(p)) && quotep(p+1)) p++;
	if (quotep(p) && !quotep(p+1)) break;
      }
      CSV_add(row, start+1, p);
      p++;
    } else {
      // Bare (unquoted) field
      while (*p && !newlinep(p) && !commap(p)) p++;
      CSV_add(row, start, p);
    }
    if (!newlinep(p) && !commap(p)) {
      int fieldnumber = row->next + 1;
      return fieldnumber;
    }
    if (newlinep(p)) return 0;
    p++;
  }
  return 0;
}

// When return code is zero, '*ptr' is set to a new CSV row that the
// caller must evenutally free.  Else, return code is EITHER the field
// number (1-based) at which the parser failed, or -1 indicating EOF.
//
// Caller has the context needed for a good error message, and can use
// csv_error() to signal it.
//
int read_CSVrow(FILE *f, CSVrow **ptr, char *buf, size_t buflen) {
  char *line = fgets(buf, buflen, f);
  if (!line) return -1;		// EOF
  *ptr = new_row();
  int errfield = parse_CSVrow(line, *ptr);
  if (errfield) {
    free_CSVrow(*ptr);
    *ptr = NULL;
    return errfield;
  }
  return 0;			// Success
}

void free_CSVrow(CSVrow *row) {
  if (!row) return;
  for (int i = 0; i < row->next; i++)
    free(row->fields[i]);
  free(row);
}

// 'colnum' should be a non-zero return value from read_CSVrow().
void csv_error(char *input, int lineno,
	       const char *desc,
	       int colnum,
	       char *buf, size_t buflen) {
  buf[buflen-1] = '\0';		// defensive move
  // Cut first newline out of 'buf' for better printing
  char *p = buf;
  while(*p && !newlinep(p)) p++;
  *p = '\0';
  if (colnum == -1)
    ERROR("CSV read error in file %s at line %d: unexpected EOF",
	  input, lineno);
  else
    ERROR("CSV read error in file %s at line %d: no %s in col %d\n"
	  "Data: %s", input, lineno, desc, colnum, p);
}

// -----------------------------------------------------------------------------
// Output file (raw data, per timed run) CONTROLLED BY Headers[]
// -----------------------------------------------------------------------------

#define WRITEFIELD(fc, fmt, value, lastfc) do {		\
    fprintf(f, fmt, value);				\
    fputc(((fc) == (lastfc - 1)) ? '\n' : ',', f);	\
  } while (0)

#define WRITEHEADER(fc, header, lastfc) do {		\
    fprintf(f, "%s", (header));				\
    fputc(((fc) == (lastfc - 1)) ? '\n' : ',', f);	\
  } while (0)

void write_header(FILE *f) {
  for (FieldCode fc = 0; fc < F_RAWNUMEND; fc++)
    WRITEHEADER(fc, Header[fc], F_RAWNUMEND);
  fflush(f);
}

#define GETFIELD(fc) (get_int64(usage, idx, (fc)))

void write_line(FILE *f, Usage *usage, int idx) {
  char *escaped_cmd = escape_csv(get_string(usage, idx, F_CMD));
  char *shell_cmd = escape_csv(get_string(usage, idx, F_SHELL));

  WRITEFIELD(F_CMD, "%s", escaped_cmd, F_RAWNUMEND);
  WRITEFIELD(F_SHELL, "%s", shell_cmd, F_RAWNUMEND);
  for (FieldCode fc = F_RAWNUMSTART; fc < F_RAWNUMEND; fc++) {
    WRITEFIELD(fc, INT64FMT, GETFIELD(fc), F_RAWNUMEND);
  }
  
  // User time in microseconds
  // System time in microseconds
  // Max RSS in kibibytes
  //   "ru_maxrss (since Linux 2.6.32) This is the maximum resident
  //   set size used (in kilobytes)."
  //   When the above was written, a kilobyte meant 1024 bytes.
  // Page reclaims (count):
  //   The number of page faults serviced without any I/O activity; here
  //   I/O activity is avoided by “reclaiming” a page frame from the
  //   list of pages awaiting reallocation.
  // Page faults (count):
  //   The number of page faults serviced that required I/O activity.
  // Voluntary context switches:
  //   The number of times a context switch resulted due to a process
  //   voluntarily giving up the processor before its time slice was
  //   completed (e.g. to await availability of a resource).
  // Involuntary context switches:
  //   The number of times a context switch resulted due to a higher
  //   priority process becoming runnable or because the current process
  //   exceeded its time slice.
  // Elapsed wall clock time in microseconds

  fflush(f);
  free(escaped_cmd);
  free(shell_cmd);
}

// -----------------------------------------------------------------------------
// Summary statistics file
// -----------------------------------------------------------------------------

#define XSUMMARYFields(X)		\
  X(S_CMD, "Command")			\
  X(S_SHELL, "Shell")			\
  X(S_RUNS, "Runs (ct)")		\
  X(S_FAILED, "Failed (ct)")		\
  X(S_TOTALMODE, "Total mode (μs)")	\
  X(S_TOTALMIN,  "Total min (μs)")	\
  X(S_TOTALQ1, "Total Q1 (μs)")	        \
  X(S_TOTALMED, "Total median (μs)")	\
  X(S_TOTALQ3, "Total Q3 (μs)")	        \
  X(S_TOTALP95, "Total p95 (μs)")	\
  X(S_TOTALP99, "Total p99 (μs)")	\
  X(S_TOTALMAX, "Total max (μs)")	\
  X(S_USERMODE, "User mode (μs)")	\
  X(S_USERMIN, "User min (μs)")		\
  X(S_USERQ1, "User Q1 (μs)")	        \
  X(S_USERMED, "User median (μs)")	\
  X(S_USERQ3, "User Q3 (μs)")	        \
  X(S_USERP95, "User p95 (μs)")		\
  X(S_USERP99, "User p99 (μs)")		\
  X(S_USERMAX, "User max (μs)")		\
  X(S_SYSTEMMODE, "System mode (μs)")	\
  X(S_SYSTEMMIN, "System min (μs)")	\
  X(S_SYSTEMQ1, "System Q1 (μs)")	\
  X(S_SYSTEMMED, "System median (μs)")	\
  X(S_SYSTEMQ3, "System Q3 (μs)")	\
  X(S_SYSTEMP95, "System p95 (μs)")	\
  X(S_SYSTEMP99, "System p99 (μs)")	\
  X(S_SYSTEMMAX,"System max (μs)")		\
  X(S_MAXRSSMODE, "Max RSS mode (bytes)")	\
  X(S_MAXRSSMIN, "Max RSS min (bytes)")		\
  X(S_MAXRSSQ1, "Max RSS Q1 (bytes)")	\
  X(S_MAXRSSMED, "Max RSS median (bytes)")	\
  X(S_MAXRSSQ3, "Max RSS Q3 (bytes)")	\
  X(S_MAXRSSP95, "Max RSS p95 (bytes)")		\
  X(S_MAXRSSP99, "Max RSS p99 (bytes)")		\
  X(S_MAXRSSMAX, "Max RSS max (bytes)")		\
  X(S_VCSWMODE, "Vol Ctx Sw mode (μs)")		\
  X(S_VCSWMIN, "Vol Ctx Sw min (ct)")		\
  X(S_VCSWQ1, "Vol Ctx Sw Q1 (ct)")	        \
  X(S_VCSWMED, "Vol Ctx Sw median (ct)")	\
  X(S_VCSWQ3, "Vol Ctx Sw Q3 (ct)")	        \
  X(S_VCSWP95, "Vol Ctx Sw p95 (μs)")		\
  X(S_VCSWP99, "Vol Ctx Sw p99 (μs)")		\
  X(S_VCSWMAX, "Vol Ctx Sw max (ct)")		\
  X(S_ICSWMODE, "Invol Ctx Sw mode (ct)")	\
  X(S_ICSWMIN, "Invol Ctx Sw min (ct)")		\
  X(S_ICSWQ1, "Invol Ctx Sw Q1 (ct)")	        \
  X(S_ICSWMED, "Invol Ctx Sw median (ct)")	\
  X(S_ICSWQ3, "Invol Ctx Sw Q3 (ct)")	        \
  X(S_ICSWP95, "Invol Ctx Sw p95 (ct)")		\
  X(S_ICSWP99, "Invol Ctx Sw p99 (ct)")		\
  X(S_ICSWMAX, "Invol Ctx Sw max (ct)")		\
  X(S_TCSWMODE, "Total Ctx Sw mode (ct)")	\
  X(S_TCSWMIN, "Total Ctx Sw min (ct)")		\
  X(S_TCSWQ1, "Total Ctx Sw Q1 (ct)")	        \
  X(S_TCSWMED, "Total Ctx Sw median (ct)")	\
  X(S_TCSWQ3, "Total Ctx Sw Q3 (ct)")	        \
  X(S_TCSWP95, "Total Ctx Sw p95 (ct)")		\
  X(S_TCSWP99, "Total Ctx Sw p99 (ct)")		\
  X(S_TCSWMAX, "Total Ctx Sw max (ct)")	        \
  X(S_WALLMODE, "Wall mode (μs)")		\
  X(S_WALLMIN, "Wall min (μs)")		        \
  X(S_WALLQ1, "Wall Q1 (μs)")		        \
  X(S_WALLMED, "Wall median (μs)")		\
  X(S_WALLQ3, "Wall Q3 (μs)")		        \
  X(S_WALLP95, "Wall p95 (μs)")		        \
  X(S_WALLP99, "Wall p99 (μs)")		        \
  X(S_WALLMAX, "Wall max (μs)")			\
  X(S_LAST,    "SENTINEL")

#define FIRST(a, b) a,
typedef enum { XSUMMARYFields(FIRST) } SummaryFieldCode;
#undef FIRST
#define SECOND(a, b) b,
const char *SummaryHeader[] = {XSUMMARYFields(SECOND) NULL};
#undef SECOND

void write_summary_header(FILE *f) {
  for (SummaryFieldCode fc = S_CMD; fc < S_LAST; fc++)
    WRITEHEADER(fc, SummaryHeader[fc], S_LAST);
  fflush(f);
}

#define MAYBE(write, fc, fmt, value, lastfc) do {    \
    if (value < 0)				     \
      fputc(',', f);				     \
    else					     \
      write(fc, fmt, value, lastfc);		     \
  } while (0)

void write_summary_line(FILE *f, Summary *s) {
  char *escaped_cmd = escape_csv(s->cmd);
  char *shell_cmd = escape_csv(config.shell);
  WRITEFIELD(S_CMD, "%s", escaped_cmd, S_LAST);
  WRITEFIELD(S_SHELL, "%s", shell_cmd, S_LAST);
  WRITEFIELD(S_RUNS, "%d", s->runs, S_LAST);
  WRITEFIELD(S_FAILED, "%d", s->fail_count, S_LAST);
  WRITEFIELD(S_TOTALMODE, INT64FMT, s->total.mode, S_LAST);
  WRITEFIELD(S_TOTALMIN, INT64FMT, s->total.min, S_LAST);
  MAYBE(WRITEFIELD, S_TOTALQ1, INT64FMT, s->total.Q1, S_LAST);
  WRITEFIELD(S_TOTALMED, INT64FMT, s->total.median, S_LAST);
  MAYBE(WRITEFIELD, S_TOTALQ3, INT64FMT, s->total.Q3, S_LAST);
  MAYBE(WRITEFIELD, S_TOTALP95, INT64FMT, s->total.pct95, S_LAST);
  MAYBE(WRITEFIELD, S_TOTALP99, INT64FMT, s->total.pct99, S_LAST);
  WRITEFIELD(S_TOTALMAX, INT64FMT, s->total.max, S_LAST);
  WRITEFIELD(S_USERMODE, INT64FMT, s->user.mode, S_LAST);
  WRITEFIELD(S_USERMIN, INT64FMT, s->user.min, S_LAST);
  MAYBE(WRITEFIELD, S_USERQ1, INT64FMT, s->user.Q1, S_LAST);
  WRITEFIELD(S_USERMED, INT64FMT, s->user.median, S_LAST);
  MAYBE(WRITEFIELD, S_USERQ3, INT64FMT, s->user.Q3, S_LAST);
  MAYBE(WRITEFIELD, S_USERP95, INT64FMT, s->user.pct95, S_LAST);
  MAYBE(WRITEFIELD, S_USERP99, INT64FMT, s->user.pct99, S_LAST);
  WRITEFIELD(S_USERMAX, INT64FMT, s->user.max, S_LAST);
  WRITEFIELD(S_SYSTEMMODE, INT64FMT, s->system.mode, S_LAST);
  WRITEFIELD(S_SYSTEMMIN, INT64FMT, s->system.min, S_LAST);
  MAYBE(WRITEFIELD, S_SYSTEMQ1, INT64FMT, s->system.Q1, S_LAST);
  WRITEFIELD(S_SYSTEMMED, INT64FMT, s->system.median, S_LAST);
  MAYBE(WRITEFIELD, S_SYSTEMQ3, INT64FMT, s->system.Q3, S_LAST);
  MAYBE(WRITEFIELD, S_SYSTEMP95, INT64FMT, s->system.pct95, S_LAST);
  MAYBE(WRITEFIELD, S_SYSTEMP99, INT64FMT, s->system.pct99, S_LAST);
  WRITEFIELD(S_SYSTEMMAX, INT64FMT, s->system.max, S_LAST);
  WRITEFIELD(S_MAXRSSMODE, INT64FMT, s->maxrss.mode, S_LAST);
  WRITEFIELD(S_MAXRSSMIN, INT64FMT, s->maxrss.min, S_LAST);
  MAYBE(WRITEFIELD, S_MAXRSSQ1, INT64FMT, s->maxrss.Q1, S_LAST);
  WRITEFIELD(S_MAXRSSMED, INT64FMT, s->maxrss.median, S_LAST);
  MAYBE(WRITEFIELD, S_MAXRSSQ3, INT64FMT, s->maxrss.Q3, S_LAST);
  MAYBE(WRITEFIELD, S_MAXRSSP95, INT64FMT, s->maxrss.pct95, S_LAST);
  MAYBE(WRITEFIELD, S_MAXRSSP99, INT64FMT, s->maxrss.pct99, S_LAST);
  WRITEFIELD(S_MAXRSSMAX, INT64FMT, s->maxrss.max, S_LAST);
  WRITEFIELD(S_VCSWMODE, INT64FMT, s->vcsw.mode, S_LAST);
  WRITEFIELD(S_VCSWMIN, INT64FMT, s->vcsw.min, S_LAST);
  MAYBE(WRITEFIELD, S_VCSWQ1, INT64FMT, s->vcsw.Q1, S_LAST);
  WRITEFIELD(S_VCSWMED, INT64FMT, s->vcsw.median, S_LAST);
  MAYBE(WRITEFIELD, S_VCSWQ3, INT64FMT, s->vcsw.Q3, S_LAST);
  MAYBE(WRITEFIELD, S_VCSWP95, INT64FMT, s->vcsw.pct95, S_LAST);
  MAYBE(WRITEFIELD, S_VCSWP99, INT64FMT, s->vcsw.pct99, S_LAST);
  WRITEFIELD(S_VCSWMAX, INT64FMT, s->vcsw.max, S_LAST);
  WRITEFIELD(S_ICSWMODE, INT64FMT, s->icsw.mode, S_LAST);
  WRITEFIELD(S_ICSWMIN, INT64FMT, s->icsw.min, S_LAST);
  MAYBE(WRITEFIELD, S_ICSWQ1, INT64FMT, s->icsw.Q1, S_LAST);
  WRITEFIELD(S_ICSWMED, INT64FMT, s->icsw.median, S_LAST);
  MAYBE(WRITEFIELD, S_ICSWQ3, INT64FMT, s->icsw.Q3, S_LAST);
  MAYBE(WRITEFIELD, S_ICSWP95, INT64FMT, s->icsw.pct95, S_LAST);
  MAYBE(WRITEFIELD, S_ICSWP99, INT64FMT, s->icsw.pct99, S_LAST);
  WRITEFIELD(S_ICSWMAX, INT64FMT, s->icsw.max, S_LAST);
  WRITEFIELD(S_TCSWMODE, INT64FMT, s->tcsw.mode, S_LAST);
  WRITEFIELD(S_TCSWMIN, INT64FMT, s->tcsw.min, S_LAST);
  MAYBE(WRITEFIELD, S_TCSWQ1, INT64FMT, s->tcsw.Q1, S_LAST);
  WRITEFIELD(S_TCSWMED, INT64FMT, s->tcsw.median, S_LAST);
  MAYBE(WRITEFIELD, S_TCSWQ3, INT64FMT, s->tcsw.Q3, S_LAST);
  MAYBE(WRITEFIELD, S_TCSWP95, INT64FMT, s->tcsw.pct95, S_LAST);
  MAYBE(WRITEFIELD, S_TCSWP99, INT64FMT, s->tcsw.pct99, S_LAST);
  WRITEFIELD(S_TCSWMAX, INT64FMT, s->tcsw.max, S_LAST);
  WRITEFIELD(S_WALLMODE, INT64FMT, s->wall.mode, S_LAST);
  WRITEFIELD(S_WALLMIN, INT64FMT, s->wall.min, S_LAST);
  MAYBE(WRITEFIELD, S_WALLQ1, INT64FMT, s->wall.Q1, S_LAST);
  WRITEFIELD(S_WALLMED, INT64FMT, s->wall.median, S_LAST);
  MAYBE(WRITEFIELD, S_WALLQ3, INT64FMT, s->wall.Q3, S_LAST);
  MAYBE(WRITEFIELD, S_WALLP95, INT64FMT, s->wall.pct95, S_LAST);
  MAYBE(WRITEFIELD, S_WALLP99, INT64FMT, s->wall.pct99, S_LAST);
  WRITEFIELD(S_WALLMAX, INT64FMT, s->wall.max, S_LAST);
  fflush(f);
  free(escaped_cmd);
  free(shell_cmd);
}

// -----------------------------------------------------------------------------
// Hyperfine-format file
// -----------------------------------------------------------------------------

// Differences with Hyperfine
//
// - The mode of the total time replaces the mean in the output file
//   because the mode is a better representation of skewed distributions
//
// - Median values replace mean values time because median is more
//   representative of skewed distributions
//
// - Stddev is omitted because its value is not supported for
//   distributions that badly fail a normality test

void write_hf_header(FILE *f) {
  fprintf(f, "command,mode,iqr,median,user,system,min,max\n");
  fflush(f);
}

void write_hf_line(FILE *f, Summary *s) {
  const int N = 8;
  const double million = MICROSECS;
  int i = 0;
  // Command
  WRITEFIELD(i++, "%s", *(s->cmd) ? s->cmd : config.shell, N);
  // Median total time (more useful than mean, not as good as mode)
  WRITEFIELD(i++, "%f", (double) s->total.mode / million, N);
  // Stddev replaced with IQR
  WRITEFIELD(i++, "%f", (double) (s->total.Q3 - s->total.Q1) / million, N);
  // Mode total time
  WRITEFIELD(i++, "%f", (double) s->total.median / million, N);
  // User time in seconds as double 
  WRITEFIELD(i++, "%f", (double) s->user.median / million, N);
  // System time in seconds as double
  WRITEFIELD(i++, "%f", (double) s->system.median / million, N);
  // Min total time in seconds as double 
  WRITEFIELD(i++, "%f", (double) s->total.min / million, N);
  // Max total time in seconds as double
  WRITEFIELD(i++, "%f", (double) s->total.max / million, N);
  fflush(f);
}




