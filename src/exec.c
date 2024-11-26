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
#include <spawn.h>
#include <fcntl.h> 

#include "csv.h"
#include "stats.h"
#include "reports.h"
#include "optable.h"
#include "utils.h"

static bool spacetab(char c) {
  return (c == ' ') || (c == '\t');
}

// Note: isblank() is a library function (see ctype.h) that does the
// same thing, but is locale-sensitive.
static bool ishorizws(const char *str) {
  while (spacetab(*str)) str++;
  return (*str == '\0');
}

static bool startswith(const char *str, const char *prefix) {
  while (*prefix) 
    if (*str == *prefix) {
      str++; prefix++;
    } else {
      return false;
    }
  return true;
}

static const char *have_name_option(char *line) {
  int i = -1;
  if (*line != '-') return NULL;
  line++;
  if (optable_shortname(OPT_NAME)
      && startswith(line, optable_shortname(OPT_NAME))) {
    i = strlen(optable_shortname(OPT_NAME));
  } else {
    if (*line != '-') return NULL;
    line++;
    if (optable_longname(OPT_NAME)
	&& startswith(line, optable_longname(OPT_NAME))) {
      i = strlen(optable_longname(OPT_NAME));
    }
  }
  if (i == -1) return NULL;
  switch (line[i]) {
    case '=':
    case ' ':
    case '\t': {
      while (line[i] && ((line[i]==' ') || (line[i]=='\t'))) i++;
      const char *start = &(line[i]);
      const char *end = read_arg(start);
      return strndup(start, (end - start));
    }
    default:
      return NULL;
  }
}

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

