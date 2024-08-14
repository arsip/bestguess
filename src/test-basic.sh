#/bin/bash
#  -*- Mode: Shell-script; -*-                                            

printf "Basic tests, e.g. checking exit status\n\n"

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

usage   ./bestguess
usage   ./bestguess -foo
usage   ./bestguess -r abc
usage   ./bestguess -w abc
usage   ./bestguess -r -1
usage   ./bestguess -w -1
usage   ./bestguess -r 1048577
usage   ./bestguess -w 1048577
usage   ./bestguess --no-such-option
usage   ./bestguess -d

ok      ./bestguess -h
ok      ./bestguess -r 0
ok      ./bestguess -w 0
ok      ./bestguess -r 1
ok      ./bestguess -w 1
ok      ./bestguess -r 10000
ok      ./bestguess -w 10000
ok      ./bestguess -r 1048576
ok      ./bestguess -w 1048576

# Bad shell programs '-fasdkl' and foobarbaz
usage   ./bestguess -S -fasdkl ls -l
runtime ./bestguess -S -fasdkl "ls -l"
runtime ./bestguess -S foobarbaz 'ls -l'
runtime ./bestguess -S foobarbaz 'ls -l'

# Command cannot be executed
runtime ./bestguess thisprogramshouldnotexist
runtime ./bestguess /thisprogramshouldnotexist
runtime ./bestguess ./thisprogramshouldnotexist
runtime ./bestguess ./nosuchdirectory/thisprogramshouldnotexist



if [[ $allpassed -eq 1 ]]; then 
    printf "All tests passed.\n"
    exit 0
else
    exit -1
fi


