//  -*- Mode: C; -*-                                                       
// 
//  utils.c
// 
//  COPYRIGHT (c) Jamie A. Jennings, 2024

#include "utils.h"
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <stdarg.h> 		// __VA_ARGS__ (var args)
#include <stdlib.h>		// exit()

#define SECOND(a, b) b,
const char *Header[] = {XFields(SECOND) NULL};
#undef SECOND

// Does not need to be 64 bits, but less code to write this way
int64_t next_batch_number(void) {
  static int64_t previous = 0;
  return ++previous;
}

// Used by the PANIC macro to report internal errors (bugs)
void panic_report(const char *prelude,
	     const char *filename, int lineno,
	     const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "%s %s:%d ", prelude, filename, lineno);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
    fflush(stderr);
}

// Used by BAIL and USAGE macros to report reason for exiting
void error_report(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "%s error: ", progname);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
    fflush(stderr);
}

// -----------------------------------------------------------------------------
// Custom usage struct with accessors and comparators
// -----------------------------------------------------------------------------

Usage *new_usage_array(int initial_capacity) {
  if (initial_capacity < 1) return NULL;
  Usage *usage = malloc(sizeof(Usage));
  if (!usage) PANIC_OOM();
  UsageData *data = calloc(initial_capacity, sizeof(UsageData));
  if (!data) PANIC_OOM();
  usage->next = 0;
  usage->capacity = initial_capacity;
  usage->data = data;
  return usage;
}

static void expand_usage_array(Usage *usage, int newcap) {
  if (!usage) PANIC_NULL();
  if (newcap <= usage->capacity) return;
  UsageData *data = realloc(usage->data, newcap * sizeof(UsageData));
  if (!data) PANIC_OOM();
  usage->capacity = newcap;
  usage->data = data;
}

int usage_next(Usage *usage) {
  if (!usage) PANIC_NULL();
  if (usage->next == usage->capacity)
    expand_usage_array(usage, 2 * usage->capacity);
  int next = usage->next;
  usage->next++;
  return next;
}

void free_usage_array(Usage *usage) {
  if (!usage) return;
  for (int i = 0; i < usage->next; i++) {
    // These fields are strings owned by the usage struct
    free(usage->data[i].cmd);
    free(usage->data[i].shell);
    free(usage->data[i].name);
  }
  free(usage->data);
  free(usage);
}

// Returns pointer to string OWNED BY USAGE STRUCT
char *get_string(Usage *usage, int idx, FieldCode fc) {
  if (!usage) PANIC_NULL();
  if ((idx < 0) || (idx >= usage->next))
    PANIC("Index %d out of range 0..%d", usage->next - 1);
  if (fc == F_CMD) return usage->data[idx].cmd;
  if (fc == F_SHELL) return usage->data[idx].shell;
  if (fc == F_NAME) return usage->data[idx].name;
  PANIC("Non-string field code (%d)", fc);
}

int64_t get_int64(Usage *usage, int idx, FieldCode fc) {
  if (!usage) PANIC_NULL();
  if ((idx < 0) || (idx >= usage->next))
    PANIC("Index %d out of range 0..%d", usage->next - 1);
  if (!FNUMERIC(fc)) PANIC("Invalid int64 field code (%d)", fc);
  return usage->data[idx].metrics[FTONUMERICIDX(fc)];
}

// Struct 'usage' gets a COPY of 'str'
void set_string(Usage *usage, int idx, FieldCode fc, const char *str) {
  if (!usage) PANIC_NULL();
  if ((idx < 0) || (idx >= usage->next))
    PANIC("Index %d out of range 0..%d", usage->next - 1);
  char *dup = strndup(str, MAXCMDLEN);
  if (!dup) PANIC_OOM();
  switch (fc) {
    case F_CMD:
      usage->data[idx].cmd = dup;
      break;
    case F_SHELL:
      usage->data[idx].shell = dup;
      break;
    case F_NAME:
      usage->data[idx].name = dup;
      break;
    default:
      free(dup);
      PANIC("Invalid string field code (%d)", fc);
  }
}

void set_int64(Usage *usage, int idx, FieldCode fc, int64_t val) {
  if (!usage) PANIC_NULL();
  if ((idx < 0) || (idx >= usage->next))
    PANIC("Index %d out of range 0..%d", idx, usage->next - 1);
  if (!FNUMERIC(fc)) PANIC("Invalid int64 field code (%d)", fc);
  usage->data[idx].metrics[FTONUMERICIDX(fc)] = val;
}

// struct rusage accessors

int64_t rmaxrss(struct rusage *ru) {
  return ru->ru_maxrss;
}

