//  -*- Mode: C; -*-                                                       
// 
//  utils.c
// 
//  COPYRIGHT (c) Jamie A. Jennings, 2024

#include "utils.h"
#include <string.h>
#include <stdarg.h>

static void warning(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "Warning: ");
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
    fflush(stderr);
}

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
  free(args);
}

// Returns error indicator, 1 for error, 0 for success.
int add_arg(arglist *args, char *newarg) {
  if (!args || !newarg) return 1;
  if (args->next == args->max) {
    warning("arg table full at %d items", args->max);
    return 1;
  }
  args->args[args->next++] = newarg;
  return 0;
}

arglist *new_arglist(size_t limit) {
  arglist *args = malloc(sizeof(arglist));
  if (!args) goto oom;
  // Important: There must be a NULL at the end of the args,
  // so we allocate 1 more than needed and use 'calloc'.
  args->args = calloc(limit+1, sizeof(char *));
  if (!args->args) goto oom;
  args->max = limit;
  args->next = 0;
  return args;

 oom:
  warning("Out of memory");
  return NULL;
}

// Split at whitespace, respecting pairs of double and single quotes.
// Returns error code: 1 for error, 0 for no error.
int split(const char *in, arglist *args) {
  if (!in || !args) {
    warning("null required arg");
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
      warning("Unmatched quotes in: %s", start);
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
      warning("Out of memory");
      return 1;
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
// escape char is backslash '\' and we examine the char that follows,
// changing \\, \", \', \n, \r, \t, and otherwise dropping the '\'.
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
    } else {
      result[i++] = *str;
    }
    str++;
  }
  result[i] = '\0';
  return result;
}
	
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
      result[i++] = '\\';
      result[i++] = chr;
    }
    str++;
  }
  result[i] = '\0';
  return result;
}

// Returns error code: 1 for error, 0 for no error.
int split_unescape(const char *in, arglist *args) {
  char *unescaped = unescape(in);
  if (!unescaped) return 1;
  return split(unescaped, args);
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
  