static int run(int num, Usage *usage, int idx, int64_t batch) {
  pid_t pid, err;
  int status;
  int64_t start, stop;
  struct rusage from_os;

  const char *cmd = option.commands[num];
  const char *name = option.names[num];

  int show_output = option.show_output;
  int use_shell = *option.shell;

  struct timeval wall_clock_start, wall_clock_stop;

  run_prep_command();

  arglist *args = new_arglist(MAXARGS);

  status = 0;
  if (use_shell) {
    status |= split_unescape(option.shell, args);
    status |= add_arg(args, strdup(cmd));
  } else {
    status |= split_unescape(cmd, args);
  }
  if (status)
    PANIC("Error preparing command and arguments for exec: '%s'", cmd);

  if (DEBUG) {
    printf("Arguments to pass to exec:\n");
    print_arglist(args);
    fflush(NULL);
  }

  if (!args->args[0]) {
    fprintf(stderr, "Error: Cannot execute null command\n");
    fprintf(stderr, "\n"
	    "Hint: An empty command that is run in a shell will measure the shell startup\n"
	    "      time.  Use  -%s or --%s to specify a shell.\n",
	      optable_shortname(OPT_SHELL), optable_longname(OPT_SHELL));

    exit(ERR_USAGE);
  }

  if (gettimeofday(&wall_clock_start, NULL)) {
    perror("could not get wall clock time");
    PANIC("Exiting...");
  }

  posix_spawn_file_actions_t file_actions = NULL;
  posix_spawnattr_t attr = NULL;
  // TODO: char *const env[] = ...;
  if (!show_output) {
    posix_spawn_file_actions_init(&file_actions);
    posix_spawn_file_actions_addopen(&file_actions, 0, "/dev/null", O_RDONLY, 0);
    posix_spawn_file_actions_addopen(&file_actions, 1, "/dev/null", O_WRONLY, 0);
    posix_spawn_file_actions_addopen(&file_actions, 2, "/dev/null", O_WRONLY, 0);
  }

  status = posix_spawnp(&pid, args->args[0], &file_actions, &attr, args->args, NULL);
  err = wait4(pid, &status, 0, &from_os);
    
  if (!show_output)
    posix_spawn_file_actions_destroy(&file_actions);

  if (gettimeofday(&wall_clock_stop, NULL)) {
    perror("could not get wall clock time");
    PANIC("Exiting...");
  }

  start = wall_clock_start.tv_sec * MICROSECS + wall_clock_start.tv_usec;
  stop = wall_clock_stop.tv_sec * MICROSECS + wall_clock_stop.tv_usec;
  set_int64(usage, idx, F_WALL, stop - start);

  set_string(usage, idx, F_CMD, cmd);
  set_string(usage, idx, F_SHELL, option.shell);
  set_string(usage, idx, F_NAME, name);
  usage->data[idx].batch = batch;

  // Check to see if cmd/shell aborted or was killed
  if ((err == -1) || !WIFEXITED(status) || WIFSIGNALED(status)) {
    fprintf(stderr, "Error: Failed to execute %s '%s'.\n",
	    use_shell ? "shell" : "command",
	    use_shell ? option.shell : cmd);

    if (!*option.shell) {
      fprintf(stderr, "\n"
	      "Hint: If this command needs to run in a shell, use -%s or --%s.\n"
	      "      An empty command run in a shell will measure shell startup time.\n",
	      optable_shortname(OPT_SHELL), optable_longname(OPT_SHELL));
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

static Usage *run_command(Usage *usage, int num, FILE *output) {

  const char *cmd = option.commands[num];
  const char *name = option.names[num];
  int64_t batch = next_batch_number();

  if (any_per_command_output())
    announce_command(name, cmd, num);

  Usage *dummy = new_usage_array(option.warmups);
  int idx;
  for (int i = 0; i < option.warmups; i++) {
    idx = usage_next(dummy);
    run(num, dummy, idx, batch);
  }
  free_usage_array(dummy);

  for (int i = 0; i < option.runs; i++) {
    idx = usage_next(usage);
    run(num, usage, idx, batch);
    if (output) write_line(output, usage, idx);
  }

  return usage;
}

Ranking *run_all_commands(void) {

  if (option.runs <= 0) 
    USAGE("Number of runs is 0, nothing to do");

  char *cmd;
  char *buf = malloc(MAXCMDLEN);
  if (!buf) PANIC_OOM();

  int lineno = 0;
  int last_named_command = option.n_commands; // *maybe* named
  FILE *input = NULL, *output = NULL, *csv_output = NULL, *hf_output = NULL;

  input = maybe_open(option.input_filename, "r");
  if (input) {
    while ((cmd = fgets(buf, MAXCMDLEN, input))) {
      // fgets() guarantees a NUL-terminated string
      lineno++;
      size_t len = strlen(cmd);
      if ((len > 0) && (cmd[len-1] == '\n')) {
	cmd[len-1] = '\0';
      } else {
	ERROR("Input file line %d too long (max length is %d bytes)",
	      lineno, MAXCMDLEN);
      }
      if (ishorizws(cmd)) continue;
      // Have non-blank line from file
      const char *name = have_name_option(cmd);
      if (name) {
	if (last_named_command == option.n_commands) {
	  USAGE("Name '%s' must follow a command", name);
	} else {
	  // Commands are added in order, with no blanks, so this name
	  // is for command number n - 1
	  option.names[option.n_commands - 1] = name;
	  last_named_command = option.n_commands;
	}
      } else {
	option.commands[option.n_commands++] = strdup(cmd);
	if (option.n_commands == MAXCMDS) goto toomany;
      }
    } // while reading from file
  } // if there is an input file of possibly-named commands

  if (option.n_commands == 0) 
    USAGE("No commands provided on command line or input file");

  // Best practice is to save the raw data (all the timing runs).
  // We provide a reminder if that data is not being saved.
  if (!option.output_filename && !option.quiet) {
    printf("⇨ Use -%s <FILE> or --%s <FILE> to write raw data to a file. ⇦\n\n",
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

  csv_output = maybe_open(option.csv_filename, "w");
  hf_output = maybe_open(option.hf_filename, "w");
  output = maybe_open(option.output_filename, "w");

  if (csv_output) write_summary_header(csv_output);
  if (hf_output) write_hf_header(hf_output);
  if (output) write_header(output);

  int start;
  Summary *s;
  Usage *usage = NULL;

  // Usage array will expand as needed, but this size should be right
  usage = new_usage_array(option.n_commands * option.runs);

  // FUTURE: We compute summaries twice.  Once in the loop below, as
  // each command's executions finish, and then again later during
  // ranking.  Do this once.

  for (int k = 0; k < option.n_commands; k++) {
    start = usage->next;
    run_command(usage, k, output);
    s = summarize(usage, start, usage->next);
    assert((option.runs <= 0) || s);
    write_summary_line(csv_output, s);
    write_hf_line(hf_output, s);
    per_command_output(s, usage, start, usage->next);
    free_summary(s);
  }

  if (output) fclose(output);
  if (csv_output) fclose(csv_output);
  if (hf_output) fclose(hf_output);
  if (input) fclose(input);
  free(buf);

  // The 'ranking' structure takes ownership of the usage array
  return rank(usage);

 toomany:
  free(buf);
  USAGE("Number of commands exceeds maximum of %d\n", MAXCMDS);
}