int64_t rusertime(struct rusage *ru) {
  return ru->ru_utime.tv_sec * MICROSECS + ru->ru_utime.tv_usec;
}

int64_t rsystemtime(struct rusage *ru) {
  return ru->ru_stime.tv_sec * MICROSECS + ru->ru_stime.tv_usec;
}

int64_t rvcsw(struct rusage *ru) {
  return ru->ru_nvcsw;
}

int64_t ricsw(struct rusage *ru) {
  return ru->ru_nivcsw;
}

int64_t rminflt(struct rusage *ru) {
  return ru->ru_minflt;
}

int64_t rmajflt(struct rusage *ru) {
  return ru->ru_majflt;
}

#define MAKE_COMPARATOR(name, fieldcode)				\
  int name(const void *idx_ptr1,					\
	   const void *idx_ptr2,					\
	   void *context) {						\
    Usage *usage = context;						\
    const int idx1 = *((const int *)idx_ptr1);				\
    const int idx2 = *((const int *)idx_ptr2);				\
    int64_t val1 = get_int64(usage, idx1, fieldcode);			\
    int64_t val2 = get_int64(usage, idx2, fieldcode);			\
    if (val1 > val2) return 1;						\
    if (val1 < val2) return -1;						\
    return 0;								\
  }

MAKE_COMPARATOR(compare_usertime, F_USER)
MAKE_COMPARATOR(compare_systemtime, F_SYSTEM)
MAKE_COMPARATOR(compare_totaltime, F_TOTAL)
MAKE_COMPARATOR(compare_maxrss, F_MAXRSS)
MAKE_COMPARATOR(compare_vcsw, F_VCSW)
MAKE_COMPARATOR(compare_icsw, F_ICSW)
MAKE_COMPARATOR(compare_tcsw, F_TCSW)
MAKE_COMPARATOR(compare_wall, F_WALL)

// The argument order for comparators passed to qsort_r differs
// between linux and macOS.  This is C, where we can't have nice
// things.
typedef struct macOS_qsort_context {
  int (*compare)(const void *, const void *, void *);
  void *context;
} macOS_qsort_context;

#ifndef HAVE_LINUX_QSORT_R
static int macOS_qsort_compare(void *_mqc, const void *a, const void *b) {
  macOS_qsort_context *mqc = _mqc;
  return mqc->compare(a, b, mqc->context);
}
#endif

void sort(void *base, size_t nel, size_t width, 
	  int (*compare)(const void *, const void *, void *),
	  void *context) {
#ifdef HAVE_LINUX_QSORT_R
  qsort_r(base, nel, width, compare, context);
#else
  macOS_qsort_context mqc;
  mqc.compare = compare;
  mqc.context = context;
  qsort_r(base, nel, width, (void *)&mqc, macOS_qsort_compare);
#endif
}

// -----------------------------------------------------------------------------
// Parsing utilities
// -----------------------------------------------------------------------------

static int whitespacep(const char c) {
  return ((c == ' ') || (c == '\t') ||
	  (c == '\n') || (c == '\r'));
}

static const char *skip_whitespace(const char *str) {
  if (!str) return NULL;
  while ((*str != '\0') && whitespacep(*str)) str++;
  return str;
}

static const char *until_whitespace(const char *str) {
  if (!str) return NULL;
  while ((*str != '\0') && !whitespacep(*str)) str++;
  return str;
}

static const char *until_doublequote(const char *str) {
  if (!str) return NULL;
  while ((*str != '\0') && (*str != '\"')) str++;
  return str;
}

static const char *until_singlequote(const char *str) {
  if (!str) return NULL;
  while ((*str != '\0') && (*str != '\'')) str++;
  return str;
}

static int is_quote(const char *p) {
  return p && ((*p == '\'') || (*p == '\"'));
}

// FUTURE: There's a version of this in optable that we may want to
// copy, called read_value().  It handles escaped quotes within a
// quoted string.
static const char *read_arg(const char *p) {
  if (*p == '\"') {
    p = until_doublequote(++p);
    if (!*p) p = NULL;
  } else if (*p == '\'') {
    p = until_singlequote(++p);
    if (!*p) p = NULL;
  } else {
    p = until_whitespace(p);
  }
  return p;
}

void free_arglist(arglist *args) {
  if (!args) return;
  for (size_t i = 0; i < args->next; i++) free(args->args[i]);
  free(args->args);
  free(args);
}

__attribute__((unused))
void print_arglist(arglist *args) {
  if (!args) {
    printf("null arglist\n");
    return;
  }
  for (size_t i = 0; i < args->next; i++)
    printf("[%zu] %s\n", i, args->args[i]);
}

