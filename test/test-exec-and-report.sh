#!/bin/bash

SHOWOUTPUT=

prog=../bestguess

printf "%s\n"   '-------------------------------------------------'
printf "%s\n"   "$0"
printf "%s\n"   'Bestguess exec and report tests (terminal output)'
printf "%s\n\n" '-------------------------------------------------'

declare -a output
allpassed=1

source ./common.sh

# ------------------------------------------------------------------
# TERMINAL output options
# ------------------------------------------------------------------

# Default report is summary
ok "$prog" /bin/bash
contains "Command 1" "/bin/bash"
has_stats
no_ranking
no_graph
no_boxplot

# Mini report
ok "$prog" -M /bin/bash
contains "Command 1" "/bin/bash"
has_mini_stats
no_ranking
no_graph
no_boxplot

# No report and only one program, so nothing to compare it with
ok "$prog" -N /bin/bash
missing "Command 1" "/bin/bash"
no_stats
no_ranking
no_graph
no_boxplot

# No report but comparison is printed
ok "$prog" -N /bin/bash ls
no_stats
has_ranking
no_statistical_ranking
no_boxplot

# Graph with default report
ok "$prog" -G /bin/bash
contains "Command 1" "/bin/bash"
has_stats
has_graph
no_boxplot
no_ranking

# Graph with no report
ok "$prog" -N -G /bin/bash
no_stats
has_graph
no_boxplot
no_ranking

# Boxplot
ok "$prog" -B /bin/bash
contains "Command 1" "/bin/bash" "Total CPU time" "Wall" "Mode" "Median" 
has_stats
no_graph
has_boxplot
no_ranking

# Boxplot with no report
ok "$prog" -N -B /bin/bash
no_stats
has_boxplot
no_graph
no_ranking

for warmups in $(seq 0 5); do
    for runs in $(seq 1 5); do
	# Default report
	ok "$prog" -w $warmups -r $runs /bin/bash
	has_stats
	no_graph
	no_boxplot
	no_ranking

	# Brief report
	ok "$prog" -w $warmups -r $runs -M /bin/bash
	has_mini_stats
	no_graph
	no_boxplot
	no_ranking
	
	# Graph
	ok "$prog" -w $warmups -r $runs -N -G /bin/bash
	no_stats
	has_graph
	no_boxplot
	no_ranking

	# Boxplot
	ok "$prog" -w $warmups -r $runs -N -B /bin/bash
	no_stats
	has_boxplot
	no_graph
	no_ranking
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


