#!/bin/bash
#  -*- Mode: Shell-script; -*-                                            

SHOWOUTPUT=

prog=../bestguess

printf "%s\n"   '--------------------------------------'
printf "%s\n"   "$0"
printf "%s\n"   'Raw data output tests'
printf "%s\n\n" '--------------------------------------'

declare -a output
allpassed=1

source ./common.sh

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

#
# -----------------------------------------------------------------------------
#

if [[ "$allpassed" == "1" ]]; then 
    printf "All tests passed.\n\n"
    exit 0
else
    exit -1
fi


