#!/bin/bash
#  -*- Mode: Shell-script; -*-                                            

prog="../bestguess"
report="../bestreport"

printf "%s\n"   '--------------------------------------'
printf "%s\n"   "$0"
printf "%s\n"   'Basic tests, e.g. checking exit status'
printf "%s\n\n" '--------------------------------------'

allpassed=1

function ok {
    "$@" >/dev/null 2>&1
    local status=$?
    if [[ $status -ne 0 ]]; then
	printf "Expected success. Failed with code %d: %s\n" $status "$*"
	allpassed=0
    fi
}

function usage {
    "$@" >/dev/null 2>&1
    local status=$?
    if [[ $status -ne 1 ]]; then
	printf "Expected usage error. Failed with code %d: %s\n" $status "$*"
	allpassed=0
    fi
}

function runtime {
    "$@" >/dev/null 2>&1
    local status=$?
    if [[ $status -ne 2 ]]; then
	printf "Expected runtime error. Failed with code %d: %s\n" $status "$*"
	allpassed=0
    fi
}

function panic {
    "$@" >/dev/null 2>&1
    local status=$?
    if [[ $status -ne 255 ]]; then
	printf "Expected panic. Failed with code %d: %s\n" $status "$*"
	allpassed=0
    fi
}

# Bad options
usage   "$prog"
usage   "$prog" -foo
usage   "$prog" -r abc
usage   "$prog" -w abc
usage   "$prog" -r -1
usage   "$prog" -w -1
usage   "$prog" -r 1048577
usage   "$prog" -w 1048577
usage   "$prog" --no-such-option
usage   "$prog" -d

# Missing commands
usage   "$prog" -r 0
usage   "$prog" -w 0
usage   "$prog" -r 1
usage   "$prog" -w 1
usage   "$prog" -r 10000
usage   "$prog" -w 10000
usage   "$prog" -r 1048576
usage   "$prog" -w 1048576

ok      "$prog" -v
ok      "$prog" -h

ok      "$report" -v
ok      "$report" -h
ok      "$report" pi1.csv

# Bad shell programs '-fasdkl' and foobarbaz
usage   "$prog" -s -fasdkl ls -l
runtime "$prog" -s -fasdkl "ls -l"
runtime "$prog" -s foobarbaz 'ls -l'
runtime "$prog" -s foobarbaz 'ls -l'

# Command cannot be executed
runtime "$prog" thisprogramshouldnotexist
runtime "$prog" /thisprogramshouldnotexist
runtime "$prog" ./thisprogramshouldnotexist
runtime "$prog" ./nosuchdirectory/thisprogramshouldnotexist

#
# -----------------------------------------------------------------------------
#

if [[ "$allpassed" == "1" ]]; then 
    printf "All tests passed.\n\n"
    exit 0
else
    exit -1
fi


