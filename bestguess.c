//  -*- Mode: C; -*-                                                       
// 
//  bestguess.c
// 
//  MIT LICENSE 
//  COPYRIGHT (C) Jamie A. Jennings, 2024

static const char *progversion = "0.1";
static const char *progname = "bestguess";

#include "utils.h"
#include "optable.h"
#include "bestguess.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 
#include <inttypes.h>
#include <math.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/resource.h>

static int reducing_mode = 0;
static int runs = 1;
static int warmups = 0;
static int first_command = 0;
static int show_output = 0;
static int ignore_failure = 0;
static char *input_filename = NULL;
static char *output_filename = NULL;
static char *hf_filename = NULL;
static const char *shell = NULL;

enum Options { 
  OPT_WARMUP,
  OPT_RUNS,
  OPT_OUTPUT,
  OPT_FILE,
  OPT_SHOWOUTPUT,
  OPT_IGNORE,
  OPT_SHELL,
  OPT_HFCSV,			// Hyperfine format CSV
  OPT_VERSION,
  OPT_HELP,
};

static void init_options(void) {
  optable_add(OPT_WARMUP,     "w",  "warmup",         1, "Number of warmup runs");
  optable_add(OPT_RUNS,       "r",  "runs",           1, "Number of timed runs");
  optable_add(OPT_OUTPUT,     "o",  "output",         1, "Write timing data (CSV) to <FILE>");
  optable_add(OPT_FILE,       "f",  "file",           1, "Read commands/data from <FILE>");
  optable_add(OPT_SHOWOUTPUT, NULL, "show-output",    0, "Show program output");
  optable_add(OPT_IGNORE,     "i",  "ignore-failure", 0, "Ignore non-zero exit codes");
  optable_add(OPT_SHELL,      "S",  "shell",          1, "Use <SHELL> (e.g. \"/bin/bash -c\") to run commands");
  optable_add(OPT_HFCSV,      NULL, "hyperfine-csv",  1, "Write Hyperfine-style summary (CSV) to <FILE>");
  optable_add(OPT_VERSION,    "v",  "version",        0, "Show version");
  optable_add(OPT_HELP,       "h",  "help",           0, "Show help");
  if (optable_error())
    bail("\nError in command-line option parser configuration");
}

typedef struct Summary {
  char   *cmd;
  int     runs;
  int64_t user;			// median
  int64_t umin;
  int64_t umax;
  int64_t system;		// median
  int64_t smin;
  int64_t smax;
  int64_t total;		// median
  int64_t tmin;
  int64_t tmax;
  int64_t rss;			// median
  int64_t rssmin;
  int64_t rssmax;
} Summary;

static void free_Summary(Summary *s) {
  free(s->cmd);
  free(s);
}

#ifdef __linux__
  #define FMTusec "ld"
#else
  #define FMTusec "d"
#endif

__attribute__((unused))
static void print_rusage(struct rusage *usage) {

  printf("User   %ld.%" FMTusec "\n", usage->ru_utime.tv_sec, usage->ru_utime.tv_usec);
  printf("System %ld.%" FMTusec "\n", usage->ru_stime.tv_sec, usage->ru_stime.tv_usec);
  printf("Total  %ld.%" FMTusec "\n",
	 usage->ru_stime.tv_sec + usage->ru_utime.tv_sec,
	 usage->ru_stime.tv_usec + usage->ru_utime.tv_usec);
  puts("");
  printf("Max RSS:      %ld (approx. %0.2f MiB)\n",
	 usage->ru_maxrss, (usage->ru_maxrss + 512) / 1024.0);
  puts("");
  printf("Page reclaims: %6ld\n", usage->ru_minflt);
  printf("Page faults:   %6ld\n", usage->ru_majflt);
  printf("Swaps:         %6ld\n", usage->ru_nswap);
  puts("");
  printf("Context switches (vol):   %6ld\n", usage->ru_nvcsw);
  printf("                 (invol): %6ld\n", usage->ru_nivcsw);
  printf("                 (TOTAL): %6ld\n", usage->ru_nvcsw + usage->ru_nivcsw);
  puts("");
  // These fields are currently unused (unmaintained) on Linux: 
  //   ru_ixrss 
  //   ru_idrss
  //   ru_isrss
  //   ru_nswap
  //   ru_msgsnd
  //   ru_msgrcv
  //   ru_nsignals
  //
  // These fields are always zero on macos, and not too useful on Linux:
  //   ru_inblock
  //   ru_oublock
}

