#/bin/bash

SHOWOUTPUT=

prog=../bestreport

printf "%s\n"   '-----------------------------------'
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

# ------------------------------------------------------------------
# Use a CSV file of raw timing data as the input
# ------------------------------------------------------------------

infile=raw1.csv

# Default report is summary
ok "$prog" "$infile"
contains "Min      Q₁    Median      Q₃       Max"
contains "Command #1: ls -l" "Total CPU time    4.63 ms" 
contains "Command #2: ps Aux" "8.87 times faster than #2: ps Aux"

# Explicitly request the summary
ok "$prog" -R summary "$infile"
contains "Min      Q₁    Median      Q₃       Max"
contains "Command #1: ls -l" "Total CPU time    4.63 ms"
contains "Command #2: ps Aux" "8.87 times faster than #2: ps Aux"

# Brief report
ok "$prog" -R brief "$infile"
contains "Min   Median      Max"
contains "Command #1: ls -l" "Total CPU time    4.63 ms"
contains "Command #2: ps Aux" "8.87 times faster than #2: ps Aux"

# No report
ok "$prog" -R none "$infile"
contains "8.87 times faster than #2: ps Aux"

# Graph with default report
ok "$prog" -g "$infile"
contains "Min      Q₁    Median      Q₃       Max"
contains "Command #1: ls -l" "Total CPU time    4.63 ms"
contains "Command #2: ps Aux" "8.87 times faster than #2: ps Aux"
contains "0     " "     max"
contains "│▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭"
contains "│▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭"

# Graph with no report
ok "$prog" -R none -g "$infile"
contains "8.87 times faster than #2: ps Aux"
contains "0     " "     max"
contains "│▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭"
contains "│▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭"

# Boxplot
ok "$prog" -B "$infile"
contains "Min      Q₁    Median      Q₃       Max"
contains "Command #1: ls -l" "Total CPU time    4.63 ms"
contains "Command #2: ps Aux" "8.87 times faster than #2: ps Aux"
contains "8.87 times faster than #2: ps Aux"
contains "├────" "─┼─"

# Boxplot with no report
ok "$prog" -R none -B "$infile"
contains "8.87 times faster than #2: ps Aux"
contains "├────" "─┼─"

if [[ $allpassed -eq 1 ]]; then 
    printf "All tests passed.\n\n"
    exit 0
else
    exit -1
fi


