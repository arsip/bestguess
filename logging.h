/*  -*- Mode: C; -*-                                                         */
/*                                                                           */
/*  logging.h                                                                */
/*                                                                           */
/*  COPYRIGHT (C) Jamie A. Jennings, 2024                                    */

/*
  NOTE: Requires -Wno-variadic-macros when -Wpedantic is used, because
  -Wpedantic enables a warning that variadic macros are not allowed by
  c89.  And c89 is the recommended dialect for code that needs to
  compile also on Windows (with minGW).
*/

#ifndef logging_h
#define logging_h

#include <stdarg.h> 		// __VA_ARGS__ (var args)

/* N.B. confess() is used in only the most awkward situations, when
   there is no easy way to return a specific error to the caller, AND
   when we do not want to ask the user to recompile with LOGGING in
   order to understand that something very strange and unrecoverable
   occurred.
*/

#define confess(who, ...) do {					\
    do_confess("err", who, __FILE__, __LINE__, __VA_ARGS__);	\
  } while (0)

static void __attribute__((unused)) 
do_confess(const char *level,
	   const char *who,
	   const char *filename, int lineno,
	   const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "bestguess [%s] %s:%d (%s) ", level, filename, lineno, who);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
    fflush(stderr);
}

/* 
   When it would be perilous to continue executing, use panic.
*/
#define panic(who, ...) do {						\
    do_confess("panic", who, __FILE__, __LINE__, __VA_ARGS__);		\
    fflush(stderr);							\
    exit(-255);								\
  } while (0)

/*
 * -----------------------------------------------------------------------------
 * Logging
 *
 * 0 = none -- print only confessions (confessions are always heard)
 *
 *     Confession messages are labeled "err" and should be used only
 *     when an internal inconsistency, i.e. a BUG, is detected, and
 *     when there is no suitable way to return a useful error
 *     indicator.
 *
 * 1 = warn -- print warnings and confessions
 *
 *     Warnings are issued for out of memory and for occasional
 *     situations where we cannot determine if there is an internal
 *     inconsistency (BUG) or the API is being used improperly.
 *
 * 2 = inform -- print information, warnings, and confessions
 *
 *     Info messages are issued for ordinary misuse of the API, e.g. a
 *     required argument is NULL or out of range.  Setting the log
 *     level to 2 should be very helpful for debugging.
 *
 * 3 = trace -- print things as they happen, plus the others above
 *
 *     Tracing information can be too much information.  Probably
 *     useful only for deep debugging.
 *
 * -----------------------------------------------------------------------------
 */

#ifndef LOGLEVEL
#define LOGLEVEL 0
#endif

#define WHEN_DEBUGGING if (DEBUG)
#define WHEN_TRACING if (LOGLEVEL > 2)

#define warn(who, ...) do {						\
    if (LOGLEVEL > 0)							\
      do_confess("warn", who, __FILE__, __LINE__, __VA_ARGS__);	\
  } while (0)

#define inform(who, ...) do {						\
    if (LOGLEVEL > 1)							\
      do_confess("info", who, __FILE__, __LINE__, __VA_ARGS__);	\
  } while (0)

#define trace(who, ...) do {						\
    WHEN_TRACING 	 						\
      do_confess("trace", who, __FILE__, __LINE__, __VA_ARGS__);	\
  } while (0)

#endif
