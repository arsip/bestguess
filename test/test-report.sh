#!/bin/bash

SHOWOUTPUT=

prog=../bestreport

printf "%s\n"   '-----------------------------------'
printf "%s\n"   "$0"
printf "%s\n"   'Bestreport tests (terminal output)'
printf "%s\n\n" '-----------------------------------'

declare -a output
allpassed=1

source ./common.sh

# ------------------------------------------------------------------
# Use a CSV file of raw timing data as the input
# ------------------------------------------------------------------

infile=raw1.csv

# Default stats report
ok "$prog" "$infile"
has_stats
contains "Command 1: ls -l" "Total CPU time    5.03 ms" 
contains "Command 2: ps Aux" "Mode" "Min" "Q₁" "Median" "Q₃" "Max"
no_graph
no_boxplot
no_dist_stats
no_tail_stats
no_explanation
has_ranking
contains "Best guess ranking" "Slower by" "797.6%"

# No stats
ok "$prog" -N "$infile"
no_stats
no_graph
no_boxplot
no_dist_stats
no_tail_stats
no_explanation
has_ranking

# Mini stats report
ok "$prog" -M "$infile"
contains "Command 1: ls -l" "Total CPU time    5.03 ms" 
contains "Command 2: ps Aux" 
has_mini_stats
no_graph
no_boxplot
no_dist_stats
no_tail_stats
no_explanation
has_ranking
contains "Slower by" "797.6%"

# No report, but ranking is printed
ok "$prog" -N "$infile"
no_stats
no_graph
no_boxplot
no_dist_stats
no_tail_stats
no_explanation
has_ranking
contains "Slower by" "797.6%"

# Graph with default report
ok "$prog" -G "$infile"
contains "Command 1: ls -l" 
contains "Command 2: ps Aux" 
has_stats
has_graph
no_boxplot
no_dist_stats
no_tail_stats
no_explanation
has_ranking
contains "Slower by" "797.6%"

# Graph with no stats
ok "$prog" -N -G "$infile"
contains "Command 1: ls -l"
contains "Command 2: ps Aux"
missing "Total CPU time    4.63 ms"
no_stats
has_graph
no_boxplot
no_dist_stats
no_tail_stats
no_explanation
has_ranking
contains "Slower by" "797.6%"

# Boxplot with default stats
ok "$prog" -B "$infile"
contains "Command 1: ls -l" "Total CPU time    5.03 ms"
contains "Command 2: ps Aux" 
has_stats
no_graph
has_boxplot
no_dist_stats
no_tail_stats
no_explanation
has_ranking
contains "Slower by" "797.6%"

# Boxplot with no stats
ok "$prog" -N -B "$infile"
missing "Total CPU time    4.63 ms"
no_stats
no_graph
has_boxplot
no_dist_stats
no_tail_stats
no_explanation
has_ranking
contains "Slower by" "797.6%"

# Boxplot with Stats (by default) and Tail and Distribution stats
ok "$prog" -TD -B "$infile"
contains "Command 1: ls -l" "Total CPU time    5.03 ms"
contains "Command 2: ps Aux" 
has_stats
no_graph
has_boxplot
has_dist_stats
has_tail_stats
no_explanation
has_ranking
contains "Slower by" "ps Aux" "797.6%"

# Full report with explanations
ok "$prog" -TD -BE "$infile"
contains "Command 1: ls -l" "Total CPU time    5.03 ms"
contains "Command 2: ps Aux" 
has_stats
no_graph
has_boxplot
has_dist_stats
has_tail_stats
has_explanation
has_ranking
contains "Slower by" "ps Aux" "797.6%"

#
# -----------------------------------------------------------------------------
#

if [[ "$allpassed" == "1" ]]; then 
    printf "All tests passed.\n\n"
    exit 0
else
    exit -1
fi