// -----------------------------------------------------------------------------
// Numbers from strings
// -----------------------------------------------------------------------------

bool try_strtoint64(const char *str, int64_t *result) {
  if (!str || !result) PANIC_NULL();
  int count;
  int scancount = sscanf(str, "%" PRId64 "%n", result, &count);
  return ((scancount == 1) && (count == (int) strlen(str)));
}

int64_t strtoint64(const char *str) {
  int64_t value;
  if (!try_strtoint64(str, &value))
    USAGE("Failed to get integer from '%s'", str);
  return value;
}

// For convenience, behaves like strtoint64 when 'end' is NULL.  In
// that usage, caller must ensure 'start' is a NUL-terminated string.
int64_t buftoint64(const char *start, const char *end) {
  if (end && (start >= end)) USAGE("Failed to get float from empty string");
  int64_t value;
  char buf[24];
  size_t len;
  if (!end)
    len = strlen(start);
  else
    len = end - start;
  if (len < 24) {
    memcpy(buf, start, len);
    buf[len] = '\0';
    if (!try_strtoint64(buf, &value))
      USAGE("Failed to get integer from '%s'", buf);
    return value;
  }
  USAGE("Failed to get integer from too-long string (%zu bytes)", len);
}

bool try_strtodouble(const char *str, double *result) {
  if (!str || !result) PANIC_NULL();
  int count;
  int scancount = sscanf(str, "%lf%n", result, &count);
  return ((scancount == 1) && (count == (int) strlen(str)));
}

double strtodouble(const char *str) {
  double value;
  if (!try_strtodouble(str, &value))
    USAGE("Failed to get float from '%s'", str);
  return value;
}

// For convenience, behaves like strtodouble when 'end' is NULL.  In
// that usage, caller must ensure 'start' is a NUL-terminated string.
double buftodouble(const char *start, const char *end) {
  if (end && (start >= end)) USAGE("Failed to get float from empty string");
  double value;
  char buf[100];
  size_t len;
  if (!end)
    len = strlen(start);
  else
    len = end - start;
  if (len < 100) {
    memcpy(buf, start, len);
    buf[len] = '\0';
    if (!try_strtodouble(buf, &value))
      USAGE("Failed to get float from '%s'", buf);
    return value;
  }
  USAGE("Failed to get float from too-long string (%zu bytes)", len);
}

// -----------------------------------------------------------------------------
// Argument arrays
// -----------------------------------------------------------------------------

// Returns error indicator, 1 for error, 0 for success.
int add_arg(arglist *args, char *newarg) {
  if (!args || !newarg) return 1;
  if (args->next == args->max)
    USAGE("Arg table full at %d items", args->max);
  args->args[args->next++] = newarg;
  return 0;
}

arglist *new_arglist(size_t limit) {
  arglist *args = malloc(sizeof(arglist));
  if (!args) PANIC_OOM();
  // Important: There must be a NULL at the end of the args,
  // so we allocate 1 more than needed and use 'calloc'.
  args->args = calloc(limit+1, sizeof(char *));
  if (!args->args) PANIC_OOM();
  args->max = limit;
  args->next = 0;
  return args;
}

// -----------------------------------------------------------------------------
// Splitting a command line into an argument list
// -----------------------------------------------------------------------------

// Split at whitespace, respecting pairs of double and single quotes.
// Returns error code: 1 for error, 0 for no error.
int split(const char *in, arglist *args) {
  if (!in || !args) PANIC_NULL();
  char *new;
  const char *p, *end, *start = in;

  p = start;
  while (*p != '\0') {
    p = skip_whitespace(p);
    if (*p == '\0') break;
    // Read an argument, which might be quoted
    start = p;
    p = read_arg(p);
    if (!p) USAGE("Unmatched quotes in: %s", start);
    // Successful read.  Store a copy in the 'args' array.
    end = p;
    if (is_quote(p)) {
      start++;
      p++;
    } 
    new = malloc(end - start + 1);
    if (!new) {
      PANIC_OOM();
    }
    memcpy(new, start, end - start);
    new[end - start] = '\0';
    add_arg(args, new);
  }

  return 0;
}

#define STRING_ESCAPE_CHARS "\\\"rnt"
#define STRING_ESCAPE_VALUES "\\\"\r\n\t"

// Return the escaped char value, or NUL on error.
static char escape_char(const char *c) {
  char *pos = strchr(STRING_ESCAPE_VALUES, *c);
  return (pos && *pos) ?
    STRING_ESCAPE_CHARS[pos - STRING_ESCAPE_VALUES] : '\0';
}