// This list is the order in which fields will print in the CSV output
#define XFields(X)					    \
  X(F_CMD,      "Command",                      "\"%s\"")   \
  X(F_EXIT,     "Exit code",                    "%d")	    \
  X(F_SHELL,    "Shell",                        "\"%s\"")   \
  X(F_USER,     "User time (us)",               "%" PRId64) \
  X(F_SYSTEM,   "System time (us)",             "%" PRId64) \
  X(F_RSS,      "Max RSS (Bytes)",              "%ld")	    \
  X(F_RECLAIMS, "Page Reclaims",                "%ld")	    \
  X(F_FAULTS,   "Page Faults",                  "%ld")	    \
  X(F_VCSW,     "Voluntary Context Switches",   "%ld")	    \
  X(F_ICSW,     "Involuntary Context Switches", "%ld")	

// F_TOTAL is a pseudo field name used only for computing summary
// statistics.  It is never written to the raw data output file.
#define F_TOTAL -1

#define FIRST(a, b, c) a,
typedef enum FieldCodes {XFields(FIRST) F_LAST};
#define SECOND(a, b, c) b,
const char *Headers[] = {XFields(SECOND) NULL};
#define THIRD(a, b, c) c,
const char *FieldFormats[] = {XFields(THIRD) NULL};

static void write_header(FILE *f) {
  for (int i = 0; i < (F_LAST - 1); i++)
    fprintf(f, "%s,", Headers[i]);
  fprintf(f, "%s\n", Headers[F_LAST - 1]);
  fflush(f);
}

#define SEP do {				\
    fputc(',', f);				\
  } while (0)
#define NEWLINE do {				\
    fputc('\n', f);				\
  } while (0)
#define WRITEFMT(f, fmt_string, val) do {	\
    fprintf((f), (fmt_string), (val));		\
  } while (0)
#define WRITE(f, fmt_idx, val) do {		\
    WRITEFMT(f, FieldFormats[fmt_idx], val);	\
    SEP;					\
  } while (0)
#define WRITELN(f, fmt_idx, val) do {		\
    WRITE(f, fmt_idx, val);			\
    NEWLINE;					\
  } while (0)

static int64_t rss(struct rusage *usage) {
  return usage->ru_maxrss;
}

static int64_t usertime(struct rusage *usage) {
  return usage->ru_utime.tv_sec * 1000 * 1000 + usage->ru_utime.tv_usec;
}

static int64_t systemtime(struct rusage *usage) {
  return usage->ru_stime.tv_sec * 1000 * 1000 + usage->ru_stime.tv_usec;
}

static int64_t totaltime(struct rusage *usage) {
  return usertime(usage) + systemtime(usage);
}

static int64_t volcsw(struct rusage *usage) {
  return usage->ru_nvcsw;
}

static int64_t involcsw(struct rusage *usage) {
  return usage->ru_nivcsw;
}

static int64_t totalcsw(struct rusage *usage) {
  return volcsw(usage) + involcsw(usage);
}

