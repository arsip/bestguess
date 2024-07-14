//  -*- Mode: C; -*-                                                       
// 
//  utils.c
// 
//  COPYRIGHT (c) Jamie A. Jennings, 2024

#include "utils.h"
#include <string.h>

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

// TODO: Keep MAX_ARGS or switch to a table?
#define MAX_ARGS 200

// Split at whitespace, respecting pairs of double and single quotes
char **split(const char *in) {
  char **args = malloc(MAX_ARGS * sizeof(char *));
  if (!args) goto oom;

  int n = 0;
  const char *p, *end, *start = in;

  p = start;
  while (*p != '\0') {
    p = skip_whitespace(p);
    if (*p == '\0') break;
    // Read an argument, which might be quoted
    start = p;
    p = read_arg(p);
    if (!p) {
      warn("args", "Unmatched quotes in: %s", start);
      return NULL;
    }
    // Successful read.  Store it in the 'args' array.
    end = p;
    if (is_quote(p)) {
      start++;
      p++;
    } 
    args[n] = malloc(end - start + 1);
    if (!args[n]) goto oom;
    memcpy(args[n], start, end - start);
    args[n][end - start] = '\0';
    n++;
  }

  args[n] = NULL;
  return args;

 oom:
  warn("args", "Out of memory");
  return NULL;
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
  //printf("*** Unescape: str = '%s' len = %zu ***\n", str, len);
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
  //printf("*** Escape IN: str = '%s' len = %zu ***\n", str, len);
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
  //printf("*** Escape OUT: str = '%s' result = %s ***\n", str, result);
  return result;
}

char **split_unescape(const char *in) {
  char *unescaped = unescape(in);
  //printf("*** Unescaped = '%s' ***\n", unescaped);
  if (!unescaped) return NULL;
  return split(unescaped);
}

__attribute__((unused))
void print_args(char **args) {
  if (args) {
    int i = 0;
    while (args[i]) {
      printf("[%d] %s\n", i, args[i]);
      i++;
    }
  } else {
    printf("null args\n");
  }
}
      
