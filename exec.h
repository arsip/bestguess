//  -*- Mode: C; -*-                                                       
// 
//  exec.h
// 
//  Copyright (C) Jamie A. Jennings, 2024

#ifndef exec_h
#define exec_h

#include "bestguess.h"
#include <sys/types.h>
#include <sys/wait.h>
#include "common.h"
#include "utils.h"

int64_t run_command(int num, char *cmd, FILE *output, FILE *hf_output);
void run_all_commands(int argc, char **argv);

#endif



