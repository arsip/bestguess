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
has_summary
contains "Command 1: ls -l" "Total CPU time    5.03 ms" 
contains "Command 2: ps Aux" "Mode" "Min" "Q₁" "Median" "Q₃" "Max"
no_graph
no_boxplot
no_dist_stats
no_tail_stats
no_explanation
has_ranking
contains "Best guess ranking" "Slower by" "8.92x"

# No outout
ok "$prog" -Q "$infile"
no_stats
no_graph
no_boxplot
no_dist_stats
no_tail_stats
no_explanation
no_ranking

# Only ranking
ok "$prog" -QR "$infile"
no_stats
no_graph
no_boxplot
no_dist_stats
no_tail_stats
no_explanation
has_ranking

# Mini stats report with ranking
ok "$prog" -M "$infile"
contains "Command 1: ls -l" "Total CPU time    5.03 ms" 
contains "Command 2: ps Aux" 
no_summary
has_mini_stats
no_graph
no_boxplot
no_dist_stats
no_tail_stats
no_explanation
has_ranking
contains "Slower by" "8.92x"

# Mini report and ranking, a different way
ok "$prog" -QMR "$infile"
no_summary
has_mini_stats
no_graph
no_boxplot
no_dist_stats
no_tail_stats
no_explanation
has_ranking
contains "Slower by" "8.92x"

# Graph with default report
ok "$prog" -G "$infile"
contains "Command 1: ls -l" 
contains "Command 2: ps Aux" 
has_summary
has_graph
no_boxplot
no_dist_stats
no_tail_stats
no_explanation
has_ranking
contains "Slower by" "8.92x"

# Graph with no stats
ok "$prog" -Q -G "$infile"
contains "Command 1: ls -l"
contains "Command 2: ps Aux"
missing "Total CPU time    4.63 ms"
no_stats
has_graph
no_boxplot
no_dist_stats
no_tail_stats
no_explanation
no_ranking
missing "Slower by" "8.92x"

# Boxplot with default stats
ok "$prog" -B "$infile"
contains "Command 1: ls -l" "Total CPU time    5.03 ms"
contains "Command 2: ps Aux" 
has_summary
no_graph
has_boxplot
no_dist_stats
no_tail_stats
no_explanation
has_ranking
contains "Slower by" "8.92x"

# Boxplot with no stats
ok "$prog" -B -Q "$infile"
missing "Total CPU time    4.63 ms"
no_stats
no_graph
has_boxplot
no_dist_stats
no_tail_stats
no_explanation
no_ranking
missing "Slower by" "8.92x"

# Boxplot with Stats (by default) and Tail and Distribution stats
ok "$prog" -BTD "$infile"
contains "Command 1: ls -l" "Total CPU time    5.03 ms"
contains "Command 2: ps Aux" 
has_summary
no_graph
has_boxplot
has_dist_stats
has_tail_stats
no_explanation
has_ranking
contains "Slower by" "ps Aux" "8.92x"

# Full report with explanations
ok "$prog" -TD -BE "$infile"
contains "Command 1: ls -l" "Total CPU time    5.03 ms"
contains "Command 2: ps Aux" 
has_summary
no_graph
has_boxplot
has_dist_stats
has_tail_stats
has_explanation
has_ranking
contains "Slower by" "ps Aux" "8.92x"

#
# -----------------------------------------------------------------------------
#

if [[ "$allpassed" == "1" ]]; then 
    printf "All tests passed.\n\n"
    exit 0
else
    exit -1
fi


