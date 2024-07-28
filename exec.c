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
  if (!args) bail("Exiting");		               

  if (use_shell) {
    if (split_unescape(shell, args)) bail("Exiting");
    add_arg(args, strdup(cmd));
  } else {
    if (split_unescape(cmd, args)) bail("Exiting");
  }

  if (DEBUG) {
    printf("Arguments to pass to exec:\n");
    print_arglist(args);
    fflush(NULL);
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

// Returns modal total runtime for 'cmd'
int64_t run_command(int num, char *cmd,
		    FILE *output, FILE *csv_output, FILE *hf_output) {
  int code, fail_count = 0;
  int64_t mode;
  struct rusage *usagedata = malloc(runs * sizeof(struct rusage));
  if (!usagedata) bail("Out of memory");

  if (!output_to_stdout) {
    printf("Command %d: %s\n", num+1, *cmd ? cmd : "(empty)");
    fflush(stdout);
  }

  for (int i = 0; i < warmups; i++) {
    code = run(cmd, &(usagedata[0]));
    fail_count += (code != 0);
  }

  for (int i = 0; i < runs; i++) {
    code = run(cmd, &(usagedata[i]));
    fail_count += (code != 0);
    if (output) write_line(output, cmd, code, &(usagedata[i]));
  }

  summary *s = summarize(cmd, fail_count, usagedata);
  if (!s) bail("ERROR: failed to generate summary statistics");

  // If raw data is going to an output file, we print a summary on the
  // terminal (else raw data goes to terminal so that it can be piped
  // to another process).
  if (!output_to_stdout) {
    print_command_summary(s);
    if (show_graph) print_graph(s, usagedata);
    printf("\n");
  }

  // If exporting CSV file of summary stats, write that line of data
  if (csv_filename) write_summary_line(csv_output, s);
  // If exporting in Hyperfine CSV format, write that line of data
  if (hf_filename) write_hf_line(hf_output, s);

  fflush(stdout);
  mode = s->total.mode;
  free_summary(s);
  free(usagedata);
  return mode;
}

void run_all_commands(int argc, char **argv) {
  int n = 0;
  char buf[MAXCMDLEN];
  int64_t modes[MAXCMDS];
  const char *commands[MAXCMDLEN];
  FILE *input = NULL, *output = NULL, *csv_output = NULL, *hf_output = NULL;

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
  csv_output = maybe_open(csv_filename, "w");
  hf_output = maybe_open(hf_filename, "w");

  if (output_to_stdout)
    output = stdout;
  else
    output = maybe_open(output_filename, "w");

  if (output) write_header(output);
  if (csv_output) write_summary_header(csv_output);
  if (hf_output) write_hf_header(hf_output);

  if (first_command > 0) {
    for (int k = first_command; k < argc; k++) {
      commands[n] = argv[k];
      modes[n] = run_command(n, argv[k], output, csv_output, hf_output);
      if (++n == MAXCMDS) goto toomany;
    }
  }

  char *cmd = NULL;
  if (input)
    while ((cmd = fgets(buf, MAXCMDLEN, input))) {
      size_t len = strlen(cmd);
      if ((len > 0) && (cmd[len-1] == '\n')) cmd[len-1] = '\0';
      commands[n] = cmd;
      modes[n] = run_command(n, cmd, output, csv_output, hf_output);
      if (++n == MAXCMDS) goto toomany;
    }
  
  if (!output_to_stdout)
    print_overall_summary(commands, modes, n);

  if (output) fclose(output);
  if (csv_output) fclose(csv_output);
  if (hf_output) fclose(hf_output);
  if (input) fclose(input);
  return;

 toomany:
  printf("ERROR: Number of commands exceeds configured maximum of %d\n",
	 MAXCMDS);
  bail("Exiting...");
}