static void write_line(FILE *f, const char *cmd, int code, struct rusage *usage) {
  char *escaped_cmd = escape(cmd);
  char *shell_cmd = shell ? escape(shell) : NULL;

  WRITE(f, F_CMD,   escaped_cmd);
  WRITE(f, F_EXIT,  code);
  if (shell_cmd) WRITE(f, F_SHELL, shell_cmd);
  else SEP;
  // User time in microseconds
  WRITE(f, F_USER, usertime(usage));
  // System time in microseconds
  WRITE(f, F_SYSTEM, systemtime(usage));
  // Max RSS in kibibytes
  // ru_maxrss (since Linux 2.6.32) This is the maximum resident set size used (in kilobytes).
  // When the above was written, a kilobyte meant 1024 bytes.
  WRITE(f, F_RSS, usage->ru_maxrss);
  // Page reclaims (count):
  // The number of page faults serviced without any I/O activity; here
  // I/O activity is avoided by “reclaiming” a page frame from the
  // list of pages awaiting reallocation.
  WRITE(f, F_RECLAIMS, usage->ru_minflt);
  // Page faults (count):
  // The number of page faults serviced that required I/O activity.
  WRITE(f, F_FAULTS, usage->ru_majflt);
  // Voluntary context switches:
  // The number of times a context switch resulted due to a process
  // voluntarily giving up the processor before its time slice was
  // completed (usually to await availability of a resource).
  WRITE(f, F_VCSW, usage->ru_nvcsw);
  // Involuntary context switches:
  // The number of times a context switch resulted due to a higher
  // priority process becoming runnable or because the current process
  // exceeded its time slice.
  WRITELN(f, F_ICSW, usage->ru_nivcsw);

  fflush(f);
  free(escaped_cmd);
  free(shell_cmd);
}

static void write_hf_header(FILE *f) {
  fprintf(f, "command,mean,stddev,median,user,system,min,max\n");
  fflush(f);
}

static void write_hf_line(FILE *f, Summary *s) {
  const double million = 1000.0 * 1000.0;
  // command
  WRITEFMT(f, "%s", *(s->cmd) ? s->cmd : shell);
  SEP;
  // mean (using median because it's more useful with log-normal distributions)
  WRITEFMT(f, "%f", (double) s->total / million);
  // median (repeated to be compatible with hf format)
  WRITEFMT(f, "%f", (double) s->total / million);
  // stddev omitted until we know what to report for a log-normal distribution
  WRITEFMT(f, "%f", (double) 0.0);
  // Total time in seconds as double
  WRITEFMT(f, "%f", (double) s->total / million);
  SEP;
  // User time in seconds as double 
  WRITEFMT(f, "%f", (double) s->user / million);
  SEP;
  // System time in seconds as double
  WRITEFMT(f, "%f", (double) s->system / million);
  SEP;
  // Min total time in seconds as double 
  WRITEFMT(f, "%f", (double) s->tmin / million);
  SEP;
  // Max total time in seconds as double
  WRITEFMT(f, "%f", (double) s->tmax / million);
  NEWLINE;
  fflush(f);
}

static FILE *maybe_open(const char *filename, const char *mode) {
  if (!filename) return NULL;
  FILE *f = fopen(filename, mode);
  if (!f) {
    fprintf(stderr, "Cannot open file: %s\n", filename);
    perror(progname);
    exit(-1);
  }
  return f;
}

#define MAKE_COMPARATOR(accessor)					\
  static int compare_##accessor(void *context,				\
				const void *idx_ptr1,			\
				const void *idx_ptr2) {			\
    struct rusage *usagedata = context;					\
    const int idx1 = *((const int *)idx_ptr1);				\
    const int idx2 = *((const int *)idx_ptr2);				\
    if (accessor(&usagedata[idx1]) > accessor(&usagedata[idx2]))	\
      return 1;								\
    if (accessor(&usagedata[idx1]) < accessor(&usagedata[idx2]))	\
      return -1;							\
    return 0;								\
  }

MAKE_COMPARATOR(usertime)
MAKE_COMPARATOR(systemtime)
MAKE_COMPARATOR(totaltime)
MAKE_COMPARATOR(rss)

#define MEDIAN_SELECT(accessor, indices, usagedata)	\
  ((runs == (runs/2) * 2) 				\
   ?							\
   ((accessor(&(usagedata)[indices[runs/2 - 1]]) +	\
     accessor(&(usagedata)[indices[runs/2]])) / 2)	\
   :							\
   (accessor(&usagedata[indices[runs/2]])))		\

