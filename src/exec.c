//  -*- Mode: C; -*-                                                       
// 
//  exec.c
// 
//  Copyright (C) Jamie A. Jennings, 2024

/*

  USE_WAIT4 ==> call wait4() which returns rusage for the child, else
    call waitpid() which does not.

  PRODUCE_ALT_MEASUREMENTS ==> call getrusage() before forking child
    and after it exits, and print the summary stats.  If USE_WAIT4,
    then we compare user and system time measurements to those
    obtained by wait4(), printing out any discrepancies.

 */
#define USE_WAIT4 1
#define PRODUCE_ALT_MEASUREMENTS 0

#include "exec.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h> 
#include "csv.h"
#include "stats.h"
#include "reports.h"
#include "optable.h"

static int run(const char *cmd,
	       struct rusage *usage,
	       struct rusage *alt_usage) {

  int status;
  pid_t pid;
  FILE *f;
  int use_shell = (config.shell && *config.shell);

  arglist *args = new_arglist(MAXARGS);
  if (!args)
    bail("Exiting");		// Warning already issued

  if (use_shell) {
    if (split_unescape(config.shell, args))
      bail("Exiting");		// Warning already issued
    add_arg(args, strdup(cmd));
  } else {
    if (split_unescape(cmd, args))
      bail("Exiting");		// Warning already issued
  }

  if (DEBUG) {
    printf("Arguments to pass to exec:\n");
    print_arglist(args);
    fflush(NULL);
  }

#if PRODUCE_ALT_MEASUREMENTS
  struct rusage before, after;
  getrusage(RUSAGE_CHILDREN, &before);
#endif

  // Goin' for a ride!
  pid = fork();

  if (pid == 0) {
    if (!config.show_output) {
      f = freopen("/dev/null", "r", stdin);
      if (!f) bail("freopen failed on stdin");
      f = freopen("/dev/null", "w", stderr);
      if (!f) bail("freopen failed on stderr");
      f = freopen("/dev/null", "w", stdout);
      if (!f) bail("freopen failed on stdout");
    }
    execvp(args->args[0], args->args);
    // The exec() functions return only if an error occurs, i.e. it
    // could not launch args[0] (a command or the shell to run it)
    bail("Exec failed");
  }

#if USE_WAIT4
  pid_t err = wait4(pid, &status, 0, usage);
#else
  pid_t err = waitpid(pid, &status, 0);
#endif
  
#if PRODUCE_ALT_MEASUREMENTS
  #define MIL 1000000
  getrusage(RUSAGE_CHILDREN, &after);
  memset(alt_usage, 0, sizeof(struct rusage));
  int64_t utime = (after.ru_utime.tv_usec - before.ru_utime.tv_usec) 
    + (int64_t) (after.ru_utime.tv_sec - before.ru_utime.tv_sec) * MIL;
  int64_t stime =  (after.ru_stime.tv_usec - before.ru_stime.tv_usec)
    + (int64_t) (after.ru_stime.tv_sec - before.ru_stime.tv_sec) * MIL;
  alt_usage->ru_utime.tv_usec = utime - (utime/MIL) * MIL;
  alt_usage->ru_utime.tv_sec = utime / MIL;
  alt_usage->ru_stime.tv_usec = stime - (stime/MIL) * MIL;
  alt_usage->ru_stime.tv_sec = stime / MIL;

  #define CHECKTIME(name, field, fmt) do {				\
      if (alt_usage->field != usage->field)				\
	printf("Discrepancy: alt " name " = " fmt "   "			\
	       "usage " name " = " fmt "\n",				\
	       alt_usage->field, usage->field);				\
    } while (0)

  #if USE_WAIT4
  CHECKTIME("user usec", ru_utime.tv_usec, "%8d");
  CHECKTIME("user sec", ru_utime.tv_usec, "%8d");
  CHECKTIME("system usec", ru_stime.tv_usec, "%8d");
  CHECKTIME("system sec", ru_stime.tv_usec, "%8d");
  #endif
#else
  // Not doing PRODUCE_ALT_MEASUREMENTS
  memset(alt_usage, 0, sizeof(struct rusage));
#endif

  // Check to see if cmd/shell aborted or was killed
  if ((err == -1) || !WIFEXITED(status) || WIFSIGNALED(status)) {
    fprintf(stderr, "Error: Could not execute %s '%s'.\n",
	    use_shell ? "shell" : "command",
	    use_shell ? config.shell : cmd);

    if (!config.shell) {
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

    fflush(stderr);
    exit(-1);  
  }

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

    fflush(stderr);
    exit(-1);
  }

  free_arglist(args);
  return WEXITSTATUS(status);
}

// Returns modal total runtime for 'cmd'
int64_t run_command(int num, char *cmd,
		    FILE *output, FILE *csv_output, FILE *hf_output) {
  int code, fail_count = 0;
  int64_t mode;
  struct rusage *usagedata = malloc(config.runs * sizeof(struct rusage));
  if (!usagedata) bail("Out of memory");

  struct rusage *alt_usagedata = malloc(config.runs * sizeof(struct rusage));
  if (!usagedata) bail("Out of memory");

  if (!config.output_to_stdout) {
    printf("Command %d: %s\n", num+1, *cmd ? cmd : "(empty)");
    fflush(stdout);
  }

  for (int i = 0; i < config.warmups; i++) {
    code = run(cmd, &(usagedata[0]), &(alt_usagedata[0]));
    fail_count += (code != 0);
  }

  for (int i = 0; i < config.runs; i++) {
    code = run(cmd, &(usagedata[i]), &(alt_usagedata[i]));
    fail_count += (code != 0);
    if (output) write_line(output, cmd, code, &(usagedata[i]));
  }

  summary *s = summarize(cmd, fail_count, usagedata);
  if (!s) bail("ERROR: failed to generate summary statistics");

  summary *alt_s = summarize(cmd, fail_count, alt_usagedata);
  if (!alt_s) bail("ERROR: failed to generate summary statistics");

  
  // If raw data is going to an output file, we print a summary on the
  // terminal (else raw data goes to terminal so that it can be piped
  // to another process).
  if (!config.output_to_stdout) {
    print_command_summary(s);

#if PRODUCE_ALT_MEASUREMENTS
    printf("ALT:\n");
    print_command_summary(alt_s);
#endif
    
    if (config.show_graph) print_graph(s, usagedata);
    printf("\n");
  }

  // If exporting CSV file of summary stats, write that line of data
  if (config.csv_filename) write_summary_line(csv_output, s);
  // If exporting in Hyperfine CSV format, write that line of data
  if (config.hf_filename) write_hf_line(hf_output, s);

  fflush(stdout);
  mode = s->total.mode;
  free_summary(s);
  free(usagedata);
  free_summary(alt_s);
  free(alt_usagedata);
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
  printf("ERROR: Number of commands exceeds maximum of %d\n", MAXCMDS);
  bail("Exiting...");
}
