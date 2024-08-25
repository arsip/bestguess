#/bin/bash

SHOWOUTPUT=

prog=../bestguess

printf "%s\n"   '-------------------------------------------------'
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
contains "times faster than"

# Graph with default report
ok "$prog" -g /bin/bash
contains "Command 1" "/bin/bash" "Total CPU time" "Wall" "Mode" "Median" "0     " "     max"

# Graph with no report
ok "$prog" -R none -g /bin/bash
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
    for runs in $(seq 0 5); do
	# Default report
	ok "$prog" -w $warmups -r $runs /bin/bash
	if [[ $runs -eq 0 ]]; then
	    contains "No data"
	else
	    contains "Command 1" "/bin/bash" "Total CPU time" "Mode" "Median" "Context"
	fi

	# Brief report
	ok "$prog" -w $warmups -r $runs -R brief /bin/bash
	if [[ $runs -eq 0 ]]; then
	    contains "No data"
	else
	    contains "Command 1" "/bin/bash" "Total CPU time" "Wall" "Mode" "Median"
	fi

	# Graph
	ok "$prog" -w $warmups -r $runs -R=brief -g /bin/bash
	if [[ $runs -eq 0 ]]; then
	    contains "No data"
	else
	    contains "Command 1" "/bin/bash" "Total CPU time" "Wall" "Mode" "Median"
	    contains "0     "  "     max"
	fi

	# Boxplot
	ok "$prog" -w $warmups -r $runs -R=none -B /bin/bash
	if [[ $runs -eq 0 ]]; then
	    contains "No data"
	else
	    contains "├────" "─┼─"
	fi
    done
done




if [[ $allpassed -eq 1 ]]; then 
    printf "All tests passed.\n\n"
    exit 0
else
    exit -1
fi


