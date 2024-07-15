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

#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/resource.h>

#ifdef __linux__
  #define FMTusec "ld"
#else
  #define FMTusec "d"
#endif

static int reducing_mode = 0;
static int runs = 1;
static int warmups = 0;
static int first_command = 0;
static int show_output = 0;
static int ignore_failure = 0;
static char *input_filename = NULL;
static char *output_filename = NULL;
static const char *shell = NULL;


//hyperfine --export-csv slee.csv --show-output --warmup 10 --runs $reps -N ${commands[@]}

// From sysctl (command-line) on macos 14.1.1:
//
// kern.osproductversion: 14.4.1
// kern.osreleasetype: User
//
// hw.ncpu: 8
// hw.byteorder: 1234
// hw.memsize: 17179869184
// hw.activecpu: 8
// hw.physicalcpu: 8
// hw.physicalcpu_max: 8
// hw.logicalcpu: 8
// hw.logicalcpu_max: 8
// hw.cputype: 16777228
// hw.cpusubtype: 2
// hw.cpu64bit_capable: 1
// hw.cpufamily: 458787763
// hw.cpusubfamily: 2
// hw.cacheconfig: 8 1 4 0 0 0 0 0 0 0
// hw.cachesize: 3499048960 65536 4194304 0 0 0 0 0 0 0
// hw.pagesize: 16384
// hw.pagesize32: 16384
// hw.cachelinesize: 128
// hw.l1icachesize: 131072
// hw.l1dcachesize: 65536
// hw.l2cachesize: 4194304
//


// On Linux with systemd:
//
// To clear PageCache, dentries and inodes, use this command:
// $ sudo sysctl vm.drop_caches=3
// 
// On Linux without systemd, run these commands as root:
//
// # sync; echo 1 > /proc/sys/vm/drop_caches # clear PageCache
// # sync; echo 2 > /proc/sys/vm/drop_caches # clear dentries and inodes
// # sync; echo 3 > /proc/sys/vm/drop_caches # clear all 3
//
// On macos:
//
// $ sync && sudo purge
//

// I/O operations
// --------------

// The macos kernel does not appear to populate the i/o block
// operation counters in r_usage, at least not on a 2020 MacBook Air
// (SSD) running macos 14.4.1.  The current linux kernels do report
// block operation counts, but (1) only for actual disk reads, not for
// files served from cache, and (2) it is not simple to determine the
// block size.  E.g. One of my machines reports a block size of 4092
// for its SSD (via 'blockdev'), but the effective size appears to be
// 8192, probably due to the filesystem or mount options.
//
// Conclusion: We will not record values for "Input ops" (ru_inblock)
// or "Output ops" (ru_oublock).



// Usage: bestguess [options] prog arg1 arg2 .. argN
//
// Options:
//
//   -w M        perform M warmup runs
//   -r M        perform M timed runs
//   -o <file>   write output to <file>
//   -i <file>   read <file> of commands to run
//   -S <shell>  use <shell> to run commands, e.g. 'bash -c'
//   -h          help
//   -v          version
//
// Possible future options:
//
//   --prepare CMD    execute CMD before each timed run
//   --conclude CMD   execute CMD after each timed run

/* 
   Rationale:

   After using hyperfine (https://github.com/sharkdp/hyperfine), I've
   concluded it has the wrong approach.  There are two problems and a
   couple of design limitations.

   Problem 1: Hyperfine assumes a normal distribution when calculating
   all of the stats, and performance measurements are not normally
   distributed.  There is a minimum time that may be approached, but
   which can never be surpassed because there is a certain number of
   cycles needed to execute a deterministic algorithm.  I do not want
   to rely its statistical calculations.

   Problem 2: There is no basis (best practice or scientific evidence)
   to suggest that any of the following default features are
   appropriate and work correctly:

   - Correcting for shell spawning time
   - Running repeatedly for at 3 seconds and at least 10 timed runs
   - Detecting statistical outliers

   Design issue 1: The only way to get all of the individual timed
   values out of Hyperfine is in JSON.  Unix tools are built for text,
   so a CSV file would be preferable.  Hyperfine has a CSV output
   option, but it does not include all of the data that the JSON
   format does.

   Design issue 2: Many other benchmarking systems use one program to
   collect the data and another to analyze it.  This is conducive to
   better science, because the raw data is always saved, apart from
   any analysis.  Saving the timing data allows several kinds of
   analysis to be done, even much later after the timing runs are
   complete.  Other people can analyze the data in different ways.

   Design issue 3: Admittedlty a minor issue, but Python is not a
   desirable prerequisite.  The data analysis scripts that come with
   Hyperfine are Python programs, and maintaining a working Python
   environment is laborious and error-prone.  Users must be able to
   compile Rust, so why not roll the optional analyses done by the
   scripts into the Hyperfine code?

   Finally, much of Hyperfine's functionality might better fit in an
   external program.  Certainly the statistical calculations are in
   that category, but it is also simple to generate a list of commands
   that iterate through parameter ranges.  And the shell, if desired,
   can be included as part of the command.  Separately measuring shell
   startup time seems prudent from a scientific perspective, both to
   record that raw data and to be able to analyze it more carefully
   than simply taking the mean of a few trials.
   
*/
		       
