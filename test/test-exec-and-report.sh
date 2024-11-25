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

# Default report is summary and ranking
ok "$prog" /bin/bash
contains "Command 1" "/bin/bash"
has_summary
no_ranking
no_graph
no_boxplot

# Mini report and ranking
ok "$prog" -M /bin/bash
contains "Command 1" "/bin/bash"
has_mini_stats
no_graph
no_boxplot
no_ranking

# No stats and only one program, so nothing to compare it with
ok "$prog" -QR /bin/bash
missing "Command 1" "/bin/bash"
no_stats
no_ranking
no_graph
no_boxplot

# Summary stats but only one program, so nothing to compare it with
ok "$prog" -QRS /bin/bash
contains "No ranking"
has_summary
no_ranking
no_graph
no_boxplot

# No output
ok "$prog" -Q /bin/bash ls
no_stats
no_ranking
no_boxplot

# No report but comparison is printed
ok "$prog" -QR /bin/bash ls
no_stats
has_ranking
no_statistical_ranking
no_boxplot

# Graph with default report
ok "$prog" -G /bin/bash
contains "Command 1" "/bin/bash"
has_summary
has_graph
no_boxplot

# Graph only
ok "$prog" -QG /bin/bash
no_stats
has_graph
no_boxplot

# Graph and ranking requested, but only one command
ok "$prog" -QGR /bin/bash
no_stats
contains "No ranking"
has_graph
no_boxplot

# Boxplot
ok "$prog" -B /bin/bash
contains "Command 1" "/bin/bash" "Total CPU time" "Wall" "Mode" "Median" 
has_summary
no_graph
has_boxplot
contains "No ranking"

# Boxplot with no reports
ok "$prog" -Q -B /bin/bash
no_stats
has_boxplot
no_graph
no_ranking
missing "No ranking"

for warmups in $(seq 0 5); do
    for runs in $(seq 1 5); do
	# Default report
	ok "$prog" -w $warmups -r $runs /bin/bash
	has_summary
	no_graph
	no_boxplot

	# Brief report
	ok "$prog" -w $warmups -r $runs -M /bin/bash
	has_mini_stats
	no_graph
	no_boxplot
	
	# Graph
	ok "$prog" -w $warmups -r $runs -Q -G /bin/bash
	no_stats
	has_graph
	no_boxplot

	# Boxplot
	ok "$prog" -w $warmups -r $runs -Q -B /bin/bash
	no_stats
	has_boxplot
	no_graph
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


