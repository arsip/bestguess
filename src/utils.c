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
    fprintf(stderr, "%s: ", progname);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
    fflush(stderr);
}

// -----------------------------------------------------------------------------
// Custom usage struct with accessors and comparators
// -----------------------------------------------------------------------------

Usage *new_usage_array(int cap) {
  if (cap < 1) return NULL;
  Usage *usage = malloc(sizeof(Usage));
  if (!usage) PANIC_OOM();
  UsageData *data = malloc(cap * sizeof(UsageData));
  if (!data) PANIC_OOM();
  usage->next = 0;
  usage->capacity = cap;
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
  }
  free(usage->data);
  free(usage);
}

// Returns pointer to string OWNED BY USAGE STRUCT
char *get_string(Usage *usage, int idx, FieldCode fc) {
  if (!usage) PANIC_NULL();
  if ((idx < 0) || (idx >= usage->next))
    PANIC("index %d out of range 0..%d", usage->next - 1);
  if (fc == F_CMD) return usage->data[idx].cmd;
  if (fc == F_SHELL) return usage->data[idx].shell;
  PANIC("Non-string field code (%d)", fc);
}

int64_t get_int64(Usage *usage, int idx, FieldCode fc) {
  if (!usage) PANIC_NULL();
  if ((idx < 0) || (idx >= usage->next))
    PANIC("index %d out of range 0..%d", usage->next - 1);
  if (!FNUMERIC(fc)) PANIC("Invalid int64 field code (%d)", fc);
  return usage->data[idx].metrics[FTONUMERICIDX(fc)];
}

// Struct 'usage' gets a COPY of 'str'
void set_string(Usage *usage, int idx, FieldCode fc, const char *str) {
  if (!usage) PANIC_NULL();
  if ((idx < 0) || (idx >= usage->next))
    PANIC("index %d out of range 0..%d", usage->next - 1);
  char *dup = strndup(str, MAXCMDLEN);
  if (!dup) PANIC_OOM();
  switch (fc) {
    case F_CMD:
      usage->data[idx].cmd = dup;
      break;
    case F_SHELL:
      usage->data[idx].shell = dup;
      break;
    default:
      free(dup);
      PANIC("Invalid string field code (%d)", fc);
  }
}

void set_int64(Usage *usage, int idx, FieldCode fc, int64_t val) {
  if (!usage) PANIC_NULL();
  if ((idx < 0) || (idx >= usage->next))
    PANIC("index %d out of range 0..%d", idx, usage->next - 1);
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

// The arg order for comparators passed to qsort_r differs between
// linux and macOS.
#ifdef __linux__
#define MAKE_COMPARATOR(name, fieldcode)				\
  int compare_##name(const void *idx_ptr1,				\
		     const void *idx_ptr2,				\
		     void *context) {					\
    Usage *usage = context;						\
    const int idx1 = *((const int *)idx_ptr1);				\
    const int idx2 = *((const int *)idx_ptr2);				\
    int64_t val1 = get_int64(usage, idx1, fieldcode);			\
    int64_t val2 = get_int64(usage, idx2, fieldcode);			\
    if (val1 > val2) return 1;						\
    if (val1 < val2) return -1;						\
    return 0;								\
  }
#else
#define MAKE_COMPARATOR(name, fieldcode)				\
  int compare_##name(void *context,					\
		     const void *idx_ptr1,				\
		     const void *idx_ptr2) {				\
    Usage *usage = context;						\
    const int idx1 = *((const int *)idx_ptr1);				\
    const int idx2 = *((const int *)idx_ptr2);				\
    int64_t val1 = get_int64(usage, idx1, fieldcode);			\
    int64_t val2 = get_int64(usage, idx2, fieldcode);			\
    if (val1 > val2) return 1;						\
    if (val1 < val2) return -1;						\
    return 0;								\
  }
#endif

MAKE_COMPARATOR(usertime, F_USER)
MAKE_COMPARATOR(systemtime, F_SYSTEM)
MAKE_COMPARATOR(totaltime, F_TOTAL)
MAKE_COMPARATOR(maxrss, F_MAXRSS)
MAKE_COMPARATOR(vcsw, F_VCSW)
MAKE_COMPARATOR(icsw, F_ICSW)
MAKE_COMPARATOR(tcsw, F_TCSW)
MAKE_COMPARATOR(wall, F_WALL)

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
    USAGE("Failed to get integer from '%s'\n", str);
  return value;
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

// Very simple unescaping, because it's not clear we need more.  The
// escape char is backslash '\'.
char *unescape(const char *str) {
  if (!str) PANIC_NULL();
  int chr, i = 0;
  size_t len = strlen(str);
  char *result = malloc(len + 1);
  while (*str) {
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
  result[i] = '\0';
  return result;
}
	
// We use "" to escape ", because that is more common in CSV files than \".
char *escape(const char *str) {
  if (!str) PANIC_NULL();
  int chr, i = 0;
  size_t len = strlen(str);
  char *result = malloc(2 * len + 1);
  while (*str) {
    if (!(chr = escape_char(str))) {
      // No escape needed
      result[i++] = *str;
    } else {
      result[i++] = (*str == '"') ? '"' : '\\';
      result[i++] = chr;
    }
    str++;
  }
  result[i] = '\0';
  return result;
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
    fprintf(stderr, "Cannot open file: %s\n", filename);
    perror(progname);
    exit(-1);
  }
  return f;
}

// -----------------------------------------------------------------------------
// Misc
// -----------------------------------------------------------------------------

int64_t min64(int64_t a, int64_t b) {
  return (a < b) ? a : b;
}

int64_t max64(int64_t a, int64_t b) {
  return (a > b) ? a : b;
}

