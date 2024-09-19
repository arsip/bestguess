//  -*- Mode: C; -*-                                                       
// 
//  exec.c
// 
//  Copyright (C) Jamie A. Jennings, 2024

#include "exec.h"
#include "cli.h"

#include <string.h>
#include <stdlib.h>
#include <unistd.h> 
#include <assert.h>
#include <sys/time.h>

#include "csv.h"
#include "stats.h"
#include "reports.h"
#include "optable.h"
#include "utils.h"

static void run_prep_command(void) {
  if (!config.prep_command) return;
  
  // TODO: Factor out commonality with run()

  FILE *f;
  pid_t pid;
  int use_shell = *config.shell;
  arglist *args = new_arglist(MAXARGS);

  if (use_shell) {
    split_unescape(config.shell, args);
    add_arg(args, strdup(config.prep_command));
  } else {
    split_unescape(config.prep_command, args);
  }

  if (DEBUG) {
    printf("Prepare command arguments:\n");
    print_arglist(args);
    fflush(NULL);
  }

  // Goin' for a ride!
  pid = fork();

  if (pid == 0) {
    f = freopen("/dev/null", "r", stdin);
    if (!f) PANIC("freopen failed on stdin");
    f = freopen("/dev/null", "w", stderr);
    if (!f) PANIC("freopen failed on stderr");
    f = freopen("/dev/null", "w", stdout);
    if (!f) PANIC("freopen failed on stdout");

    execvp(args->args[0], args->args);
    // The exec() functions return only if an error occurs, i.e. it
    // could not launch args[0] (a command or the shell to run it)
    PANIC("Exec failed");
  }

  int status;
  pid_t err = wait4(pid, &status, 0, NULL);

  // Check to see if cmd/shell aborted or was killed
  if ((err == -1) || !WIFEXITED(status) || WIFSIGNALED(status)) {
    PANIC("Error: Could not execute %s '%s'.\n",
	  use_shell ? "shell" : "command",
	  use_shell ? config.shell : config.prep_command);
  }

  if (!config.ignore_failure && WEXITSTATUS(status)) {
    if (use_shell) 
      PANIC("Prepare command under %s produced non-zero exit code %d\n",
	    args->args[0], WEXITSTATUS(status));
    else
      PANIC("Prepare command produced non-zero exit code %d\n",
	      WEXITSTATUS(status));
  }
}

