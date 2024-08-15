//  -*- Mode: C; -*-                                                       
// 
//  exec.c
// 
//  Copyright (C) Jamie A. Jennings, 2024

#include "exec.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h> 
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
    PANIC("Exiting");
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
    PANIC("Exiting");
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
      fprintf(stderr, "Executing command under %s produced non-zero exit code %d.\n",
	      args->args[0], WEXITSTATUS(status));
    else
      fprintf(stderr, "Executing command produced non-zero exit code %d.\n",
	      WEXITSTATUS(status));

    if (use_shell && !ends_in(config.shell, " -c"))
      fprintf(stderr, "Note that shells commonly require the '-c' option to run a command.\n");
    else
      fprintf(stderr, "Use the -i/--ignore-failure option to ignore non-zero exit codes.\n");

    exit(ERR_RUNTIME);
  }

  free_arglist(args);
  return WEXITSTATUS(status);
}

// Returns modal total runtime for 'cmd'
static int64_t run_command(int num,
			   char *cmd,
			   FILE *output,
			   FILE *csv_output,
			   FILE *hf_output) {
  int64_t mode;

  if (!config.output_to_stdout) {
    printf("Command %d: %s\n", num+1, *cmd ? cmd : "(empty)");
    fflush(stdout);
  }

  Usage *usage = new_usage_array(config.runs);

  // Even when config.runs is 0, we allocate one usage struct
  int idx = 0;
  for (int i = 0; i < config.warmups; i++) {
    run(cmd, usage, idx);
  }

  for (int i = 0; i < config.runs; i++) {
    idx = usage_next(usage);
    run(cmd, usage, idx);
    if (output) write_line(output, usage, idx);
  }

  int next = 0;
  Summary *s = summarize(usage, &next);

  // If raw data is going to an output file, we print a summary on the
  // terminal (else raw data goes to terminal so that it can be piped
  // to another process).
  if (!config.output_to_stdout) {
    print_summary(s, config.brief_summary);
    if (config.show_graph) print_graph(s, usage);
    printf("\n");
  }

  // If exporting CSV file of summary stats, write that line of data
  if (config.csv_filename) write_summary_line(csv_output, s);
  // If exporting in Hyperfine CSV format, write that line of data
  if (config.hf_filename) write_hf_line(hf_output, s);

  fflush(stdout);
  mode = s->total.mode;
  free_usage_array(usage);
  free_summary(s);
  return mode;
}

void run_all_commands(int argc, char **argv) {
  int n = 0;
  char buf[MAXCMDLEN];
  int64_t modes[MAXCMDS];
  char *commands[MAXCMDLEN];
  FILE *input = NULL, *output = NULL, *csv_output = NULL, *hf_output = NULL;

  if (!config.output_to_stdout && !config.output_filename && !config.brief_summary) {
    printf("Use -%s <FILE> or --%s <FILE> to write raw data to a file.\n",
	   optable_shortname(OPT_OUTPUT), optable_longname(OPT_OUTPUT));
    printf("A single dash '-' instead of a file name prints to stdout.\n\n");
    fflush(stdout);
  }

  if (config.output_filename && (*config.output_filename == '-')) {
    printf("Warning: Output filename '%s' begins with a dash\n\n", config.output_filename);
    fflush(stdout);
  }

  input = maybe_open(config.input_filename, "r");
  csv_output = maybe_open(config.csv_filename, "w");
  hf_output = maybe_open(config.hf_filename, "w");

  if (config.output_to_stdout)
    output = stdout;
  else
    output = maybe_open(config.output_filename, "w");

  if (output) write_header(output);
  if (csv_output) write_summary_header(csv_output);
  if (hf_output) write_hf_header(hf_output);

  // TODO: Now we store the commands in the usage array, so we should
  // not need a separate array of them
  if (config.first_command > 0) {
    for (int k = config.first_command; k < argc; k++) {
      commands[n] = strndup(argv[k], MAXCMDLEN);
      modes[n] = run_command(n, argv[k], output, csv_output, hf_output);
      if (++n == MAXCMDS) goto toomany;
    }
  }

  char *cmd = NULL;
  int group_start = 0;
  if (input)
    while ((cmd = fgets(buf, MAXCMDLEN, input))) {
      size_t len = strlen(cmd);
      if ((len > 0) && (cmd[len-1] == '\n')) cmd[len-1] = '\0';
      if ((!*cmd) && config.groups) {
	// Blank line in input file AND groups option is set
	if (!config.output_to_stdout) {
	  if (group_start == n) continue; // multiple blank lines
	  print_overall_summary(commands, modes, group_start, n);
	  group_start = n;
	  puts("");
	}
      } else {
	// Regular command, which may be blank
	commands[n] = strndup(cmd, MAXCMDLEN);
	modes[n] = run_command(n, cmd, output, csv_output, hf_output);
	if (++n == MAXCMDS) goto toomany;
      }
    } // while

  if (!config.output_to_stdout)
    print_overall_summary(commands, modes, group_start, n);

  if (output) fclose(output);
  if (csv_output) fclose(csv_output);
  if (hf_output) fclose(hf_output);
  if (input) fclose(input);

  for (int i = 0; i < n; i++) free(commands[i]);
  return;

 toomany:
  USAGE("Number of commands exceeds maximum of %d\n", MAXCMDS);
}
