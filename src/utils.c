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

// Used by PANIC and WARN macros
void __attribute__((unused)) 
report_error(const char *prelude,
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

// -----------------------------------------------------------------------------
// Custom usage struct with accessors and comparators
// -----------------------------------------------------------------------------

Usage *new_usage_array(int n) {
  if (n < 1) PANIC("Invalid number of usage structs requested");
  Usage *usage = malloc(n * sizeof(Usage));
  if (!usage) PANIC_OOM();
  return usage;
}

void free_usage(Usage *usage, int n) {
  if (!usage || (n < 1)) return;
  // These fields are strings owned by the usage struct
  for (int i = 0; i < n; i++) {
    free(usage[i].cmd);
    free(usage[i].shell);
  }
  free(usage);
}

// Returns pointer to string OWNED BY USAGE STRUCT
char *get_usage_string(Usage *usage, FieldCode fc) {
  if (!usage) PANIC_NULL();
  if (fc == F_CMD) return usage->cmd;
  if (fc == F_SHELL) return usage->shell;
  PANIC("Non-string field code (%d)", fc);
}

int64_t get_usage_int64(Usage *usage, FieldCode fc) {
  if (!usage) PANIC_NULL();
  if (!FNUMERIC(fc)) PANIC("Invalid int64 field code (%d)", fc);
  return usage->metrics[FTONUMERICIDX(fc)];
}

// Struct 'usage' takes OWNERSHIP of 'str'
void set_usage_string(Usage *usage, FieldCode fc, char *str) {
  if (!usage) PANIC_NULL();
  switch (fc) {
    case F_CMD: usage->cmd = str; break;
    case F_SHELL: usage->shell = str; break;
    default: PANIC("Invalid string field code (%d)", fc);
  }
}

void set_usage_int64(Usage *usage,  FieldCode fc, int64_t val) {
  if (!usage) PANIC_NULL();
  if (!FNUMERIC(fc)) PANIC("Invalid int64 field code (%d)", fc);
  usage->metrics[FTONUMERICIDX(fc)] = val;
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
// linux and macos.
#ifdef __linux__
#define MAKE_COMPARATOR(name, fieldcode)				\
  int compare_name(const void *idx_ptr1,				\
		   const void *idx_ptr2,				\
		   void *context) {					\
    Usage *usagedata = context;						\
    const int idx1 = *((const int *)idx_ptr1);				\
    const int idx2 = *((const int *)idx_ptr2);				\
    int64_t val1 = get_usage_int64(&usagedata[idx1], fieldcode);	\
    int64_t val2 = get_usage_int64(&usagedata[idx2], fieldcode);	\
    if (val1 > val2) return 1;						\
    if (val1 < val2) return -1;						\
    return 0;								\
  }
#else
#define MAKE_COMPARATOR(name, fieldcode)				\
  int compare_##name(void *context,					\
		     const void *idx_ptr1,				\
		     const void *idx_ptr2) {				\
    Usage *usagedata = context;						\
    const int idx1 = *((const int *)idx_ptr1);				\
    const int idx2 = *((const int *)idx_ptr2);				\
    int64_t val1 = get_usage_int64(&usagedata[idx1], fieldcode);	\
    int64_t val2 = get_usage_int64(&usagedata[idx2], fieldcode);	\
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

int64_t strtoint64 (const char *str) {
  int count;
  int64_t value;
  int scancount = sscanf(str, "%" PRId64 "%n", &value, &count);
  if ((scancount != 1) || (count != (int) strlen(str))) 
    PANIC("Failed to get integer from '%s'\n", str);
  return value;
}

// -----------------------------------------------------------------------------
// Argument arrays
// -----------------------------------------------------------------------------

// Returns error indicator, 1 for error, 0 for success.
int add_arg(arglist *args, char *newarg) {
  if (!args || !newarg) return 1;
  if (args->next == args->max) {
    WARN("arg table full at %d items", args->max);
    return 1;
  }
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
  if (!in || !args) {
    WARN("null required arg");
    return 1;
  }
  char *new;
  const char *p, *end, *start = in;

  p = start;
  while (*p != '\0') {
    p = skip_whitespace(p);
    if (*p == '\0') break;
    // Read an argument, which might be quoted
    start = p;
    p = read_arg(p);
    if (!p) {
      WARN("Unmatched quotes in: %s", start);
      return 1;
    }
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
  if (!str) return NULL;
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
	// Not a recognized escape char
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
  if (!str) return NULL;
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
