#/bin/bash

SHOWOUTPUT=

prog=../bestreport

printf "%s\n"   '-----------------------------'
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

# ------------------------------------------------------------------
# Input is CSV file of raw timing data.  Output is summary CSV file.
# ------------------------------------------------------------------

infile=raw1.csv
expectfile=summary1.csv
outfile=$(mktemp -p /tmp)

# Write summary stats (and get default report on terminal)
ok "$prog" --export-csv "$outfile" "$infile"
contains "Min       Q₁    Median      Q₃      Max"
contains "Command 1: ls -l" "Total CPU time    4.6 ms"
contains "Command 2: ps Aux" "8.87 times faster than ps Aux"
ok diff "$outfile" "$expectfile"

# Write summary stats (and get brief report on terminal)
ok "$prog" -R brief --export-csv "$outfile" "$infile"
contains "Min    Median     Max"
contains "Command 1: ls -l" "Total CPU time    4.6 ms"
contains "Command 2: ps Aux" "8.87 times faster than ps Aux"
ok diff "$outfile" "$expectfile"

# Write summary stats (and get no report on terminal)
ok "$prog" -R none --export-csv "$outfile" "$infile"
contains "8.87 times faster than ps Aux"
ok diff "$outfile" "$expectfile"

# ------------------------------------------------------------------
# Input is CSV of raw timing data.  Output is Hyperfine-style CSV.
# ------------------------------------------------------------------

infile=raw1.csv
expectfile=hf1.csv
outfile=$(mktemp -p /tmp)

# Write summary stats (and get default report on terminal)
ok "$prog" --hyperfine-csv "$outfile" "$infile"
contains "Min       Q₁    Median      Q₃      Max"
contains "Command 1: ls -l" "Total CPU time    4.6 ms"
contains "Command 2: ps Aux" "8.87 times faster than ps Aux"
ok diff "$outfile" "$expectfile"

# Write summary stats (and get brief report on terminal)
ok "$prog" -R brief --hyperfine-csv "$outfile" "$infile"
contains "Min    Median     Max"
contains "Command 1: ls -l" "Total CPU time    4.6 ms"
contains "Command 2: ps Aux" "8.87 times faster than ps Aux"
ok diff "$outfile" "$expectfile"

# Write summary stats (and get no report on terminal)
ok "$prog" -R none --hyperfine-csv "$outfile" "$infile"
contains "8.87 times faster than ps Aux"
ok diff "$outfile" "$expectfile"

if [[ $allpassed -eq 1 ]]; then 
    printf "All tests passed.\n\n"
    exit 0
else
    exit -1
fi


