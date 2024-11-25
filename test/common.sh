
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

function missing {
    for str in "$@"; do
	if [[ "$output" == *"$str"* ]]; then
	    printf "Output contained '%s': %s\n" "$str" "$command"
	    allpassed=0
	fi
    done
}

# -----------------------------------------------------------------------------

function has_summary {
    contains "Command 1"
    contains "Mode" "Min" "Q₁" "Median" "Q₃" "Max"
    contains "Total CPU time"  "User time"  "System time"
    contains "Context sw"  "Max RSS"  "Wall clock"
}

function no_summary {
    missing "Context sw"  "Max RSS"
}

function no_stats {
    missing "Min" "Q₁" "Median" "Q₃" "Max"
    missing "Total CPU time"  "Wall clock"
    missing "Context sw"  "Max RSS"  "Wall clock"
}

function has_mini_stats {
    contains "Command 1"
    contains "Min   Median      Max"
    missing  "Q₁"  "Q₃"
    contains "Total CPU time"  "Wall clock"
    missing  "User time"  "System time"  "Context sw"  "Max RSS"
}

function has_graph {
    contains "Command 1"
    contains "│▭▭▭▭▭▭▭▭" "max"
}

function no_graph {
    missing "│▭▭▭▭▭▭▭▭" "max"
}

function has_boxplot {
    contains "legend"
    contains "├────" "─┼─"
}

function no_boxplot {
    missing "├────" "─┼─"
}

function has_statistical_ranking {
    missing "Lacking the"  "timed runs to statistically rank"
}

function no_statistical_ranking {
    contains "Lacking the"  "timed runs to statistically rank"
}

function has_ranking {
    contains "Best guess ranking"
    contains "Command ═════════════" "Total time ═════" "Slower by"
}

function no_ranking {
#    contains "No ranking"
    missing "Command ═════════" "Total time ═════" " Slower by"
    missing "Lacking the"  "timed runs to statistically rank"
}

function has_explanation {
    contains "Best guess inferential statistics"
    contains "Mann-Whitney"
    contains "Hodges-Lehmann"
}

function no_explanation {
    missing "Best guess inferential statistics"
    missing "Mann-Whitney"
    missing "Hodges-Lehmann"
}

function has_tail_stats {
    contains "Total CPU Time Distribution Tail" "Tail shape"
}

function no_tail_stats {
    missing "Total CPU Time Distribution Tail" "Tail shape"
}

function has_dist_stats {
    contains "Total CPU Time Distribution"
    contains "normality" "Skew" "kurtosis"
}

function no_dist_stats {
    missing "Total CPU Time Distribution"
    missing "normality" "Skew" "kurtosis"
}