static int run(const char *cmd, Usage *usage, int idx) {
  FILE *f;
  pid_t pid;
  int status;
  int64_t start, stop;

  int show_output = config.show_output;
  int use_shell = *config.shell;

  struct timeval wall_clock_start, wall_clock_stop;

  run_prep_command();

  arglist *args = new_arglist(MAXARGS);

  if (use_shell) {
    split_unescape(config.shell, args);
    add_arg(args, strdup(cmd));
  } else {
    split_unescape(cmd, args);
  }

  if (DEBUG) {
    printf("Arguments to pass to exec:\n");
    print_arglist(args);
    fflush(NULL);
  }

  if (gettimeofday(&wall_clock_start, NULL)) {
    perror("could not get wall clock time");
    PANIC("Exiting...");
  }

  // Goin' for a ride!
  pid = fork();

  if (pid == 0) {
    if (!show_output) {
      f = freopen("/dev/null", "r", stdin);
      if (!f) PANIC("freopen failed on stdin");
      f = freopen("/dev/null", "w", stderr);
      if (!f) PANIC("freopen failed on stderr");
      f = freopen("/dev/null", "w", stdout);
      if (!f) PANIC("freopen failed on stdout");
    }
    execvp(args->args[0], args->args);
    // The exec() functions return only if an error occurs, i.e. it
    // could not launch args[0] (a command or the shell to run it)
    PANIC("Exec failed");
  }
  
  struct rusage from_os;
  pid_t err = wait4(pid, &status, 0, &from_os);

  if (gettimeofday(&wall_clock_stop, NULL)) {
    perror("could not get wall clock time");
    PANIC("Exiting...");
  }

  start = wall_clock_start.tv_sec * MICROSECS + wall_clock_start.tv_usec;
  stop = wall_clock_stop.tv_sec * MICROSECS + wall_clock_stop.tv_usec;
  set_int64(usage, idx, F_WALL, stop - start);

  set_string(usage, idx, F_CMD, cmd);
  set_string(usage, idx, F_SHELL, config.shell);;

  // Check to see if cmd/shell aborted or was killed
  if ((err == -1) || !WIFEXITED(status) || WIFSIGNALED(status)) {
    fprintf(stderr, "Error: Could not execute %s '%s'.\n",
	    use_shell ? "shell" : "command",
	    use_shell ? config.shell : cmd);

    if (!*config.shell) {
      fprintf(stderr, "\nHint: No shell option specified.  Use -%s or --%s to specify a shell.\n",
	      optable_shortname(OPT_SHELL), optable_longname(OPT_SHELL));
      fprintf(stderr, "      An empty command run in a shell will measure shell startup.\n");
    }

    if (config.input_filename) {
      fprintf(stderr, "\nHint: Commands are being read from file '%s'.  Blank lines\n",
	      config.input_filename);
      fprintf(stderr, "      are read as empty commands unless option --%s is given.\n",
	      optable_longname(OPT_GROUPS));
    }
    exit(ERR_RUNTIME);
  }

  set_int64(usage, idx, F_CODE, WEXITSTATUS(status));
  
  // Fill the rest of the usage metrics from what the OS reported
  set_int64(usage, idx, F_USER, rusertime(&from_os));
  set_int64(usage, idx, F_SYSTEM, rsystemtime(&from_os));
  set_int64(usage, idx, F_TOTAL, rusertime(&from_os) + rsystemtime(&from_os));
  set_int64(usage, idx, F_MAXRSS, rmaxrss(&from_os));
  set_int64(usage, idx, F_RECLAIMS, rminflt(&from_os));
  set_int64(usage, idx, F_FAULTS, rmajflt(&from_os));
  set_int64(usage, idx, F_VCSW, rvcsw(&from_os));
  set_int64(usage, idx, F_ICSW, ricsw(&from_os));
  set_int64(usage, idx, F_TCSW, rvcsw(&from_os) + ricsw(&from_os)); 

  // If we get here, the child process exited normally, though the
  // exit code might not be zero (and zero indicates success)
  if (!config.ignore_failure && WEXITSTATUS(status)) {

    if (use_shell) 
      fprintf(stderr,
	      "\nExecuting command under %s produced"
	      " non-zero exit code %d.\n",
	      args->args[0], WEXITSTATUS(status));
    else
      fprintf(stderr,
	      "\nExecuting command produced non-zero"
	      " exit code %d.\n",
	      WEXITSTATUS(status));

    if (use_shell && !ends_in(config.shell, " -c"))
      fprintf(stderr,
	      "Note that shells commonly require the"
	      " '-c' option to run a command.\n");
    else
      fprintf(stderr,
	      "Use the -i/--ignore-failure option to"
	      " ignore non-zero exit codes.\n");

    exit(ERR_RUNTIME);
  }

  free_arglist(args);
  return WEXITSTATUS(status);
}

// Returns modal total runtime for 'cmd'
static Summary *run_command(int num,
			    char *cmd,
			    FILE *output,
			    FILE *csv_output,
			    FILE *hf_output) {

  if (!config.output_to_stdout) {
    if ((config.report != REPORT_NONE) || config.graph) {
      announce_command(cmd, num, "Command #%d: %s", NOLIMIT);
      printf("\n");
      fflush(stdout);
    }
  }

  Usage *usage = new_usage_array(config.runs);
  assert((config.runs <= 0) || usage);

  Usage *dummy = new_usage_array(1);
  int idx = usage_next(dummy);
  for (int i = 0; i < config.warmups; i++) {
    run(cmd, dummy, idx);
  }

  for (int i = 0; i < config.runs; i++) {
    idx = usage_next(usage);
    run(cmd, usage, idx);
    if (output) write_line(output, usage, idx);
  }

  int ignore = 0;
  Summary *s = summarize(usage, &ignore);
  assert((config.runs <= 0) || s);

  // If raw data is going to an output file, we print a summary on the
  // terminal (else raw data goes to terminal so that it can be piped
  // to another process).
  if (!config.output_to_stdout) {
    if (config.report != REPORT_NONE)
      print_summary(s, (config.report == REPORT_BRIEF));
    if (config.report == REPORT_FULL) 
      print_descriptive_stats(s);
    if (config.graph && usage) {
      print_graph(s, usage, 0, usage->next);
    }
    if ((config.report != REPORT_NONE) || config.graph)
      printf("\n");
  }

  // If exporting CSV file of summary stats, write that line of data
  if (config.csv_filename) write_summary_line(csv_output, s);
  // If exporting in Hyperfine CSV format, write that line of data
  if (config.hf_filename) write_hf_line(hf_output, s);

  free_usage_array(usage);
  fflush(stdout);
  return s;
}

