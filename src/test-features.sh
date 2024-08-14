#/bin/bash
#  -*- Mode: Shell-script; -*-                                            

DEBUG=true

printf "Feature tests\n"

declare -a output
allpassed=1

function ok {
    output=$(${@})
    local status=$?
    if [[ -n "$DEBUG" ]]; then
	printf "\n"
	echo "$output"
    fi
    if [[ $status -ne 0 ]]; then
	printf "Expected success. Failed with code %d: %s\n" $status "$*"
	allpassed=0
    fi
}

function contains {
    for str in "$@"; do
	if [[ "$output" != *"$str"* ]]; then
	    printf "Output did not contain '%s'\n" "$str"
	    allpassed=0
	fi
    done
}

# ------------------------------------------------------------------
# TERMINAL Output options
# ------------------------------------------------------------------

# Default report
ok ./bestguess /bin/bash
contains "Command 1" "/bin/bash" "Total CPU time" "Mode" "Median" "Context"

# Brief report
ok ./bestguess -b /bin/bash
contains "Command 1" "/bin/bash" "Total CPU time" "Wall" "Mode" "Median"

# Graph
ok ./bestguess -bg /bin/bash
contains "Command 1" "/bin/bash" "Total CPU time" "Wall" "Mode" "Median" "0     " "     max"

for warmups in $(seq 0 5); do
    for runs in $(seq 0 5); do
	# Default report
	ok ./bestguess -w $warmups -r $runs /bin/bash
	if [[ $runs -eq 0 ]]; then
	    contains "No data"
	else
	    contains "Command 1" "/bin/bash" "Total CPU time" "Mode" "Median" "Context"
	fi

	# Brief report
	ok ./bestguess -w $warmups -r $runs -b /bin/bash
	if [[ $runs -eq 0 ]]; then
	    contains "No data"
	else
	    contains "Command 1" "/bin/bash" "Total CPU time" "Wall" "Mode" "Median"
	fi
	# Graph
	ok ./bestguess -w $warmups -r $runs -bg /bin/bash
	if [[ $runs -eq 0 ]]; then
	    contains "No data"
	else
	    contains "Command 1" "/bin/bash" "Total CPU time" "Wall" "Mode" "Median"
	    contains "0     "  "     max"
	fi
    done
done

# ------------------------------------------------------------------
# RAW DATA output
# ------------------------------------------------------------------




if [[ $allpassed -eq 1 ]]; then 
    printf "\nAll tests passed.\n"
    exit 0
else
    exit -1
fi


