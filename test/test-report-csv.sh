#!/bin/bash

SHOWOUTPUT=

prog=../bestreport

printf "%s\n"   '-----------------------------'
printf "%s\n"   "$0"
printf "%s\n"   'Bestreport tests (CSV output)'
printf "%s\n\n" '-----------------------------'

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
# Input is CSV file of raw timing data.  Output is summary CSV file.
# ------------------------------------------------------------------

infile=raw1.csv
expectfile=summary1.csv
outfile=$(mktemp -p /tmp)

# Write summary stats (and get default report on terminal)
ok "$prog" --export-csv "$outfile" "$infile"
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
ok diff "$outfile" "$expectfile"

# Write summary stats (and get brief report on terminal)
ok "$prog" -R brief --export-csv "$outfile" "$infile"
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
ok diff "$outfile" "$expectfile"

# Write summary stats (and get no report on terminal)
ok "$prog" -R none --export-csv "$outfile" "$infile"
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
ok diff "$outfile" "$expectfile"

# ------------------------------------------------------------------
# Input is CSV of raw timing data.  Output is Hyperfine-style CSV.
# ------------------------------------------------------------------

infile=raw1.csv
expectfile=hf1.csv
outfile=$(mktemp -p /tmp)

# Write summary stats (and get default report on terminal)
ok "$prog" --hyperfine-csv "$outfile" "$infile"
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
ok diff "$outfile" "$expectfile"

# Write summary stats (and get brief report on terminal)
ok "$prog" -R brief --hyperfine-csv "$outfile" "$infile"
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
ok diff "$outfile" "$expectfile"

# Write summary stats (and get no report on terminal)
ok "$prog" -R none --hyperfine-csv "$outfile" "$infile"
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


