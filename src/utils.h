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

// How many millisecs or microsecs in one second
#define MILLISECS 1000
#define MICROSECS 1000000

// 1024 * 1024 = How many things are in a mega-thing
#define MEGA 1048576

int64_t min64(int64_t a, int64_t b);
int64_t max64(int64_t a, int64_t b);

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

typedef struct UsageData {
  char         *cmd;
  char         *shell;
  int64_t       metrics[F_LAST - F_NUMSTART];
} UsageData;
  
typedef struct Usage {
  int           next;
  int           capacity;
  UsageData    *data;
} Usage;


// Accessors for 'Usage' struct
char       *get_string(Usage *usage, int idx, FieldCode fc);
int64_t     get_int64(Usage *usage, int idx, FieldCode fc);
// Setters
void        set_string(Usage *usage, int idx, FieldCode fc, const char *str);
void        set_int64(Usage *usage, int idx, FieldCode fc, int64_t val);


Usage *new_usage_array(int capacity);
void   free_usage_array(Usage *usage);
int    usage_next(Usage *usage);

int64_t rmaxrss(struct rusage *ru);
int64_t rusertime(struct rusage *ru);
int64_t rsystemtime(struct rusage *ru);
int64_t rvcsw(struct rusage *ru);
int64_t ricsw(struct rusage *ru);
int64_t rminflt(struct rusage *ru);
int64_t rmajflt(struct rusage *ru);

typedef int (Comparator)(const void *, const void *, void *);

#define COMPARATOR(name) Comparator name

COMPARATOR(compare_usertime);
COMPARATOR(compare_systemtime);
COMPARATOR(compare_totaltime);
COMPARATOR(compare_maxrss);
COMPARATOR(compare_vcsw);
COMPARATOR(compare_icsw);
COMPARATOR(compare_tcsw);
COMPARATOR(compare_wall);

#if (defined __APPLE__ || defined __MACH__ || defined __DARWIN__ ||	\
     defined __DragonFly__ || (defined __FreeBSD__ && !defined(qsort_r)))
#define HAVE_BSD_QSORT_R
#elif (defined __GLIBC__ || (defined (__FreeBSD__) && defined(qsort_r)))
#define HAVE_LINUX_QSORT_R
#elif (defined _WIN32 || defined _WIN64 || defined __WINDOWS__ ||	\
       defined __MINGW32__ || defined __MINGW64__)
#define HAVE_MS_QSORT_R
#else
#error "Cannot determine the version of qsort_r for this platform"
#endif

#ifdef HAVE_MS_QSORT_R
#error "MS qsort_r or qsort_s not supported"
#endif

// Like the GNU/Linux qsort_r
void sort(void *base, size_t nel, size_t width, 
	  int (*compare)(const void *, const void *, void *),
	  void *context);

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
char  *unescape_csv(const char *str);
char  *escape(const char *str);
char  *escape_csv(const char *str);

int split(const char *in, arglist *args);
int split_unescape(const char *in, arglist *args);
int ends_in(const char *str, const char *suffix);

int64_t     strtoint64(const char *str);
bool    try_strtoint64(const char *str, int64_t *result);

// Misc:

FILE *maybe_open(const char *filename, const char *mode);

char *lefttrim(char *str);

int   utf8_length(const char *str);
int   utf8_width(const char *str, int count);

// TODO: DROP THESE!
// Scale and round to one or two decimal places
#define ROUND1(intval, divisor) \
  (round((double)((intval) * 10) / (divisor)) / 10.0)
#define ROUND2(intval, divisor) \
  (round((double)((intval) * 100) / (divisor)) / 100.0)

typedef struct Units {
  const char    *unitname;  // Optionally display this after the value
  const int64_t  divisor;   // Scale down from native units
  const int64_t  threshold; // This Units applies to values below threshold
  const char    *fmt_units;
  const char    *fmt_nounits;
} Units;

static Units time_units[] = {
  {"μs", 1,          1000,      "%7.0f %-2s", "%7.0f"}, // 1234567 Un
  {"ms", 1000,       1000*1000, "%7.2f %-2s", "%7.2f"}, // 1234.67 Un
  {"s",  1000*1000, -1,         "%7.2f %-2s", "%7.2f"}, // 1234.67 Un
};

static Units space_units[] = {
  {"B",   1,              1024,           "%7.0f %-2s", "%7.0f"},
  {"KB", 1024,            1024*1024,      "%7.2f %-2s", "%7.2f"},
  {"MB", 1024*1024,       1024*1024*1024, "%7.2f %-2s", "%7.2f"},
  {"GB", 1024*1024*1024, -1,              "%7.2f %-2s", "%7.2f"},
};

static Units count_units[] = {
  {"ct", 1,               1000,           "%7.0f %-2s", "%7.0f"},
  {"K",  1000,            1000*1000,      "%7.2f %-2s", "%7.2f"},
  {"M",  1000*1000,       1000*1000*1000, "%7.2f %-2s", "%7.2f"},
  {"G",  1000*1000*1000, -1,              "%7.2f %-2s", "%7.2f"},
};

#define UNITS 1
#define NOUNITS 0
#define NOLIMIT -1

Units *select_units(int64_t maxvalue, Units *options);
char  *apply_units(int64_t value, Units *units, bool show_unit_names);

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
