#/bin/bash
#  -*- Mode: Shell-script; -*-                                            

SHOWOUTPUT=

prog=../bestguess

printf "%s\n"   '--------------------------------------'
printf "%s\n"   'Report tests (terminal output)'
printf "%s\n\n" '--------------------------------------'

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

# Default report
ok "$prog" /bin/bash
contains "Command 1" "/bin/bash" "Total CPU time" "Mode" "Median" "Context"

# Brief report
ok "$prog" -b /bin/bash
contains "Command 1" "/bin/bash" "Total CPU time" "Wall" "Mode" "Median"

# Graph
ok "$prog" -bg /bin/bash
contains "Command 1" "/bin/bash" "Total CPU time" "Wall" "Mode" "Median" "0     " "     max"

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
	ok "$prog" -w $warmups -r $runs -b /bin/bash
	if [[ $runs -eq 0 ]]; then
	    contains "No data"
	else
	    contains "Command 1" "/bin/bash" "Total CPU time" "Wall" "Mode" "Median"
	fi
	# Graph
	ok "$prog" -w $warmups -r $runs -bg /bin/bash
	if [[ $runs -eq 0 ]]; then
	    contains "No data"
	else
	    contains "Command 1" "/bin/bash" "Total CPU time" "Wall" "Mode" "Median"
	    contains "0     "  "     max"
	fi
    done
done




if [[ $allpassed -eq 1 ]]; then 
    printf "All tests passed.\n\n"
    exit 0
else
    exit -1
fi