static int unescape_char(const char *p) {
  const char *pos = strchr(STRING_ESCAPE_CHARS, *p);
  return (pos && *pos) ? 
    STRING_ESCAPE_VALUES[pos - STRING_ESCAPE_CHARS] : -1;
}

// Very simple unescaping, because it's not clear we need more than
// this.  The escape char is backslash '\' for everything except a
// double quote, in which case the caller supplies the escape
// char.
//
// Caller must free the returned string.
// 
static char *unescape_using(const char *str, char quote_esc) {
  if (!str) PANIC_NULL();
  int chr, i = 0;
  size_t len = strlen(str);
  char *result = malloc(len + 1);
  while (*str) {
    // Special case: CSV files often use "" within a string to escape
    // a double quote
    if (*str == quote_esc) {
      if (*(str+1) == '"') {
	result[i++] = '"';
	str += 2;
      }
    } else {
      // Usual case: either we have the escape char or we don't
      if (*str == '\\') {
	str++;
	if (!*str) {
	  free(result);
	  return NULL;
	}
	if ((chr = unescape_char(str)) < 0)
	  // Not a recognized escape char, so
	  // ignore the backslash
	  result[i++] = *str;
	else
	  result[i++] = chr;
	str++;
      } else {
	// *str is not backslash
	result[i++] = *str++;
      }
    }
  }
  result[i] = '\0';
  return result;
}
	
char *unescape(const char *str) {
  return unescape_using(str, '\\');
}

char *unescape_csv(const char *str) {
  return unescape_using(str, '"');
}

// In CSV files, it is more common to escape a double quote using ""
// than \".  On command lines and in printed output, \" should be used.
static char *escape_using(const char *str, char quote_esc) {
  if (!str) PANIC_NULL();
  int chr, i = 0;
  size_t len = strlen(str);
  char *result = malloc(2 * len + 1);
  while (*str) {
    if (!(chr = escape_char(str))) {
      // No escape needed
      result[i++] = *str;
    } else {
      result[i++] = (*str == '"') ? quote_esc : '\\';
      result[i++] = chr;
    }
    str++;
  }
  result[i] = '\0';
  return result;
}

char *escape_csv(const char *str) {
  return escape_using(str, '"');
}

char *escape(const char *str) {
  return escape_using(str, '\\');
}

// Returns error code: 1 for error, 0 for no error.
int split_unescape(const char *in, arglist *args) {
  int err;
  char *unescaped = unescape(in);
  if (!unescaped) return 1;
  err = split(unescaped, args);
  free(unescaped);
  return err;
}

int ends_in(const char *str, const char *suffix) {
  if (!str || !suffix) return 0;
  const char *end1 = str;
  const char *end2 = suffix;
  while (*end1) end1++;
  while (*end2) end2++;
  while ((end1 > str) && (end2 > suffix) &&
	 (*end1 == *end2)) {
    end1--;
    end2--;
  }
  return (*end1 == *end2);
}

// -----------------------------------------------------------------------------
// Misc
// -----------------------------------------------------------------------------

FILE *maybe_open(const char *filename, const char *mode) {
  if (!filename) return NULL;
  FILE *f = fopen(filename, mode);
  if (!f) {
    fprintf(stderr, "%s: %s: ", progname, filename);
    perror(NULL);
    exit(-1);
  }
  return f;
}

// Caller must free the returned string
char *lefttrim(char *str) {
  if (!str) PANIC_NULL();
  while (*str && (*str == ' ')) str++;
  return strdup(str);
}

// Problem: Microseconds are awkward to display because the numbers
// are large.  Milliseconds is better, until the number exceeds 1,000
// (then seconds is the better unit).  We need to (1) convert μs to ms
// or sec, (2) format the number appropriately, and (3) know what unit
// to print after it.
//
// It's a two-step process, because we choose units based on the
// maximum of a set of values to be displayed together.  We need a
// function to choose the units, and another to apply that choice.
//

Units time_units[] = {
  {"μs", 1,          1000,      "%7.0f %-2s", "%7.0f"}, // 1234567 Un
  {"ms", 1000,       1000*1000, "%7.2f %-2s", "%7.2f"}, // 1234.67 Un
  {"s",  1000*1000, -1,         "%7.2f %-2s", "%7.2f"}, // 1234.67 Un
};

Units space_units[] = {
  {"B",  1,               1024,           "%7.0f %-2s", "%7.0f"},
  {"KB", 1024,            1024*1024,      "%7.2f %-2s", "%7.2f"},
  {"MB", 1024*1024,       1024*1024*1024, "%7.2f %-2s", "%7.2f"},
  {"GB", 1024*1024*1024, -1,              "%7.2f %-2s", "%7.2f"},
};