static int64_t find_median(struct rusage *usagedata, int field) {
  if (runs < 1) return 0;
  int *indices = malloc(runs * sizeof(int));
  if (!indices) bail("out of memory");
  for (int i = 0; i < runs; i++) indices[i] = i;
  switch (field) {
    case F_RSS:
      qsort_r(indices, runs, sizeof(int), usagedata, compare_rss);
      return MEDIAN_SELECT(rss, indices, usagedata);
    case F_USER:
      qsort_r(indices, runs, sizeof(int), usagedata, compare_usertime);
      return MEDIAN_SELECT(usertime, indices, usagedata);
    case F_SYSTEM:
      qsort_r(indices, runs, sizeof(int), usagedata, compare_systemtime);
      return MEDIAN_SELECT(systemtime, indices, usagedata);
    case F_TOTAL:
      qsort_r(indices, runs, sizeof(int), usagedata, compare_totaltime);
      return MEDIAN_SELECT(totaltime, indices, usagedata);
    default:
      warn(progname, "Unsupported field code %d", field);
      return INT64_MAX;
  }
}

static Summary *calc_summary(char *cmd,
			     struct rusage *usagedata) {

  Summary *s = calloc(1, sizeof(Summary));
  if (!s) bail("Out of memory");

  s->cmd = escape(cmd);
  s->runs = runs;

  if (runs < 1) {
    // No data to process, and 's' contains all zeros in the numeric
    // fields except for number of runs
    return s;
  }

  int64_t umin = INT64_MAX, smin = INT64_MAX, tmin = INT64_MAX;
  int64_t umax = 0, smax = 0, tmax = 0;
  int64_t rssmin = INT64_MAX, rssmax = 0;

  // Find mins and maxes
  for (int i = 0; i < runs; i++) {
    if (usertime(&usagedata[i]) < umin) umin = usertime(&usagedata[i]);
    if (systemtime(&usagedata[i]) < smin) smin = systemtime(&usagedata[i]);
    if (totaltime(&usagedata[i]) < tmin) tmin = totaltime(&usagedata[i]);
    if (rss(&usagedata[i]) < rssmin) rssmin = rss(&usagedata[i]);

    if (usertime(&usagedata[i]) > umax) umax = usertime(&usagedata[i]);
    if (systemtime(&usagedata[i]) > smax) smax = systemtime(&usagedata[i]);
    if (totaltime(&usagedata[i]) > tmax) tmax = totaltime(&usagedata[i]);
    if (rss(&usagedata[i]) > rssmax) rssmax = rss(&usagedata[i]);
  }
  
  s->tmin = tmin;
  s->tmax = tmax;
  s->umin = umin;
  s->umax = umax;
  s->smin = smin;
  s->smax = smax;
  s->rssmin = rssmin;
  s->rssmax = rssmax;

  // Find medians
  s->user = find_median(usagedata, F_USER);
  s->system = find_median(usagedata, F_SYSTEM);
  s->total = find_median(usagedata, F_TOTAL);
  s->rss = find_median(usagedata, F_RSS);

  return s;
}

// Note: For a log-normal distribution, the geometric mean is the
// median value and is a more useful measure of the "typical" value
// than is the arithmetic mean.
static void print_summary(int num, Summary *s, struct rusage *usagedata) {

  printf("  Summary:                 Range               Median\n");

  if (runs < 1) {
    printf("    No data (number of timed runs was %d)\n", runs);
    return;
  }

  const char *units;
  double divisor;
  
  // Decide on which time unit to use for printing the summary
  if (s->tmax > (1000 * 1000)) {
    units = "s";
    divisor = 1000.0 * 1000.0;
  } else {
    units = "ms";
    divisor = 1000.0;
  }

  printf("    User time     %6.3f %3s - %6.3f %3s    %6.3f %3s\n",
	 (double)(s->umin / divisor), units,
	 (double)(s->umax / divisor), units,
	 (double)(s->user / divisor), units);

  printf("    System time   %6.3f %3s - %6.3f %3s    %6.3f %3s\n",
	 (double)(s->smin / divisor), units,
	 (double)(s->smax / divisor), units,
	 (double)(s->system / divisor), units);

  printf("    Total time    %6.3f %3s - %6.3f %3s    %6.3f %3s\n",
	 (double)(s->tmin / divisor), units,
	 (double)(s->tmax / divisor), units,
	 (double)(s->total / divisor), units);

  printf("    Max RSS       %6.3f %3s - %6.3f %3s    %6.3f %3s\n",
	 (double) s->rssmin / (1024.0 * 1024.0), "MiB",
	 (double) s->rssmax / (1024.0 * 1024.0), "MiB",
	 (double) s->rss / (1024.0 * 1024.0), "MiB");

  int64_t csw;
  const char *descriptor;
  for (int i = 0; i < runs; i++) {
    csw = totalcsw(&usagedata[i]);
    if (csw > MED_CSW) {
      if (csw > HIGH_CSW) descriptor = "high";
      else descriptor = "moderate";
      printf("Iteration %2d: %s number of context switches (%" PRId64 ")"
	     " for command #%d: %s\n", i, descriptor, csw, num,
	     (s->cmd && *(s->cmd)) ? s->cmd : "(empty)");
    }
  } // for each iteration executed

}