enum Options { 
  OPT_WARMUP,
  OPT_RUNS,
  OPT_OUTPUT,
  OPT_FILE,
  OPT_SHOWOUTPUT,
  OPT_IGNORE,
  OPT_SHELL,
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
  optable_add(OPT_SHELL,      "S",  "shell",          1, "Use this shell to run commands (else no shell)");
  optable_add(OPT_VERSION,    "v",  "version",        0, "Show version");
  optable_add(OPT_HELP,       "h",  "help",           0, "Show help");
  if (optable_error())
    bail("\nError in command-line option parser configuration");
}

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

static void write_header(FILE *f) {
  fprintf(f, "Command, \"Exit code\", "
	  "Shell, "
	  "\"User time (us)\", \"System time (us)\", "
	  "\"Max RSS (KiByte)\", "
	  "\"Page Reclaims\", \"Page Faults\", "
	  "\"Voluntary Context Switches\", \"Involuntary Context Switches\" "
	  "\n");
}

static void write_line(FILE *f, const char *cmd, int code, struct rusage *usage) {
  int64_t time;

  char *escaped_cmd = escape(cmd);
  char *shell_cmd = shell ? escape(shell) : NULL;

  fprintf(f, "\"%s\", ", escaped_cmd);
  fprintf(f, "%d, ", code);
  fprintf(f, "\"%s\", ", shell_cmd ?: "");

  // User time in microseconds
  time = usage->ru_utime.tv_sec * 1000 * 1000 + usage->ru_utime.tv_usec;
  fprintf(f, "%" PRId64 ", ", time);
  // System time in microseconds
  time = usage->ru_stime.tv_sec * 1000 * 1000 + usage->ru_stime.tv_usec;
  fprintf(f, "%" PRId64 ", ", time);
  // Max RSS in kibibytes
  // ru_maxrss (since Linux 2.6.32) This is the maximum resident set size used (in kilobytes).
  // When the above was written, a kilobyte meant 1024 bytes.
  fprintf(f, "%ld, ", usage->ru_maxrss);
  // Page reclaims (count):
  // The number of page faults serviced without any I/O activity; here
  // I/O activity is avoided by “reclaiming” a page frame from the
  // list of pages awaiting reallocation.
  fprintf(f, "%ld, ", usage->ru_minflt);
  // Page faults (count):
  // The number of page faults serviced that required I/O activity.
  fprintf(f, "%ld, ", usage->ru_majflt);

  // Voluntary context switches:
  // The number of times a context switch resulted due to a process
  // voluntarily giving up the processor before its time slice was
  // completed (usually to await availability of a resource).
  fprintf(f, "%ld, ", usage->ru_nvcsw);

  // Involuntary context switches:
  // The number of times a context switch resulted due to a higher
  // priority process becoming runnable or because the current process
  // exceeded its time slice.
  fprintf(f, "%ld ", usage->ru_nivcsw);

  fprintf(f, "\n");
  free(escaped_cmd);
  free(shell_cmd);
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

static int run(const char *cmd, struct rusage *usage) {

  int status;
  pid_t pid;
  FILE *f;

  arglist *args = new_arglist(MAXARGS);
  if (!args) exit(-1);		// Warning already issued

  if (!shell || !*shell) {
    if (split_unescape(cmd, args)) exit(-1); // Warning already issued
  } else {
    if (split_unescape(shell, args)) exit(-1); // Warning already issued
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

static void run_command(char *cmd, FILE *output) {
  int code;
  struct rusage usagedata;

  if (output_filename) fprintf(stderr, "Command: %s\n", cmd);

  if (output_filename) fprintf(stderr, "Warming up...\n");
  for (int i = 0; i < warmups; i++) {
    code = run(cmd, &usagedata);
  }

  if (output_filename) fprintf(stderr, "Timed runs...\n");
  for (int i = 0; i < runs; i++) {
    code = run(cmd, &usagedata);
    write_line(output, cmd, code, &usagedata);
  }
}

static void run_benchmarks(int argc, char **argv) {
  FILE *input = NULL, *output = NULL;
  char buf[MAXCMDLEN];

  input = maybe_open(input_filename, "r");
  output = maybe_open(output_filename, "w");
  if (!output) output = stdout;

  write_header(output);

  for (int k = first_command; k < argc; k++)
    run_command(argv[k], output);

  char *cmd = NULL;
  if (input)
    while ((cmd = fgets(buf, MAXCMDLEN, input)))
      run_command(cmd, output);
  
  if (output) fclose(output);
  if (input) fclose(input);
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
    run_benchmarks(argc, argv);
  }

  return 0;
}