void run_all_commands(int argc, char **argv) {
  // 'n' is the number of summaries == number of commands
  int n = 0;
  Summary *summaries[MAXCMDS];
  char *buf = malloc(MAXCMDLEN);
  if (!buf) PANIC_OOM();
  FILE *input = NULL, *output = NULL, *csv_output = NULL, *hf_output = NULL;

  // Best practice is to save the raw data (all the timing runs).
  // Provide a reminder if that data is not being saved.
  if (!config.output_to_stdout && !config.output_filename) {
    printf("Use -%s <FILE> or --%s <FILE> to write raw data to a file.\n"
	   "A single dash '-' instead of a file name prints to stdout.\n\n",
	   optable_shortname(OPT_OUTPUT), optable_longname(OPT_OUTPUT));
    fflush(stdout);
  }

  // Give a warning in case the user provided a command line flag
  // after the "output file" option, instead of a file name.
  if (config.output_filename && (*config.output_filename == '-')) {
    printf("Warning: Output filename '%s' begins with a dash\n\n",
	   config.output_filename);
    fflush(stdout);
  }

  input = maybe_open(config.input_filename, "r");
  csv_output = maybe_open(config.csv_filename, "w");
  hf_output = maybe_open(config.hf_filename, "w");

  output = (config.output_to_stdout)
    ? stdout
    : maybe_open(config.output_filename, "w");

  if (output) write_header(output);
  if (csv_output) write_summary_header(csv_output);
  if (hf_output) write_hf_header(hf_output);

  if (config.first > 0) {
    for (int k = config.first; k < argc; k++) {
      summaries[n] = run_command(n, argv[k], output, csv_output, hf_output);
      if (++n == MAXCMDS) goto toomany;
    }
  }

  char *cmd = NULL;
  int group_start = 0;
  if (input) {
    while ((cmd = fgets(buf, MAXCMDLEN, input))) {
      // fgets() guarantees a NUL-terminated string
      size_t len = strlen(cmd);
      if ((len > 0) && (cmd[len-1] == '\n')) {
	cmd[len-1] = '\0';
      } else {
	ERROR("Read error on input file (max command length is %d bytes)", MAXCMDLEN);
      }
      if ((!*cmd) && config.groups) {
	// Blank line in input file AND groups option is set
	if (!config.output_to_stdout) {
	  if (group_start == n) continue; // multiple blank lines
	  print_overall_summary(summaries, group_start, n);
	  printf("\n");
	  if (config.boxplot)
	    print_boxplots(summaries, group_start, n);
	  group_start = n;
	  puts("");
	}
      } else {
	// Regular command, which may be blank
	summaries[n] = run_command(n, cmd, output, csv_output, hf_output);
	if (++n == MAXCMDS) goto toomany;
      }
    } // while
  }

  if (!config.output_to_stdout) {
    print_overall_summary(summaries, group_start, n);
    if (config.boxplot)
      print_boxplots(summaries, group_start, n);
  }

  if (output) fclose(output);
  if (csv_output) fclose(csv_output);
  if (hf_output) fclose(hf_output);
  if (input) fclose(input);

  for (int i = 0; i < n; i++) free_summary(summaries[i]);
  free(buf);
  return;

 toomany:
  free(buf);
  USAGE("Number of commands exceeds maximum of %d\n", MAXCMDS);
}