static int run(const char *cmd, struct rusage *usage) {

  int status;
  pid_t pid;
  FILE *f;

  // In the error checks below, we can exit immediately on error,
  // because a warning will have already been issued
  arglist *args = new_arglist(MAXARGS);
  if (!args) exit(-1);		               

  if (!shell || !*shell) {
    if (split_unescape(cmd, args)) exit(-1);
  } else {
    if (split_unescape(shell, args)) exit(-1);
    add_arg(args, strdup(cmd));
  }

  if (DEBUG) {
    printf("Arguments to pass to exec:\n");
    print_arglist(args);
  }

  // Goin' for a ride!
  pid = fork();

  if (pid == 0) {
    if (!show_output) {
      f = freopen("/dev/null", "r", stdin);
      if (!f) bail("freopen failed on stdin");
      f = freopen("/dev/null", "w", stderr);
      if (!f) bail("freopen failed on stderr");
      f = freopen("/dev/null", "w", stdout);
      if (!f) bail("freopen failed on stdout");
    }
    execvp(args->args[0], args->args);
  }

  wait4(pid, &status, 0, usage);

  if (!ignore_failure && WEXITSTATUS(status)) {
    fprintf(stderr, "Command exited with non-zero exit code: %d. ", WEXITSTATUS(status));
    fprintf(stderr, "Use the -i/--ignore-failure option to ignore non-zero exit codes.\n");
    exit(-1);
  }
  free_arglist(args);
  return WEXITSTATUS(status);
}

static void run_command(int num, char *cmd,
			FILE *output,
			FILE *hf_output) {
  int code;
  struct rusage *usagedata = malloc(runs * sizeof(struct rusage));
  if (!usagedata) bail("Out of memory");

  if (output_filename)
    printf("Command %d: %s\n", num, *cmd ? cmd : "(empty)");

  if (output_filename)
    printf("  Warming up with %d runs...\n", warmups);

  for (int i = 0; i < warmups; i++) {
    code = run(cmd, &(usagedata[0]));
  }

  if (output_filename)
    printf("  Timing %d runs...\n", runs);

  for (int i = 0; i < runs; i++) {
    code = run(cmd, &(usagedata[i]));
    write_line(output, cmd, code, &(usagedata[i]));
  }

  Summary *s = calc_summary(cmd, usagedata);
  if (!s) bail("Error generating summary statistics");

  // If raw data is going to an output file, we print a summary on the
  // terminal (else raw data goes to terminal so that it can be piped
  // to another process).
  if (output_filename) print_summary(num, s, usagedata);

  if (hf_filename) write_hf_line(hf_output, s);


  free_Summary(s);
  
}

static void run_all_commands(int argc, char **argv) {
  FILE *input = NULL, *output = NULL, *hf_output = NULL;
  int num = 1;
  char buf[MAXCMDLEN];

  input = maybe_open(input_filename, "r");
  output = maybe_open(output_filename, "w");
  hf_output = maybe_open(hf_filename, "w");
  if (!output) output = stdout;

  write_header(output);
  if (hf_output) write_hf_header(hf_output);

  for (int k = first_command; k < argc; k++)
    run_command(num++, argv[k], output, hf_output);

  char *cmd = NULL;
  if (input)
    while ((cmd = fgets(buf, MAXCMDLEN, input)))
      run_command(num++, cmd, output, hf_output);
  
  if (output) fclose(output);
  if (input) fclose(input);
  if (hf_output) fclose(hf_output);
}

