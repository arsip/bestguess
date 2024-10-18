#!/bin/bash

SHOWOUTPUT=

prog=../bestreport

printf "%s\n"   '-----------------------------------'
printf "%s\n"   "$0"
printf "%s\n"   'Bestreport tests (terminal output)'
printf "%s\n\n" '-----------------------------------'

declare -a output
allpassed=1

function ok {
    command="$*"
    output=$(${@})
    local status=$?
    if [[ -n "$SHOWOUTPUT" ]]; then
	printf "\n"
	echo "$output"
    fi
    if [[ $status -ne 0 ]]; then
	printf "Expected success. Failed with code %d: %s\n" $status "$command"
	allpassed=0
    fi
}

function contains {
    for str in "$@"; do
	if [[ "$output" != *"$str"* ]]; then
	    printf "Output did not contain '%s': %s\n" "$str" "$command"
	    allpassed=0
	fi
    done
}

function missing {
    for str in "$@"; do
	if [[ "$output" == *"$str"* ]]; then
	    printf "Output contained '%s': %s\n" "$str" "$command"
	    allpassed=0
	fi
    done
}

# ------------------------------------------------------------------
# Use a CSV file of raw timing data as the input
# ------------------------------------------------------------------

infile=raw1.csv

# Default report is summary
ok "$prog" "$infile"
contains "Min      Q₁    Median      Q₃       Max"
contains "Command 1: ls -l" "Total CPU time    5.03 ms" 
contains "Command 2: ps Aux" "Mode" "Min" "Q₁" "Median" "Q₃" "Max"
contains "Best guess ranking" "Slower by" "797.6%"
missing "Min   Median      Max"
missing "▭▭▭▭▭▭"
missing "├─────────┼─"
missing "Best guess inferential statistics"
missing "Mann-Whitney"
missing "Total CPU Time Distribution"
missing "Total CPU Time Distribution Tail"

# Explicitly request the summary
ok "$prog" -R summary "$infile"
contains "Min      Q₁    Median      Q₃       Max"
contains "Command 1: ls -l" "Total CPU time    5.03 ms" 
contains "Command 2: ps Aux" "Mode" "Min" "Q₁" "Median" "Q₃" "Max"
contains "Best guess ranking" "Slower by" "797.6%"
missing "Min   Median      Max"
missing "▭▭▭▭▭▭"
missing "├─────────┼─"
missing "Best guess inferential statistics"
missing "Mann-Whitney"
missing "Total CPU Time Distribution"
missing "Total CPU Time Distribution Tail"

# Brief report
ok "$prog" -R brief "$infile"
contains "Min   Median      Max"
contains "Command 1: ls -l" "Total CPU time    5.03 ms" 
contains "Command 2: ps Aux" "Mode"
contains "Best guess ranking" "Slower by" "797.6%"
missing "Min      Q₁    Median      Q₃       Max"
missing "▭▭▭▭▭▭"
missing "├─────────┼─"
missing "Best guess inferential statistics"
missing "Mann-Whitney"
missing "Total CPU Time Distribution"
missing "Total CPU Time Distribution Tail"

# No report, but summary prints
ok "$prog" -R none "$infile"
contains "Best guess ranking" "Slower by" "797.6%"
missing "Command 1: ls -l" "Total CPU time    5.03 ms" 
missing "Command 2: ps Aux" "Mode" "Min" "Q₁" "Median" "Q₃" "Max"
missing "Min      Q₁    Median      Q₃       Max"
missing "Min   Median      Max"
missing "▭▭▭▭▭▭"
missing "├─────────┼─"
missing "Best guess inferential statistics"
missing "Mann-Whitney"
missing "Total CPU Time Distribution"
missing "Total CPU Time Distribution Tail"

# Graph with default report
ok "$prog" -G "$infile"
contains "Min      Q₁    Median      Q₃       Max"
contains "Command 1: ls -l" "Total CPU time    5.03 ms"
contains "Command 2: ps Aux" "Mode" "Min" "Q₁" "Median" "Q₃" "Max"
contains "Best guess ranking" "Slower by" "797.6%"
contains "0     " "     max"
contains "│▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭"
missing "Min   Median      Max"
missing "├─────────┼─"
missing "Best guess inferential statistics"
missing "Mann-Whitney"
missing "Total CPU Time Distribution"
missing "Total CPU Time Distribution Tail"

# Graph with no report
ok "$prog" -R none -G "$infile"
contains "Command 1: ls -l"
missing "Total CPU time    4.63 ms"
contains "Command 2: ps Aux"
missing "Mode" "Min" "Q₁" "Median" "Q₃" "Max"
contains "Slower by" "ps Aux" "797.6%"
contains "0     " "     max"
contains "│▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭"
missing "Min   Median      Max"
missing "├─────────┼─"
missing "Best guess inferential statistics"
missing "Mann-Whitney"
missing "Total CPU Time Distribution"
missing "Total CPU Time Distribution Tail"

# Boxplot with default report
ok "$prog" -B "$infile"
contains "Min      Q₁    Median      Q₃       Max"
contains "Command 1: ls -l" "Total CPU time    5.03 ms"
contains "Command 2: ps Aux"
contains "Slower by" "ps Aux" "797.6%"
contains "├────" "─┼─"
missing "Min   Median      Max"
missing "Best guess inferential statistics"
missing "Mann-Whitney"
missing "Total CPU Time Distribution"
missing "Total CPU Time Distribution Tail"

# Boxplot with no report
ok "$prog" -R none -B "$infile"
contains "├────" "─┼─"
missing "Min      Q₁    Median      Q₃       Max"
missing "Command 1: ls -l" "Total CPU time    5.03 ms"
missing "Command 2: ps Aux"
contains "Slower by" "ps Aux" "797.6%"
missing "Min   Median      Max"
missing "Best guess inferential statistics"
missing "Mann-Whitney"
missing "Total CPU Time Distribution"
missing "Total CPU Time Distribution Tail"

# Full report
ok "$prog" -R full -B "$infile"
contains "Slower by" "ps Aux" "797.6%"
contains "├────" "─┼─"
contains "Min      Q₁    Median      Q₃       Max"
contains "Command 1: ls -l" "Total CPU time    5.03 ms"
missing "Min   Median      Max"
missing "Best guess inferential statistics"
missing "Mann-Whitney"
contains "Total CPU Time Distribution"
contains "Total CPU Time Distribution Tail"

# Full report with explanations
ok "$prog" -R full -BE "$infile"
contains "├────" "─┼─"
contains "Min      Q₁    Median      Q₃       Max"
contains "Command 1: ls -l" "Total CPU time    5.03 ms"
contains "Slower by" "ps Aux" "797.6%"
missing "Min   Median      Max"
contains "Best guess inferential statistics"
contains "Mann-Whitney"
contains "Total CPU Time Distribution"
contains "Total CPU Time Distribution Tail"

#
# -----------------------------------------------------------------------------
#

if [[ "$allpassed" == "1" ]]; then 
    printf "All tests passed.\n\n"
    exit 0
else
    exit -1
fi