Units count_units[] = {
  {"ct", 1,               1000,           "%7.0f %-2s", "%7.0f"},
  {"K",  1000,            1000*1000,      "%7.2f %-2s", "%7.2f"},
  {"M",  1000*1000,       1000*1000*1000, "%7.2f %-2s", "%7.2f"},
  {"G",  1000*1000*1000, -1,              "%7.2f %-2s", "%7.2f"},
};

Units *select_units(int64_t maxvalue, Units *options) {
  if (!options) PANIC_NULL();
  int i = 0;
  while (1) {
    if (options[i].threshold == -1) break;
    if (maxvalue < options[i].threshold) break;
    i++;
  }
  return &(options[i]);
}

// Caller must free the returned string
char *apply_units(int64_t value, Units *units, bool include_unit_name) {
  if (!units) PANIC_NULL();
  char *str;
  double display_value = (double) value / (double) units->divisor;
  // Note: The printf family of functions rounds float types to the
  // precision specified in the format string.
  if (include_unit_name)
    ASPRINTF(&str, units->fmt_units, display_value, units->unitname);
  else
    ASPRINTF(&str, units->fmt_nounits, display_value);
  return str;
}

// -----------------------------------------------------------------------------
// UTF-8 (for now, we don't need much, so we'll avoid adding a dependency)
// -----------------------------------------------------------------------------

int64_t min64(int64_t a, int64_t b) {
  return (a < b) ? a : b;
}

int64_t max64(int64_t a, int64_t b) {
  return (a > b) ? a : b;
}

// Returns pointer to first byte after CURRENT utf-8 character, or
// NULL if there are none.
static const char *utf8_next(const char *str) {
  if (!str || !*str) return NULL;
  int cbytes;
  const char *s = str;
  if ((*s & 0x80) == 0x00) {
    cbytes = 0; 
  } else if ((*s & 0xE0) == 0xC0) {
    cbytes = 1;
  } else if ((*s & 0xF0) == 0xE0) {
    cbytes = 2;
  } else if ((*s & 0xF8) == 0xF0) {
    cbytes = 3;
  } else {
  invalid:
    PANIC("Invalid UTF-8 at index %zu of '%s'", (s - str), str);
  }
  s++;

  while (cbytes--) {
    if ((*(s++) & 0xC0) != 0x80) goto invalid;
  }
  return s;
}

// Return a pointer to the start of the ith UTF-8 character
// (codepoint) in 'str'.  The first character is at i = 0.  If there
// are less than i characters, NULL is returned.
//
// NOTE: When called with i=0, current character is NOT examined, and
// may be the end of 'str' or may be invalid UTF-8.
//
static const char *utf8_index(const char *str, int i) {
  if (!str) PANIC_NULL();
  if (i == 0) return str;
  while (--i >= 0) str = utf8_next(str);
  return str;
}

// Limitation: Counts codepoints, not displayed characters, so the
// utf8 length of 'str' is not the same as its displayed width.
// E.g. if 'str' contains Unicode combining characters, or zero width
// characters, it will take up less horizontal space than its utf8
// length suggests.
// 
size_t utf8_length(const char *str) {
  if (!str) PANIC_NULL();
  size_t len = 0;
  while ((str = utf8_next(str))) len++;
  return len;
}

// How many bytes of 'str' are in the first 'count' characters?  If
// there are less than 'count' characters, returns the length of
// 'str', i.e. all the bytes.
//
size_t utf8_width(const char *str, int count) {
  if (!str) PANIC_NULL();
  if (count < 0) PANIC("Negative string length argument %d", count);
  const char *s = utf8_index(str, count);
  return s ? (size_t) (s - str) : strlen(str);
}

// -----------------------------------------------------------------------------
// Printing utilities, for consistent presentation
// -----------------------------------------------------------------------------

// Note: 'len' is the maximum length of the printed string.  Caller
// must free the returned string.
char *command_announcement(const char *cmd, int index, const char *fmt, int len) {
  char *tmp;
  ASPRINTF(&tmp, fmt, index + 1, *cmd ? cmd : "(empty)");
  if ((len != NOLIMIT) && ((int) strlen(tmp) > len))
    tmp[len] = '\0';
  return tmp;
}

void announce_command(const char *cmd, int index) {
  const char *fmt = "Command %d: %s";
  char *announcement = command_announcement(cmd, index, fmt, NOLIMIT); 
  printf("%s", announcement);
  printf("\n");
  fflush(stdout);
  free(announcement);
}

