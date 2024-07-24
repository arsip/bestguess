//  -*- Mode: C; -*-                                                       
// 
//  exec.h
// 
//  Copyright (C) Jamie A. Jennings, 2024

#ifndef exec_h
#define exec_h

#include "bestguess.h"
#include "utils.h"
#include <sys/types.h>
#include <sys/wait.h>

int64_t run_command(int num, char *cmd, FILE *output, FILE *csv_output, FILE *hf_output);
void    run_all_commands(int argc, char **argv);

#endif



