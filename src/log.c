//  -*- Mode: C; -*-                                                       
// 
//  log.c
// 
//  Copyright (C) Jamie A. Jennings, 2024

#include "bestguess.h"
#include "log.h"
#include "utils.h"
#include "csv.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

/*

  Two data sets are always produced when an experiment is run:

  (1) Raw data, i.e. measurements for each timed execution of each
      command

  (2) Experiment log, containing meta-data such as an experiment
      timestamp, some information about the platform, and more.

  Log file contents:

  - Platform description
  - For each command:
    - Raw data filename, batch number, command, display name (if any)
    - Timestamp, system load, 'setup' command, exit status
    - For each warmup:
      - Timestamp, system load, warmup number, exit status
      - Timestamp, system load, 'pre' command, exit status
      - Timestamp, system load, 'post' command, exit status
    - For each timed execution:
      - Timestamp, system load, timed execution number
      - Timestamp, system load, 'pre' command, exit status
      - Timestamp, system load, 'post' command, exit status
    - Timestamp, system load, 'cleanup' command, exit status


 */
