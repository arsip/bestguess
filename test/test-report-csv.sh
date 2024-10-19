#!/bin/bash

SHOWOUTPUT=

prog=../bestreport

printf "%s\n"   '-----------------------------'
printf "%s\n"   "$0"
printf "%s\n"   'Bestreport tests (CSV output)'
printf "%s\n\n" '-----------------------------'

declare -a output
allpassed=1

source ./common.sh

# ------------------------------------------------------------------
# Input is CSV file of raw timing data.  Output is summary CSV file.
# ------------------------------------------------------------------

infile=raw1.csv
expectfile=summary1.csv
outfile=$(mktemp -p /tmp)

# Write summary stats (and get default report on terminal)
ok "$prog" --export-csv "$outfile" "$infile"
contains "Command 1: ls -l" "Total CPU time    5.03 ms" 
contains "Command 2: ps Aux" 
has_stats
no_graph
no_boxplot
no_dist_stats
no_tail_stats
no_explanation
has_ranking
contains "Best guess ranking" "Slower by" "797.6%"
ok diff "$outfile" "$expectfile"

# Write summary stats and print mini stats
ok "$prog" -M --export-csv "$outfile" "$infile"
has_mini_stats
contains "Command 1: ls -l" "Total CPU time    5.03 ms" 
contains "Command 2: ps Aux" 
no_graph
no_boxplot
no_dist_stats
no_tail_stats
no_explanation
has_ranking
contains "Best guess ranking" "Slower by" "797.6%"
ok diff "$outfile" "$expectfile"

# Write summary stats (and get no stats on terminal)
ok "$prog" -N --export-csv "$outfile" "$infile"
missing "Command 1: ls -l" "Total CPU time    5.03 ms" 
missing "Command 2: ps Aux" 
no_stats
no_graph
no_boxplot
no_dist_stats
no_tail_stats
no_explanation
has_ranking
contains "Best guess ranking" "Slower by" "797.6%"
ok diff "$outfile" "$expectfile"

# ------------------------------------------------------------------
# Input is CSV of raw timing data.  Output is Hyperfine-style CSV.
# ------------------------------------------------------------------

infile=raw1.csv
expectfile=hf1.csv
outfile=$(mktemp -p /tmp)

# Write summary stats (and get default stats on terminal)
ok "$prog" --hyperfine-csv "$outfile" "$infile"
contains "Command 1: ls -l" "Total CPU time    5.03 ms" 
contains "Command 2: ps Aux" 
has_stats
no_graph
no_boxplot
no_dist_stats
no_tail_stats
no_explanation
has_ranking
contains "Best guess ranking" "Slower by" "797.6%"

ok diff "$outfile" "$expectfile"

# Write summary stats (and get brief report on terminal)
ok "$prog" -M --hyperfine-csv "$outfile" "$infile"
has_mini_stats
contains "Command 1: ls -l" "Total CPU time    5.03 ms" 
contains "Command 2: ps Aux" 
no_graph
no_boxplot
no_dist_stats
no_tail_stats
no_explanation
has_ranking
contains "Best guess ranking" "Slower by" "797.6%"
ok diff "$outfile" "$expectfile"

# Write summary stats (and get no report on terminal)
ok "$prog" -N --hyperfine-csv "$outfile" "$infile"
missing "Command 1: ls -l" "Total CPU time    5.03 ms" 
missing "Command 2: ps Aux" 
no_stats
no_graph
no_boxplot
no_dist_stats
no_tail_stats
no_explanation
has_ranking
contains "Best guess ranking" "Slower by" "797.6%"
ok diff "$outfile" "$expectfile"

#
# -----------------------------------------------------------------------------
#

if [[ "$allpassed" == "1" ]]; then 
    printf "All tests passed.\n\n"
    exit 0
else
    exit -1
fi


