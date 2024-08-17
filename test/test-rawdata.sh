#/bin/bash
#  -*- Mode: Shell-script; -*-                                            

SHOWOUTPUT=

prog=../bestguess

printf "%s\n"   '--------------------------------------'
printf "%s\n"   'Raw data output tests'
printf "%s\n\n" '--------------------------------------'

declare -a output
allpassed=1

function ok {
    output=$(${@})
    local status=$?
    if [[ -n "$SHOWOUTPUT" ]]; then
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
# RAW DATA output
# ------------------------------------------------------------------
ofile=$(mktemp)
ok "$prog" -o "$ofile" /bin/bash
lines=$(wc -l "$ofile" | awk '{print $1}')
if [[ $lines -ne 2 ]]; then
    printf "Expected 2 lines in output file, saw $lines \n"
    allpassed=0
fi



if [[ $allpassed -eq 1 ]]; then 
    printf "All tests passed.\n"
    exit 0
else
    exit -1
fi


