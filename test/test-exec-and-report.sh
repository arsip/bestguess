#!/bin/bash

SHOWOUTPUT=

prog=../bestguess

printf "%s\n"   '-------------------------------------------------'
printf "%s\n"   "$0"
printf "%s\n"   'Bestguess exec and report tests (terminal output)'
printf "%s\n\n" '-------------------------------------------------'

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
# TERMINAL output options
# ------------------------------------------------------------------

# Default report is summary
ok "$prog" /bin/bash
contains "Command 1" "/bin/bash" "Total CPU time" "Mode" "Median" "Context"
ok "$prog" -R summary /bin/bash
contains "Command 1" "/bin/bash" "Total CPU time" "Mode" "Median" "Context"

# Brief report
ok "$prog" -R brief /bin/bash
contains "Command 1" "/bin/bash" "Total CPU time" "Wall" "Mode" "Median"

# No report and only one program, so nothing to compare it with
ok "$prog" -R none /bin/bash

# No report but comparison is printed
ok "$prog" -R none /bin/bash ls
contains "Minimum observations"

# Graph with default report
ok "$prog" -G /bin/bash
contains "Command 1" "/bin/bash" "Total CPU time" "Wall" "Mode" "Median" "0     " "     max"

# Graph with no report
ok "$prog" -R none -G /bin/bash
contains "Command 1" "/bin/bash"
contains "│▭▭▭▭▭▭▭▭" "0" "max"

# Boxplot
ok "$prog" -B /bin/bash
contains "Command 1" "/bin/bash" "Total CPU time" "Wall" "Mode" "Median" 
contains "├────" "─┼─"

# Boxplot with no report
ok "$prog" -R none -B /bin/bash
contains "├────" "─┼─"

for warmups in $(seq 0 5); do
    for runs in $(seq 1 5); do
	# Default report
	ok "$prog" -w $warmups -r $runs /bin/bash
	contains "Command 1" "/bin/bash" "Total CPU time" "Mode" "Median" "Context"

	# Brief report
	ok "$prog" -w $warmups -r $runs -R brief /bin/bash
	contains "Command 1" "/bin/bash" "Total CPU time" "Wall" "Mode" "Median"

	# Graph
	ok "$prog" -w $warmups -r $runs -R=brief -G /bin/bash
	contains "Command 1" "/bin/bash" "Total CPU time" "Wall" "Mode" "Median"
	contains "0     "  "     max"

	# Boxplot
	ok "$prog" -w $warmups -r $runs -R=none -B /bin/bash
	contains "├────" "─┼─"
    done
done


#
# -----------------------------------------------------------------------------
#

if [[ "$allpassed" == "1" ]]; then 
    printf "All tests passed.\n\n"
    exit 0
else
    exit -1
fi


