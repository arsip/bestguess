//  -*- Mode: C; -*-                                                       
// 
//  bestguess.c
// 
//  MIT LICENSE 
//  COPYRIGHT (C) Jamie A. Jennings, 2024

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

static int runs = 1;
static int warmups = 0;
static int first_command = 0;
static int show_output = 0;
//static char *input_filename = NULL;
static char *output_filename = NULL;

//hyperfine --export-csv slee.csv --show-output --warmup 10 --runs $reps -N ${commands[@]}


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
  OPT_INPUT,
  OPT_SHOWOUTPUT,
  OPT_SHELL,
  OPT_VERSION,
  OPT_HELP,
};

static void init_options(void) {
  optable_add(OPT_WARMUP,     "w", "warmup",       1, "Number of warmup runs");
  optable_add(OPT_RUNS,       "r", "runs",         1, "Number of timed runs");
  optable_add(OPT_OUTPUT,     "o", "output",       1, "Filename for timing data (CSV)");
  optable_add(OPT_INPUT,      "i", "input",        1, "Filename for input (commands)");
  optable_add(OPT_SHOWOUTPUT, NULL, "show-output", 1, "When set, program output is shown");
  optable_add(OPT_SHELL,      "S", "shell",        1, "Shell to use to run commands (default is none)");
  optable_add(OPT_VERSION,    "v", "version",      0, "Show version");
  optable_add(OPT_HELP,       "h", "help",         0, "Show help");
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

}

static void write_header(FILE *f) {
  fprintf(f, "Command, "
	  "\"User time (us)\", \"System time (us)\", "
	  "\"Max RSS (KiByte)\", "
	  "\"Page Reclaims\", \"Page Faults\", "
	  "\"Voluntary Context Switches\", \"Involuntary Context Switches\" "
	  "\n");
}

static void write_line(FILE *f, const char *cmd, struct rusage *usage) {
  int64_t time;

  // TODO: Escape chars like quotes in 'cmd'
  fprintf(f, "\"%s\", ", cmd);

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

  // These fields are currently unused (unmaintained) on Linux: 
  //   ru_ixrss 
  //   ru_idrss
  //   ru_isrss
  //   ru_nswap
  //   ru_msgsnd
  //   ru_msgrcv
  //   ru_nsignals
  //
  // These fields are always zero on macos, and not useful on Linux:
  //   ru_inblock
  //   ru_oublock

  fprintf(f, "\n");

}

static void run(const char *cmd, struct rusage *usage) {

  int status;
  pid_t pid;
  FILE *f;

  char **args = split(cmd);

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
    
    execvp(args[0], args);

  } else {

    //    waitpid(pid, &status, 0);
    //    getrusage(RUSAGE_CHILDREN, usage);
    wait4(pid, &status, 0, usage);

    // TODO: Handle non-zero status
    if (WEXITSTATUS(status))
      printf("Child exited with non-zero status: %d\n", WEXITSTATUS(status));


    
    for (int i = 0; args[i]; i++) free(args[i]);
    free(args);
  }
}

static int bad_option_value(const char *val, int n, const char *arg) {
  if (val) {
    if (!optable_numvals(n)) {
      printf("Error: option '%s' does not take a value\n",
	     optable_longname(n));
      return 1;
    } else if (!*val) {
      printf("Error: option value is empty '%s'\n", arg);
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

static void process_args(int argc, char **argv) {
  int n, i;
  const char *val;

  i = optable_init(argc, argv);
  if (i < 0) bail("Error initializing option parser");

  while ((i = optable_next(&n, &val, i))) {
    if (n < 0) {
      if (val) {
	// Command argument, not an option or switch
	first_command = i;
	continue;
      }
      printf("Error: invalid option/switch '%s'\n", argv[i]);
      exit(-1);
    }
    if (first_command) {
      printf("Error: options found after first command '%s'\n",
	     argv[first_command]);
      exit(-1);
    }
    switch (n) {
      case OPT_WARMUP:
	sscanf(val, "%d", &warmups);
	printf("%15s  %s ==> %d\n", "warmups", val, warmups);
	if (bad_option_value(val, n, argv[i])) exit(-1);
	break;
      case OPT_RUNS:
	// TODO: Check return value of sscanf
	sscanf(val, "%d", &runs);
	printf("%15s  %s ==> %d\n", "runs", val, runs);
	if (bad_option_value(val, n, argv[i])) exit(-1);
	break;
      case OPT_OUTPUT:
	printf("%15s  %s\n", "output", val);
	if (bad_option_value(val, n, argv[i])) exit(-1);
	output_filename = strdup(val);
	break;
      case OPT_INPUT:
	printf("%15s  %s\n", "input", val);
	if (bad_option_value(val, n, argv[i])) exit(-1);
	break;
      case OPT_SHOWOUTPUT:
	printf("%15s  %s\n", "show-output", val);
	if (bad_option_value(val, n, argv[i])) exit(-1);
	show_output = 1;
	break;
      case OPT_SHELL:
	printf("%15s  %s\n", "shell", val);
	if (bad_option_value(val, n, argv[i])) exit(-1);
	break;
      case OPT_VERSION:
	printf("%15s  %s\n", "version", val);
	if (bad_option_value(val, n, argv[i])) exit(-1);
	break;
      case OPT_HELP:
	printf("%15s  %s\n", "help", val);
	if (bad_option_value(val, n, argv[i])) exit(-1);
	break;
      default:
	printf("Error: Invalid option index %d\n", n);
	exit(-1);
    }
  }
  optable_free();
}

int main(int argc, char *argv[]) {

  struct rusage usage;
  FILE *output = NULL;
  if (argc) progname = argv[0];
  
  if (argc < 2) {
    printf("Usage: %s [options] <cmd> ...\n\n", progname);
    printf("For more information, try %s --help\n", progname);
    exit(-1);
  }

  init_options();
  process_args(argc, argv);
  
  if (output_filename) {
    output = fopen(output_filename, "w");
    if (!output) {
      perror(progname);
      exit(-1);
    }
  } else output = stdout;

  write_header(output);

  for (int cmd = first_command; cmd < argc; cmd++) {

    printf("Warming up...\n");
    for (int i = 0; i < warmups; i++) {
      run(argv[cmd], &usage);
      write_line(output, argv[cmd], &usage);
    }

    printf("Timed runs...\n");
    for (int i = 0; i < runs; i++) {
      run(argv[cmd], &usage);
      write_line(output, argv[cmd], &usage);
    }
  }

  if (output) fclose(output);
  if (output_filename) free(output_filename);
  return 0;
}