static int bad_option_value(const char *val, int n) {
  if (DEBUG)
    printf("%20s: %s\n", optable_longname(n), val);

  if (val) {
    if (!optable_numvals(n)) {
      printf("Error: option '%s' does not take a value\n",
	     optable_longname(n));
      return 1;
    }
  } else {
    if (optable_numvals(n)) {
      printf("Error: option '%s' requires a value\n",
	     optable_longname(n));
      return 1;
    }
  }
  return 0;
}

static int process_args(int argc, char **argv) {
  int n, i;
  const char *val;

  // 'i' steps through the args from 1 to argc-1
  i = optable_init(argc, argv);
  if (i < 0) bail("Error initializing option parser");

  while ((i = optable_next(&n, &val, i))) {
    if (n < 0) {
      if (val) {
	// Command argument, not an option or switch
	if ((i == 1) && (strcmp(val, PROCESS_DATA_COMMAND) == 0)) {
	  if (DEBUG) printf("*** Reducing mode ***\n");
	  reducing_mode = 1;
	  continue;
	}
	if (!first_command) first_command = i;
	continue;
      }
      fprintf(stderr, "Error: invalid option/switch '%s'\n", argv[i]);
      exit(-1);
    }
    if (first_command) {
      fprintf(stderr, "Error: options found after first command '%s'\n",
	      argv[first_command]);
      exit(-1);
    }
    switch (n) {
      case OPT_WARMUP:
	if (bad_option_value(val, n)) exit(-1);
	if (!sscanf(val, "%d", &warmups)) {
	  fprintf(stderr, "Failed to get number of warmups from '%s'\n", val);
	  exit(-1);
	}
	break;
      case OPT_RUNS:
	if (bad_option_value(val, n)) exit(-1);
	if (!sscanf(val, "%d", &runs)) {
	  fprintf(stderr, "Failed to get number of runs from '%s'\n", val);
	  exit(-1);
	}
	break;
      case OPT_OUTPUT:
	if (bad_option_value(val, n)) exit(-1);
	output_filename = strdup(val);
	break;
      case OPT_FILE:
	if (bad_option_value(val, n)) exit(-1);
	input_filename = strdup(val);
	break;
      case OPT_SHOWOUTPUT:
	if (bad_option_value(val, n)) exit(-1);
	show_output = 1;
	break;
      case OPT_IGNORE:
	if (bad_option_value(val, n)) exit(-1);
	ignore_failure = 1;
	break;
      case OPT_SHELL:
	if (bad_option_value(val, n)) exit(-1);
	shell = val;
	break;
      case OPT_HFCSV:
	if (bad_option_value(val, n)) exit(-1);
	hf_filename = strdup(val);
	break;
      case OPT_VERSION:
	if (bad_option_value(val, n)) exit(-1);
	return n;
	break;
      case OPT_HELP:
	if (bad_option_value(val, n)) exit(-1);
	return n;
	break;
      default:
	fprintf(stderr, "Error: Invalid option index %d\n", n);
	exit(-1);
    }
  }
  optable_free();
  return -1;
}

static int reduce_data(void) {
  FILE *input = NULL, *output = NULL;
  char buf[MAXCSVLEN];

  printf("Not fully implemented yet\n");

  input = maybe_open(input_filename, "r");
  output = maybe_open(output_filename, "w");
  if (!output) output = stdout;
  if (!input) input = stdin;

  char *line = NULL;
  while ((line = fgets(buf, MAXCSVLEN, input)))
    printf("%s", line);

  if (input) fclose(input);
  if (output) fclose(output);
  return 0;
}

int main(int argc, char *argv[]) {

  int immediate, code;
  if (argc) progname = argv[0];
  optable_setusage("[options] <cmd> ...");

  if (argc < 2) {
    optable_printusage(progname);
    fprintf(stderr, "For more information, try %s --help\n", progname);
    exit(-1);
  }

  init_options();
  immediate = process_args(argc, argv);
  
  if (immediate == OPT_VERSION) {
    printf("%s: %s\n", progname, progversion);
    exit(0);
  } else if (immediate == OPT_HELP) {
    optable_printhelp(progname);
    exit(0);
  }

  if (reducing_mode) {
    code = reduce_data();
    exit(code);
  } else {
    run_all_commands(argc, argv);
  }

  return 0;
}
