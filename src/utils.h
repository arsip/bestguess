//  -*- Mode: C; -*-                                                       
// 
//  utils.h
// 
//  COPYRIGHT (c) Jamie A. Jennings, 2024

#ifndef utils_h
#define utils_h

#include "bestguess.h"
#include <stdio.h>
#include <sys/resource.h>

// How many microseconds in one second
#define MICROSECS 1000000

// 1024 * 1024 = How many things are in a mega-thing
#define MEGA 1048576

// -----------------------------------------------------------------------------
// Raw data output file (CSV) follows Usage struct contents
// -----------------------------------------------------------------------------
     
#define XFields(X)				 \
  /* -------- String fields ----------------- */ \
  X(F_CMD,      "Command"                      ) \
  X(F_SHELL,    "Shell"                        ) \
  /* -------- Numeric metrics from rusage --- */ \
  X(F_CODE,     "Exit code"                    ) \
  X(F_USER,     "User time (us)"               ) \
  X(F_SYSTEM,   "System time (us)"             ) \
  X(F_MAXRSS,   "Max RSS (Bytes)"              ) \
  X(F_RECLAIMS, "Page Reclaims"                ) \
  X(F_FAULTS,   "Page Faults"                  ) \
  X(F_VCSW,     "Voluntary Context Switches"   ) \
  X(F_ICSW,     "Involuntary Context Switches" ) \
  X(F_WALL,     "Wall clock (us)"              ) \
  /* -------- Computed metrics -------------- */ \
  X(F_TOTAL,    "Total time (us)"              ) \
  X(F_TCSW,     "Total Context Switches"       ) \
  X(F_LAST,     "SENTINEL"                     )

#define FIRST(a, b) a,
typedef enum { XFields(FIRST) } FieldCode;
#undef FIRST
extern const char *Header[];

// IMPORTANT: Check/alter these if the table structure changes
#define F_NUMSTART F_CODE
#define F_NUMEND F_TOTAL
#define F_RAWNUMSTART F_CODE
#define F_RAWNUMEND F_TOTAL

#define FSTRING(f) (((f) >= 0) || ((f) < F_NUMSTART))
#define FRAWDATA(f) (((f) >= F_NUMSTART) && ((f) < F_NUMEND))
#define FNUMERIC(f) (((f) >= F_NUMSTART) && ((f) < F_LAST))
#define FTONUMERICIDX(f) ((f) - F_NUMSTART)

#define INT64FMT "%" PRId64

// -----------------------------------------------------------------------------
// Custom usage struct with accessors and comparators
// -----------------------------------------------------------------------------

typedef struct Usage {
  char         *cmd;		 // command that was executed
  char         *shell;		 // shell used (optional, may be NULL)
  int64_t       metrics[F_LAST - F_NUMSTART];
} Usage;


// Accessors for 'Usage' struct
char    *get_usage_string(Usage *usage, FieldCode fc);
int64_t  get_usage_int64(Usage *usage,  FieldCode fc);
// Setters
void     set_usage_string(Usage *usage, FieldCode fc, char *str);
void     set_usage_int64(Usage *usage,  FieldCode fc, int64_t val);


Usage *new_usage_array(int n);
void   free_usage(Usage *usage, int n);

int64_t rmaxrss(struct rusage *ru);
int64_t rusertime(struct rusage *ru);
int64_t rsystemtime(struct rusage *ru);
int64_t rvcsw(struct rusage *ru);
int64_t ricsw(struct rusage *ru);
int64_t rminflt(struct rusage *ru);
int64_t rmajflt(struct rusage *ru);

// The arg order for comparators passed to qsort_r differs between
// linux and macos.
#ifdef __linux__
typedef int (comparator)(const void *, const void *, void *);
#else
typedef int (comparator)(void *, const void *, const void *);
#endif

#define COMPARATOR(accessor) comparator compare_##accessor

COMPARATOR(usertime);
COMPARATOR(systemtime);
COMPARATOR(totaltime);
COMPARATOR(maxrss);
COMPARATOR(vcsw);
COMPARATOR(icsw);
COMPARATOR(tcsw);
COMPARATOR(wall);

// -----------------------------------------------------------------------------
// Argument lists for calling exec
// -----------------------------------------------------------------------------

typedef struct arglist {
  size_t max;
  size_t next;
  char **args;
} arglist;

arglist *new_arglist(size_t limit);
int      add_arg(arglist *args, char *newarg);
void     free_arglist(arglist *args);
void     print_arglist(arglist *args);

// -----------------------------------------------------------------------------
// String operations
// -----------------------------------------------------------------------------

char  *unescape(const char *str);
char  *escape(const char *str);

int split(const char *in, arglist *args);
int split_unescape(const char *in, arglist *args);
int ends_in(const char *str, const char *suffix);

int64_t strtoint64 (const char *str);

// Misc:

FILE *maybe_open(const char *filename, const char *mode);

/* ----------------------------------------------------------------------------- */
/* Error handling for runtime errors                                             */
/* ----------------------------------------------------------------------------- */

#define ERR_USAGE   1
#define ERR_RUNTIME 2

#define ERROR(...) do {							\
    error_report(__VA_ARGS__);						\
    exit(ERR_RUNTIME);							\
  } while (0)

#define USAGE(...) do {							\
    error_report(__VA_ARGS__);						\
    exit(ERR_USAGE);							\
  } while (0)

void error_report(const char *fmt, ...);

/* ----------------------------------------------------------------------------- */
/* Error handling for internal errors (bugs) and warnings                        */
/* ----------------------------------------------------------------------------- */

#define ERR_PANIC 255

#define PANIC(...) do {							\
    panic_report("Panic at", __FILE__, __LINE__,  __VA_ARGS__);	\
    exit(ERR_PANIC);							\
  } while (0)

#define PANIC_OOM() do {			        \
    PANIC("Out of memory");				\
  } while (0)

#define PANIC_NULL() do {				\
    PANIC("Required argument is NULL");			\
  } while (0)

void panic_report(const char *prelude,
		  const char *filename, int lineno,
		  const char *fmt, ...);


#endif
