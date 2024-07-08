//  -*- Mode: C; -*-                                                       
// 
//  bestguess.c
// 
//  MIT LICENSE 
//  COPYRIGHT (C) Jamie A. Jennings, 2024

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
		       
#include "utils.h"
#include "bestguess.h"

static void print_rusage(struct rusage *usage) {

  printf("User   %ld.%06d\n", usage->ru_utime.tv_sec, usage->ru_utime.tv_usec);
  printf("System %ld.%06d\n", usage->ru_stime.tv_sec, usage->ru_stime.tv_usec);
  printf("Total  %ld.%06d\n",
	 usage->ru_stime.tv_sec + usage->ru_utime.tv_sec,
	 usage->ru_stime.tv_usec + usage->ru_utime.tv_usec);

  puts("");

  printf("Max RSS:      %ld (approx. %0.2f MiB)\n",
	 usage->ru_maxrss, (usage->ru_maxrss + 512) / (1024.0 * 1024.0));
  
//   long ru_ixrss;           /* integral shared text memory size */
//   long ru_idrss;           /* integral unshared data size */
//   long ru_isrss;           /* integral unshared stack size */

  puts("");

  printf("Page reclaims: %6ld\n", usage->ru_minflt);
  printf("Page faults:   %6ld\n", usage->ru_majflt);
  printf("Swaps:         %6ld\n", usage->ru_nswap);

  puts("");

//   printf("Input ops:     %6ld\n", usage->ru_inblock);
//   printf("Output ops:    %6ld\n", usage->ru_oublock);

//   printf("Messages (sent):          %6ld\n", usage->ru_msgsnd);
//   printf("         (recvd):         %6ld\n", usage->ru_msgrcv);
//   printf("Signals  (recvd):         %6ld\n", usage->ru_nsignals);

//   puts("");

  printf("Context switches (vol):   %6ld\n", usage->ru_nvcsw);
  printf("                 (invol): %6ld\n", usage->ru_nivcsw);
  printf("                 (TOTAL): %6ld\n", usage->ru_nvcsw + usage->ru_nivcsw);

  puts("");

}

static void run(const char *cmd) {

  struct rusage usage;
  int status;
  pid_t pid;
  FILE *f;

  char **args = split(cmd);

  // Goin' for a ride!
  pid = fork();

  if (pid == 0) {

    f = freopen("/dev/null", "r", stdin);
    if (!f) bail("freopen failed on stdin");
    f = freopen("/dev/null", "w", stderr);
    if (!f) bail("freopen failed on stderr");
    f = freopen("/dev/null", "w", stdout);
    if (!f) bail("freopen failed on stdout");

    execvp(args[0], args);

  } else {

    wait4(pid, &status, 0, &usage);

    print_rusage(&usage);

    if (WEXITSTATUS(status))
      printf("Child exited with non-zero status: %d\n", WEXITSTATUS(status));
    
    for (int i = 0; args[i]; i++) free(args[i]);
  }
}

int main(int argc, char *argv[]) {

  if (argc) progname = argv[0];
  
  if (argc < 2) {
    printf("Usage: %s [options] <cmd> ...\n\n", progname);
    printf("For more information, try %s --help\n", progname);
    exit(-1);
  }
  
  for (int i = 1; i < argc; i++)
    run(argv[i]);

  return 0;
}
