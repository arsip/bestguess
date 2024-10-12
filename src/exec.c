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
  if (!option.prep_command) return;
  
  // TODO: Factor out commonality with run()

  FILE *f;
  pid_t pid;
  int use_shell = *option.shell;
  arglist *args = new_arglist(MAXARGS);

  if (use_shell) {
    split_unescape(option.shell, args);
    add_arg(args, strdup(option.prep_command));
  } else {
    split_unescape(option.prep_command, args);
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
    PANIC("Error trying to execute %s '%s'.\n",
	  use_shell ? "shell" : "command",
	  use_shell ? option.shell : option.prep_command);
  }

  if (!option.ignore_failure && WEXITSTATUS(status)) {
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

  int show_output = option.show_output;
  int use_shell = *option.shell;

  struct timeval wall_clock_start, wall_clock_stop;

  run_prep_command();

  arglist *args = new_arglist(MAXARGS);

  if (use_shell) {
    split_unescape(option.shell, args);
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
  set_string(usage, idx, F_SHELL, option.shell);;

  // Check to see if cmd/shell aborted or was killed
  if ((err == -1) || !WIFEXITED(status) || WIFSIGNALED(status)) {
    fprintf(stderr, "Error: Could not execute %s '%s'.\n",
	    use_shell ? "shell" : "command",
	    use_shell ? option.shell : cmd);

    if (!*option.shell) {
      fprintf(stderr, "\nHint: No shell option specified.  Use -%s or --%s to specify a shell.\n",
	      optable_shortname(OPT_SHELL), optable_longname(OPT_SHELL));
      fprintf(stderr, "      An empty command run in a shell will measure shell startup.\n");
    }

//     if (option.input_filename) {
//       fprintf(stderr, "\nHint: Commands are being read from file '%s'.  Blank lines\n",
// 	      option.input_filename);
//       fprintf(stderr, "      are read as empty commands unless option --%s is given.\n",
// 	      optable_longname(OPT_GROUPS));
//     }

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
  if (!option.ignore_failure && WEXITSTATUS(status)) {

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

    if (use_shell && !ends_in(option.shell, " -c"))
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

static Usage *run_command(Usage *usage, int num, char *cmd, FILE *output) {

  if (!option.output_to_stdout) {
    if ((option.report != REPORT_NONE) || option.graph) {
      announce_command(cmd, num);
      printf("\n");
      fflush(stdout);
    }
  }

  Usage *dummy = new_usage_array(option.warmups);
  int idx;
  for (int i = 0; i < option.warmups; i++) {
    idx = usage_next(dummy);
    run(cmd, dummy, idx);
  }
  free_usage_array(dummy);

  for (int i = 0; i < option.runs; i++) {
    idx = usage_next(usage);
    run(cmd, usage, idx);
    if (output) write_line(output, usage, idx);
  }

  return usage;
}

void run_all_commands(int argc, char **argv) {
  // 'n' is the number of summaries == number of commands
  int n = 0;

  char *buf = malloc(MAXCMDLEN);
  if (!buf) PANIC_OOM();

  FILE *input = NULL, *output = NULL, *csv_output = NULL, *hf_output = NULL;

  // Best practice is to save the raw data (all the timing runs).
  // Provide a reminder if that data is not being saved.
  if (!option.output_to_stdout && !option.output_filename) {
    printf("Use -%s <FILE> or --%s <FILE> to write raw data to a file.\n"
	   "A single dash '-' instead of a file name prints to stdout.\n\n",
	   optable_shortname(OPT_OUTPUT), optable_longname(OPT_OUTPUT));
    fflush(stdout);
  }

  // Give a warning in case the user provided a command line flag
  // after the "output file" option, instead of a file name.
  if (option.output_filename && (*option.output_filename == '-')) {
    printf("Warning: Output filename '%s' begins with a dash\n\n",
	   option.output_filename);
    fflush(stdout);
  }

  input = maybe_open(option.input_filename, "r");
  csv_output = maybe_open(option.csv_filename, "w");
  hf_output = maybe_open(option.hf_filename, "w");

  output = (option.output_to_stdout)
    ? stdout
    : maybe_open(option.output_filename, "w");

  if (output) write_header(output);
  if (csv_output) write_summary_header(csv_output);
  if (hf_output) write_hf_header(hf_output);

  if (option.runs <= 0) {
    printf("  No data\n");
    goto done;
  }

  int start;
  Summary *s;
  Usage *usage = new_usage_array(2 * option.runs);

  if (option.first > 0) {
    for (int k = option.first; k < argc; k++) {
      start = usage->next;
      run_command(usage, n, argv[k], output);
      s = summarize(usage, start, usage->next);
      assert((option.runs <= 0) || s);
      write_summary_stats(s, csv_output, hf_output);
      report_one_command(s);
      graph_one_command(s, usage, start, usage->next);
      free_summary(s);
      if (++n == MAXCMDS) goto toomany;
    }
  }

  char *cmd = NULL;
  if (input) {
    while ((cmd = fgets(buf, MAXCMDLEN, input))) {
      // fgets() guarantees a NUL-terminated string
      size_t len = strlen(cmd);
      if ((len > 0) && (cmd[len-1] == '\n')) {
	cmd[len-1] = '\0';
      } else {
	ERROR("Read error on input file (max command length is %d bytes)", MAXCMDLEN);
      }
      // Command from file (may be blank)
      start = usage->next;
      run_command(usage, n, cmd, output);
      s = summarize(usage, start, usage->next);
      assert((option.runs <= 0) || s);
      write_summary_stats(s, csv_output, hf_output);
      report_one_command(s);
      graph_one_command(s, usage, start, usage->next);
      free_summary(s);
      if (++n == MAXCMDS) goto toomany;
    } // while
  }

  if (!option.output_to_stdout) {
    Ranking *ranking = rank(usage);
    if (ranking) {
      report(ranking);
      free_ranking(ranking);
    }
  }

 done:
  if (output) fclose(output);
  if (csv_output) fclose(csv_output);
  if (hf_output) fclose(hf_output);
  if (input) fclose(input);

  free(buf);
  return;

 toomany:
  free(buf);
  USAGE("Number of commands exceeds maximum of %d\n", MAXCMDS);
}
