//  -*- Mode: C; -*-                                                       
// 
//  exec.c
// 
//  Copyright (C) Jamie A. Jennings, 2024

#include "exec.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h> 
#include "csv.h"
#include "stats.h"
#include "reports.h"
#include "optable.h"

static int run(const char *cmd, struct rusage *usage) {

  int status;
  pid_t pid;
  FILE *f;
  int use_shell = (shell && *shell);

  // In the error checks below, we can exit immediately on error,
  // because a warning will have already been issued
  arglist *args = new_arglist(MAXARGS);
  if (!args) exit(-1);		               

  if (use_shell) {
    if (split_unescape(shell, args)) exit(-1);
    add_arg(args, strdup(cmd));
  } else {
    if (split_unescape(cmd, args)) exit(-1);
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
    // The exec() functions return only if an error occurs, i.e. it
    // could not launch args[0] (a command or the shell to run it)
    abort();
  }

  pid_t err = wait4(pid, &status, 0, usage);

  // Check to see if cmd/shell aborted or was killed
  if ((err == -1) || !WIFEXITED(status) || WIFSIGNALED(status)) {
    fprintf(stderr, "Error: could not execute %s '%s'.\n",
	    use_shell ? "shell" : "command",
	    use_shell ? shell : cmd);
    fflush(stderr);
    exit(-1);  
  }

  // If we get here, we had a normal process exit, though the exit
  // code might not be zero (and zero indicates success)
  if (!ignore_failure && WEXITSTATUS(status)) {

    if (use_shell) 
      fprintf(stderr, "Executing command under %s produced non-zero exit code %d.\n",
	      args->args[0], WEXITSTATUS(status));
    else
      fprintf(stderr, "Executing command produced non-zero exit code %d.\n",
	      WEXITSTATUS(status));

    if (use_shell && !ends_in(shell, " -c"))
      fprintf(stderr, "Note that shells commonly require the '-c' option to run a command.\n");
    else
      fprintf(stderr, "Use the -i/--ignore-failure option to ignore non-zero exit codes.\n");

    fflush(stderr);
    exit(-1);
  }

  free_arglist(args);
  return WEXITSTATUS(status);
}

// Returns median total runtime for 'cmd'
int64_t run_command(int num, char *cmd, FILE *output, FILE *hf_output) {
  int code;
  int64_t median;
  struct rusage *usagedata = malloc(runs * sizeof(struct rusage));
  if (!usagedata) bail("Out of memory");

  if (!output_to_stdout) {
    printf("Command %d: %s\n", num+1, *cmd ? cmd : "(empty)");
    fflush(stdout);
  }

  for (int i = 0; i < warmups; i++) {
    code = run(cmd, &(usagedata[0]));
  }

  if (!output_to_stdout) {
    fflush(stdout);
  }

  for (int i = 0; i < runs; i++) {
    code = run(cmd, &(usagedata[i]));
    if (output) write_line(output, cmd, code, &(usagedata[i]));
  }

  summary *s = summarize(cmd, usagedata);
  if (!s) bail("ERROR: failed to generate summary statistics");

  // If raw data is going to an output file, we print a summary on the
  // terminal (else raw data goes to terminal so that it can be piped
  // to another process).
  if (!output_to_stdout) {
    print_command_summary(s);
    if (show_graph) print_graph(s, usagedata);
    printf("\n");
  }

  // If exporting in Hyperfine CSV format, write that line of data
  if (hf_filename) write_hf_line(hf_output, s);

  fflush(stdout);
  median = s->total;
  free_summary(s);
  free(usagedata);
  return median;
}

void run_all_commands(int argc, char **argv) {
  int n = 0;
  char buf[MAXCMDLEN];
  int64_t mediantimes[MAXCMDS];
  const char *commands[MAXCMDLEN];
  FILE *input = NULL, *output = NULL, *hf_output = NULL;

  if (!output_to_stdout && !output_filename && !brief_summary) {
    printf("Use -%s <FILE> or --%s <FILE> to write raw data to a file.\n",
	   optable_shortname(OPT_OUTPUT), optable_longname(OPT_OUTPUT));
    printf("A single dash '-' instead of a file name prints to stdout.\n\n");
    fflush(stdout);
  }

  if (output_filename && (*output_filename == '-')) {
    printf("Warning: Output filename '%s' begins with a dash\n\n", output_filename);
    fflush(stdout);
  }

  input = maybe_open(input_filename, "r");
  hf_output = maybe_open(hf_filename, "w");

  if (output_to_stdout)
    output = stdout;
  else
    output = maybe_open(output_filename, "w");

  if (output) write_header(output);
  if (hf_output) write_hf_header(hf_output);

  for (int k = first_command; k < argc; k++) {
    commands[n] = argv[k];
    mediantimes[n] = run_command(n, argv[k], output, hf_output);
    if (++n == MAXCMDS) goto toomany;
  }

  char *cmd = NULL;
  if (input)
    while ((cmd = fgets(buf, MAXCMDLEN, input))) {
      commands[n] = cmd;
      mediantimes[n] = run_command(n, cmd, output, hf_output);
      if (++n == MAXCMDS) goto toomany;
    }
  
  if (!output_to_stdout)
    print_overall_summary(commands, mediantimes, n);

  if (output) fclose(output);
  if (input) fclose(input);
  if (hf_output) fclose(hf_output);
  return;

 toomany:
  printf("ERROR: Number of commands exceeds configured maximum of %d\n",
	 MAXCMDS);
  bail("Exiting...");
}
