# Best Guess

## Description

The goal, in short, is to replace Hyperfine (a popular benchmarking tool) in my
work.  I built BestGuess to provide more accurate measurements and well-founded
statistical calculations.

* Use `bestguess` to conduct experiments (measure run times, memory usage, and more).
* Use `bestreport` to produce a variety of statistical analyses and also cheap
  plots on the terminal.

BestGuess is a single executable, and all of the reports can be obtained at the
time an experiment is run.  I recommend using the `-o` (output) option of
`bestguess` to save the raw data to a CSV file.  Later, reports can be re-run as
needed using `bestreport`.

## Build and run

To build BestGuess in release mode:

```
make
```

To install to default location:

```
make install
```

To install to custom location `/some/path/bin`:

```
make DESTDIR=/some/path install
```

## Philosphy

* **BestGuess and Hyperfine are tools, not oracles.**  Neither the tools nor their
  makers know anything about the experiments you are doing.  (Measuring
  performance is an experiment.)
  - You may be measuring cold start times, or you may be looking for the best
    possible performance, which probably requires warmed-up caches, etc.
  - The tool should not advise you to use warmup runs because we don't know what
    you're trying to measure.  The tool should run untimed warmup runs at your
    direction.
  - The tool should not automatically select and employ a shell to run your
    experiment, nor estimate and substract the shell startup time.  The tool
    should give you a way to run your experiment using any shell, and to measure
    that shell's startup time yourself.
* **A tool like BestGuess should not get in your way.**
  - We should provide measurements as accurately as possible,
  - And collect the raw data, so that
  - You can analyze the data in whatever way is appropriate.
  - Though we can provide enough descriptive statistics to help you decide what
    to measure and how.
* **Descriptive statistics are important.**  They summarize the distribution of
  measurements in a handful of numbers.
  - A distribution of performance measurements, even if it contains a large
    number of data points, is not likely to produce a normal distribution.
  - The mean and standard deviation are not useful statistics
    here.  The median and interquartile range are more appropriate measures of
    central tendency and spread.
  - The best guess for what is a "typical" run time is the mode, at least for
    unimodal distributions.
  - There are no outliers.  A long running time is just as valid a data point as
    a short one.  The proper way to deal with such "outliers" is not to rerun an
    experiment until it does not produce any.  Simply citing the median or mode
    run time of the actual measurements collected will do.

## If you already use Hyperfine

BestGuess does not support all of the options that Hyperfine does.  And
BestGuess is currently tested only on Unix (macos) and Linux (several distros).

If you use Hyperfine like we used to, it's nearly a drop-in replacement.  We
most often used the warmup, runs, shell, and CSV export options.  BestGuess can
produce a Hyperfine-compatible CSV file containing summary statistics, as
well as its own format.

The BestGuess `--hyperfine-csv <FILE>` option is used wherever you would use
`--export-csv <FILE>` with Hyperfine to get essentially the same summary
statistics in CSV form.  Two important differences:
  1. BestGuess does not calculate arithmetic mean.  In the summary file, the
     `mean` column has been replaced by `mode`.  The median and mode values of
     total CPU time are each more representative than mean for skewed and
     heavy-tailed distributions.
  2. BestGuess does not calculate standard deviation because we can show that
     performance distributions are rarely normal.  Instead, the file produced by
     `--hyperfine-csv` contains the interquartile range instead.

These Hyperfine options are effectively the same in BestGuess:
  * `-w`, `--warmup`
  * `-r`, `--runs`
  * `-i`, `--ignore-failure`
  * `-s`, `--shell` (Note the change to lower-case `s`, and see the caveat below.)
  * `-n`, `--name`
  * `-p`, `--prepare` (The long name will be changing to `--pre`.)
  * `--show-output`

Key changes from Hyperfine:

  * You can run an experiment (measure command executions) using `bestguess` and
    save the raw timing data as a CSV file.  While running the experiment, you
    can get a variety of reports, but you can also run `bestreport` later,
    providing one or more files of raw timing data as input.
  * Many Hyperfine options are _not_ supported in BestGuess.
    * Some are planned, like _setup_, _conclude_ and _cleanup_, as is the
      ability to randomize the environment size.
	* Other options, like the ones that automate parameter generations, are not
      planned.  BestGuess can read commands from a file, and we think it's
      easier to generate a command file with the desired parameters.
  * One option works a little differently.
	* `-s`, `--shell <CMD>` For BestGuess, the default is no shell.  And when
	  this option is used, you must provide the entire shell command,
	  e.g. `/bin/bash -c`.
  * Many options are _new_ because they support unique BestGuess features.  Here
    are a few:
    * `-o`, `--output <FILE>` (save raw data for individual executions)
    * `-f`, `--file <FILE>` (read commands, or _more_ commands, from a file)
    * `-G`, `--graph` (show graph of one command's total CPU time)
    * `-B`, `--boxplot` (show rough boxplots of total CPU time on terminal)
	* `-D`, `--dist-stats` (describes a distribution's relation to normal)
	* `-T`, `--tail-stats` (describes tail of a distribution)
    * `-M`, `--mini-stats` (show key time stats only)
    * `-N`, `--no-stats` (do not show summary stats for each command)

**Reports:** Best practice is to save raw measurement data (which includes CPU
times, max RSS, page faults, and context switch counts).  Once saved via `-o
<FILE>`, you can later do any of the following:
    - Reproduce the summary statistics that were printed on the terminal when
      the experiment was conducted: `bestreport <INFILE> ...`
	  - Note that many input files can be given, so results of separate
        experiments can be compared.
	- See an analysis of the distribution of total run times recorded for each
      command: `bestreport -D <INFILE> ...`
	  - The distribution analysis provides evidence for confidently ruling out
        that a distribution is normal, using various statistical measures.
	- Examine a rough boxplot on the terminal comparing the various commands:
      `bestreport -B <INFILE> ...`
	- Generate a CSV file of summary statistics: `bestreport --export-csv
      <OUTFILE> <INFILE> ...`

**Shell usage:** For BestGuess, the default is to _not_ use a shell at all.  If
you supply a shell, you will need to give the entire command including the `-c`
that most shells require.  Rationale: BestGuess should not supply a missing `-c`
shell argument.  It doesn't know what shell you are using or how to invoke it.
You may be launching a program with `/usr/bin/env` or some other utility.

**Shell startup time:** If you want to measure shell startup time, supply the
`-S` option and use `""` (empty string) for one of the commands.  This starts
the shell and then runs, as you would expect, no command.  Rationale: Hidden
measurements are bad science.  If you want to measure something, do so
explicitly.  Subtracting the estimated shell startup time from the measured
command times should certainly be done explicitly so that is transparent.

**Input file\:** You can supply commands, one to a line, in a file.  BestGuess
first executes any commands supplied on the command line, and then if an input
file was given, it runs the commands given there.  We sometimes generate our
commands using a script, and recommend this.  We like the flexibility and
transparency more than asking a benchmarking tool like BestGuess or Hyperfine to
step through a range or set of parameter values.

**Output file\:** BestGuess saves the "raw data", i.e. the measurements from
every timed command.  With the `-o` option you specify where it should be saved.
From a "good science" perspective, we want to save the raw data.  Retaining that
data allows multiple kinds of analysis to be done, either immediately or in the
future. 

Note that Hyperfine will export the raw timing data only in JSON format.  We
find that many more tools, from command line tools to spreadsheets, are able to
process CSV data, so we prefer it.  Also, the Hyperfine JSON output contains the
total time for each run, but not the user and system times.  BestGuess saves all
of the measured data from `rusage`, plus the wall clock time.

**Summary statistics:** BestGuess prints summary statistics to the terminal,
provided the raw data output is not directed there.  BestGuess estimates the
mode, which for unimodal distributions is the definition of "most common value
seen".  Distributions of performance data are often statistically _not_ normal
and are often skewed, so we do not show a mean or standard deviation.  The
summary statistics provided are mode, min, mean, 95th and 99th percentiles, and
max. 

**Descriptive statistics:** The "distribution statistics" option provides an
overview of the distribution of total CPU times for a single command.  (Total
CPU time is the sum of user time and system time.)  A statistical test for
normality is performed.  When the result is significant, we can rule out the
hypothesis that the total time is normally distributed.  A simple measure of
skew is also performed, and a measure of excess kurtosis.  Of course, the usual
descriptive statistics are displayed (Q0 or min, Q1, Q2 or median, Q3, Q4 or
max), along with the number of data points and an interpretation of the
interquartile range.

**Tail statistics:** The "tail statistics" option displays the
statistical summary (mode, min, mean, 95th and 99th percentiles, and max) for
each measured quantity, but also analyzes the distribution of total CPU times. 

**Measurements:** Hyperfine measures user and system time.  BestGuess measures
these plus the maximum RSS size, the number of Page Reclaims and Faults, and the
number of Context Switches.  In early testing, we are seeing an unsurprising
correlation between context switches and run time.  It seems likely that each
context switch is polluting the CPU caches and its branch predictor, and maybe
some other things.  The more often this happens during a single run of the timed
command, the worse it performs.

**Ranking:** When there are at least two commands, BestGuess will rank them from
fastest to slowest.  When the number of runs is less than 5, the ranking is
simply a sort by median total CPU time.  With more runs, several statistical
measures are used to produce the ranking.  Sometimes, several commands'
performance will not be distinguishable.  These will be grouped at the top, and
all of them marked as the fastest.  The `-E` (explain) option shows the
statistical measures and highlights the ones that make a command not
distinguishable from the fastest.

## Examples

### Summary to terminal, no raw data output

Here, the empty command provides a good measure of shell startup time.  The 95th
and 99th percentile figures are not available because the number of runs is too
small.  You need at least 20 runs to get 95th and at least 100 runs to get 99th
percentile numbers.  These are of interest because they summarize statistically
what the "long tail" of high run times looks like.

Note that the Mode value may be the most relevant, as it is the "typical"
run time.  However, Median is also a good indication of central tendency for
performance data.  The figures to the right represent the range from minimum (0th
percentile) through median (50th percentile) to the 95th, 99th, and maximum
measurements.

```shell
$ bestguess -r=10 -s "/bin/bash -c" "" "ls -l" "ps Aux"
Use -o <FILE> or --output <FILE> to write raw data to a file.

Command 1: (empty)
                      Mode    ┌     Min      Q₁    Median      Q₃       Max   ┐
   Total CPU time    1.16 ms  │    1.15     1.18     1.48     1.94     2.80   │
        User time    0.43 ms  │    0.43     0.44     0.54     0.71     0.72   │
      System time    0.74 ms  │    0.73     0.74     0.93     1.23     2.08   │
       Wall clock    1.64 ms  │    1.63     1.66     2.06     2.66     5.17   │
          Max RSS    1.67 MB  │    1.67     1.67     1.67     1.67     1.67   │
       Context sw       2 ct  └       2        2        2        2       15   ┘

Command 2: ls -l
                      Mode    ┌     Min      Q₁    Median      Q₃       Max   ┐
   Total CPU time    2.83 ms  │    2.14     2.39     2.72     2.97     4.09   │
        User time    0.91 ms  │    0.79     0.89     0.97     1.08     1.17   │
      System time    1.89 ms  │    1.35     1.49     1.73     1.90     2.94   │
       Wall clock    4.19 ms  │    3.39     3.70     4.19     4.42     7.57   │
          Max RSS    1.77 MB  │    1.59     1.77     1.77     1.84     1.88   │
       Context sw      15 ct  └      15       15       15       15       30   ┘

Command 3: ps Aux
                      Mode    ┌     Min      Q₁    Median      Q₃       Max   ┐
   Total CPU time   35.05 ms  │   32.33    32.60    33.84    34.99    36.20   │
        User time    8.13 ms  │    7.68     7.76     7.93     8.09     8.14   │
      System time   26.92 ms  │   24.65    24.84    25.92    26.86    28.18   │
       Wall clock   36.70 ms  │   34.02    34.25    35.49    36.62    38.02   │
          Max RSS    2.75 MB  │    2.66     2.75     2.77     2.81     3.58   │
       Context sw      14 ct  └      14       14       14       14       18   ┘

Best guess ranking:

  ══════ Command ═══════════════════════════ Total time ═════ Slower by ════════
  ✻   1: (empty)                                1.48 ms
  ══════════════════════════════════════════════════════════════════════════════
      2: ls -l                                  2.72 ms    1.22 ms    82.8% 
      3: ps Aux                                33.84 ms   32.23 ms  2185.1% 
  ══════════════════════════════════════════════════════════════════════════════
$ 
```

If the summary statistics printed in the example above are more than you want to
see, use the "mini stats" option (see below).

### Mini stats

```shell
$ bestguess -M -r 10 -s "/bin/bash -c" "" "ls -l" "ps Aux"
Use -o <FILE> or --output <FILE> to write raw data to a file.

Command 1: (empty)
                      Mode    ┌     Min   Median      Max   ┐
   Total CPU time    1.75 ms  │    1.72     1.97     4.95   │
       Wall clock    2.91 ms  └    2.40     2.75     9.14   ┘

Command 2: ls -l
                      Mode    ┌     Min   Median      Max   ┐
   Total CPU time    2.90 ms  │    2.76     3.03     5.76   │
       Wall clock    4.29 ms  └    4.15     4.57    11.01   ┘

Command 3: ps Aux
                      Mode    ┌     Min   Median      Max   ┐
   Total CPU time   32.28 ms  │   32.28    33.10    36.14   │
       Wall clock   34.02 ms  └   33.87    34.79    37.96   ┘

Best guess ranking:

  ══════ Command ═══════════════════════════ Total time ═════ Slower by ════════
  ✻   1: (empty)                                1.97 ms 
  ══════════════════════════════════════════════════════════════════════════════
      2: ls -l                                  3.03 ms    1.11 ms    56.3% 
      3: ps Aux                                33.10 ms   31.15 ms  1585.3% 
  ══════════════════════════════════════════════════════════════════════════════
$
```

### Raw data output to file

Use `-o <FILE>` to save the raw timing data.  This silences the pedantic
admonition to use this option "to write raw data to a file".

```shell
$ bestguess -o /tmp/data.csv -M -w=2 -r=5 -s "/bin/bash -c" "" "ls -l" "ps Aux"
Command 1: (empty)
                      Mode    ┌     Min   Median      Max   ┐
   Total CPU time    1.04 ms  │    1.02     1.15     1.98   │
       Wall clock    1.51 ms  └    1.49     1.65     2.78   ┘

Command 2: ls -l
                      Mode    ┌     Min   Median      Max   ┐
   Total CPU time    3.71 ms  │    2.59     3.67     3.77   │
       Wall clock    5.44 ms  └    4.05     5.33     5.65   ┘

Command 3: ps Aux
                      Mode    ┌     Min   Median      Max   ┐
   Total CPU time   33.76 ms  │   31.14    33.33    34.57   │
       Wall clock   35.76 ms  └   32.73    35.40    36.45   ┘

Best guess ranking:

  ══════ Command ═══════════════════════════ Total time ═════ Slower by ════════
  ✻   1: (empty)                                1.15 ms 
  ══════════════════════════════════════════════════════════════════════════════
      2: ls -l                                  3.67 ms    2.11 ms   182.7% 
      3: ps Aux                                33.33 ms   32.17 ms  2790.4% 
  ══════════════════════════════════════════════════════════════════════════════
$ 
```

### Summary statistics to file

The BestGuess option `--export-csv <FILE>` writes detailed summary statistics to
`<FILE>`.  Important measurements include: 

  * total time (microseconds)
  * user time (microseconds)
  * system time (microseconds)
  * maximum resident set size (bytes)
  * voluntary context switches (count)
  * involuntary context switches (count)
  * total context switches (count)
  
For each of the measures above, there is a column for:

  * mode
  * minimum (Q0)
  * Q1
  * median (Q2)
  * Q3
  * 95th percentile, provided there were at least 20 runs
  * 99th percentile, provided there were at least 100 runs
  * maximum (Q4)

Note that the BestGuess option `--hyperfine-csv <FILE>` writes summary
statistics to a CSV file in the same format used by Hyperfine, but with more
relevant information.  Note these differences in the file contents:

1. In the second column, Hyperfine reports the mean total CPU time, which is not
   a useful measure.  BestGuess substitutes the estimated mode and the header is
   changed accordingly.
2. The standard deviation figure is replaced by the interquartile range and the
   header is changed accordingly.  IQR is considered a better measure of
   dispersion for non-normal distributions.
3. After the `median` total time column, Hyperfine writes mean values of user
   and system times.  BestGuess reports the `median` user and system times
   instead.  Medians are considered more representative in arbitrary (and
   particularly skewed) distributions.


## Measuring with BestGuess

### Know what you are measuring

As with Hyperfine, you can give BestGuess an ordinary command like `ls -l`.  The
`execvp()` system call can run this command without a shell by searching the
PATH for the executable.  That cost is included in the system time measured.

For example, on my laptop, Hyperfine reports these times comparing `ls` with
`/bin/ls`:

```shell

$ hyperfine --style=basic -w=3 -r=10 -S none "ls -l" "/bin/ls -l"
Benchmark 1: ls -l
  Time (mean ± σ):       7.6 ms ±   0.7 ms    [User: 2.7 ms, System: 2.1 ms]
  Range (min … max):     6.6 ms …   9.4 ms    10 runs
 
Benchmark 2: /bin/ls -l
  Time (mean ± σ):       6.6 ms ±   0.5 ms    [User: 2.7 ms, System: 1.9 ms]
  Range (min … max):     5.8 ms …   7.3 ms    10 runs
 
Summary
  /bin/ls -l ran
    1.14 ± 0.14 times faster than ls -l
$ 
```

As expected, `ls` is slower to execute than `/bin/ls`, presumably due to the
extra cost of finding `ls` in the PATH.

The BestGuess measurements below confirm that indeed `ls` is slower than
`/bin/ls`, though we see a somewhat larger difference between the two commands.
The BestGuess numbers state that `ls` ran 1.44 times faster (2.99/2.07) with
good confidence.  (Use the `-E` option to see an explanation of the measures
used.)

Especially when measuring short duration commands, specifying the full
executable path for all commands will help level the playing field.  (However,
configuring programs to run longer than 1ms would seem to be a good practice.)

Interestingly, the absolute run times reported by BestGuess are smaller than
those reported by Hyperfine:

```shell
$ bestguess -M -w=3 -r=10 "ls -l" "/bin/ls -l"
Command 1: ls -l
                      Mode    ┌     Min   Median      Max   ┐
   Total CPU time    2.98 ms  │    2.49     2.99     4.62   │
       Wall clock    5.00 ms  └    3.83     5.00    12.83   ┘

Command 2: /bin/ls -l
                      Mode    ┌     Min   Median      Max   ┐
   Total CPU time    1.97 ms  │    1.97     2.07     2.77   │
       Wall clock    3.40 ms  └    3.32     3.47     4.94   ┘

Best guess ranking:

  ══════ Command ═══════════════════════════ Total time ═════ Slower by ════════
  ✻   2: /bin/ls -l                             2.07 ms                         
  ══════════════════════════════════════════════════════════════════════════════
      1: ls -l                                  2.99 ms    0.93 ms    44.9%     
  ══════════════════════════════════════════════════════════════════════════════
$ 
```

Neither program is using a shell to run these commands, so there is no shell
startup time to account for.  It is unclear why there is a difference in the
measurements.

In a development build of BestGuess, we called `getrusage()` on RUSAGE_CHILDREN
before spawning the child process and after waiting for its termination -- the
same technique used by Hyperfine.  (Note: We can only get meaningful user and
system times this way, not memory usage or context switch counts.)  We found
that the time measurements match those returned by `wait4()` to the microsecond.

We have accounted (in testing) for the obvious differences between Hyperfine and
BestGuess as follows:
1. How `getrusage()` is used: two calls bracketing `waitpid()` versus no calls
   by using `wait4()`.  We see identical measurements.
2. Optimizations around `/dev/null`:  Many libraries skip the work of formatting
   output when the destination is `/dev/null`.  In our tests, we ensured that
   the commands we measured sent their outputs to files.
3. Shell usage: Unlike the results shown above, where no shell is used, we used
   `-S "/bin/bash -c"` for both Hyperfine and BestGuess in order to get output
   redirection. 

Some obvious remaining differences between BestGuess and Hyperfine need
examining.  First, Hyperfine represents user and system times with floating
point, in units of seconds.  BestGuess follows the best practice of using
integer arithmetic for its internal representations, in this case in units of
microseconds as reported by `getrusage()`.  Second, Hyperfine uses a Rust crate
(library) that wraps the work of spawning a child process.  Perhaps after
calling `fork()` (on Unix), the Rust library does some non-trivial work before
calling `exec()`.  That work would accrue resources to the child process.  See
[these notes](notes/hyperfine.md) for more.


### You can measure the shell startup time

If you are using a shell (`-s <shell>` in BestGuess) to run your commands, then
you might want to separately measure the shell startup time.  You can do this by
specifying an empty command string.  BestGuess in this case is timing `/bin/bash
-c ""`, i.e. launching a shell exactly the same way it does to run each command,
except the command is empty.

On my machine, as shown by the data below, it takes about 1.8ms to launch bash,
only to have it see no command and exit.

Setting aside that this is a small set of measurements (and for a short duration
command), in general we might subtract the bash startup time from the run time of
the other commands to obtain an estimate of the net run time.

```shell
$ bestguess -M -w 3 -r 3 -s "/bin/bash -c" "" "ls -l"
Use -o <FILE> or --output <FILE> to write raw data to a file.

Command 1: (empty)
                      Mode    ┌     Min   Median      Max   ┐
   Total CPU time    2.11 ms  │    1.99     2.09     2.14   │
       Wall clock    2.92 ms  └    2.78     2.92     2.92   ┘

Command 2: ls -l
                      Mode    ┌     Min   Median      Max   ┐
   Total CPU time    2.97 ms  │    2.95     2.99     3.59   │
       Wall clock    4.42 ms  └    4.39     4.46     5.40   ┘

Best guess ranking: (Lacking the 5 timed runs to statistically rank)

  ══════ Command ═══════════════════════════ Total time ═════ Slower by ════════
      1: (empty)                                2.09 ms 
      2: ls -l                                  2.99 ms 
  ══════════════════════════════════════════════════════════════════════════════
$ 
```

## Bar graph

There's a "cheap" but useful bar graph feature in BestGuess (`-G` or `--graph`)
that shows the total time taken for each iteration as a horizontal bar.

The bar is scaled to the maximum time needed for any iteration of command.  The
chart, therefore, is meant to show variation between iterations of the same
command.  Iteration 0 prints first.

The bar graph is meant to provide an easy way to estimate how many warmup runs
may be needed, but can also give some insight about whether performance settles
into a steady state or oscillates.

### Using the graph to assess needed warmups

The contrived example below measures shell startup time against the time to run
`ls` without a shell.  It looks like `bash` would could use a few warmup runs,
and so could `ls`.  Interestingly, the performance of `ls` got worse again after
a few runs.  Longer-running commands and more runs are recommended, of course.
This is an example of how to use BestGuess, not a guideline.

```shell 
$ bestguess -N -G -r 10 /bin/bash ls
Use -o <FILE> or --output <FILE> to write raw data to a file.

Command 1: /bin/bash
0                                                                               max
│▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
│▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
│▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
│▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
│▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
│▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
│▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
│▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
│▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
│▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭

Command 2: ls
0                                                                               max
│▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
│▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
│▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
│▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
│▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
│▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
│▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
│▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
│▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
│▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭

Best guess ranking:

  ══════ Command ═══════════════════════════ Total time ═════ Slower by ════════
  ✻   2: ls                                     1.31 ms                         
  ══════════════════════════════════════════════════════════════════════════════
      1: /bin/bash                              1.98 ms    0.58 ms    44.3%     
  ══════════════════════════════════════════════════════════════════════════════
$ 
```

### Cheap box plots on the terminal

Box plots are a convenient way to get a sense of how two distributions compare.
We found, when using BestGuess (and before that, Hyperfine) that we didn't want
to wait to do statistical analysis of our raw data using a separate program.  To
get a sense of what the data looked like as we collected it, I implemented a box
plot feature.

The edges of the box are the interquartile range, and the median is shown inside
the box.  The whiskers reach out to the minimum and maximum values.

In the example below, although `bash` (launching the shell with no command to
run) appears faster than `ls`, we can see that their distributions overlap
considerably.  The BestGuess ranking analysis concludes that these two commands,
at least in this particular experiment, did not perform statically differently.

```shell 

$ bestguess -N -B -r 100 /bin/bash ls
Use -o <FILE> or --output <FILE> to write raw data to a file.

 1.0       1.4       1.8       2.2       2.6       3.0       3.4       3.8 
   ├─────────┼─────────┼─────────┼─────────┼─────────┼─────────┼─────────┼──────
       ┌──┬──────┐
 1:├┄┄┄┤  │      ├┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┤
       └──┴──────┘
             ╓─────┐
 2:        ├┄╢     ├┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┤
             ╙─────┘
   ├─────────┼─────────┼─────────┼─────────┼─────────┼─────────┼─────────┼──────
 1.0       1.4       1.8       2.2       2.6       3.0       3.4       3.8 

Box plot legend:
  1: /bin/bash
  2: ls

Best guess ranking: The top 2 commands performed identically

  ══════ Command ═══════════════════════════ Total time ═════ Slower by ════════
  ✻   1: /bin/bash                              1.14 ms 
  ✻   2: ls                                     1.29 ms    0.21 ms    18.6% 
  ══════════════════════════════════════════════════════════════════════════════
$ 
```

### Explanations of statistical measures

Re-running the box plot example above, we get quite different results, with `ls`
coming out on top.  This is unsurprising for programs that take very little time
to complete.  Unlike before, here we asked for an explanation (`-E`), and we can
see that the reason these two commands were judged indistinguishable was
twofold:

  1. The statistically significant median shift, Δ, was only 0.12 ms, and thus
     less than the configured minimum needed, 500 μs = 0.50 ms.  The minimum
     effect size threshold exists because when the median difference is very
     small, we suspect we could get a different ranking in a new experiment.  In
     fact, that happened between the previous experiment, above, and the one
     whose output is below.
  2. The confidence interval for the median difference is 95.07%, which looks
     good, except that the interval itself is (68, 159) μs.  68 micro-seconds is
     very close to zero.  Experience suggests that we evaluate whether a
     confidence interval contains zero using an epsilon parameter.  Ours
     defaults to 250 μs.  Applying it gives an expanded interval of (-182, 409),
     which includes zero.
	 
Note that the calculated probability of superiority was just about high enough
to become a third reason to consider these commands indistinguishable.

```shell
$ bestguess -N -BE -r 100 /bin/bash ls
Use -o <FILE> or --output <FILE> to write raw data to a file.

 1.0       1.3       1.5       1.8       2.1       2.3       2.6       2.8      
   ├─────────┼─────────┼─────────┼─────────┼─────────┼─────────┼─────────┼──────
             ┌──┬──────┐
 1:├┄┄┄┄┄┄┄┄┄┤  │      ├┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┤
             └──┴──────┘
        ┌──┬───────┐
 2:   ├┄┤  │       ├┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┤
        └──┴───────┘
   ├─────────┼─────────┼─────────┼─────────┼─────────┼─────────┼─────────┼──────
 1.0       1.3       1.5       1.8       2.1       2.3       2.6       2.8      

Box plot legend:
  1: /bin/bash
  2: ls

Best guess inferential statistics:

  ┌────────────────────────────────────────────────────────────────────────────┐
  │  Parameter:                                    Settings: (modify with -x)  │
  │    Minimum effect size (H.L. median shift)       effect   500 μs           │
  │    Significance level, α                         alpha    0.05             │
  │    C.I. ± ε contains zero                        epsilon  250 μs           │
  │    Probability of superiority                    super    0.33             │
  └────────────────────────────────────────────────────────────────────────────┘

Best guess ranking: The top 2 commands performed identically

  ══════ Command ═══════════════════════════ Total time ═════ Slower by ════════
  ✻   2: ls                                     1.29 ms 
                                                                                
  ══════ Command ═══════════════════════════ Total time ═════ Slower by ════════
  ✻   1: /bin/bash                              1.42 ms    0.12 ms     9.0% 
                                                                                
      Timed observations      N = 100 
      Mann-Whitney            W = 8344 
      p-value (adjusted)      p < 0.001  (< 0.001) 
      Hodges-Lehmann          Δ = 0.12 ms              ✗ Effect size < 500 μs 
      Confidence interval     95.07% (68, 159) μs      ✗ CI ± 250 μs contains 0 
      Prob. of superiority    Â = 0.33 
                                                                                
$
```

BestGuess considers the performance of two commands to be indistinguishable if
_any of the following_ occur:

  1. The Mann-Whitney W value has an associated p-value larger than the
     configured alpha parameter.
  2. The Hodges-Lehmann median shift estimate is smaller than the configured
     minimum effect size.
  3. The confidence interval for the median shift, give or take epsilon,
     contains zero.
  4. The probability that a random observation (total CPU time) from a command
     will be smaller (faster) than a random observation from the fastest command
     is high.  This is the probability of superiority.

A discussion of these statistical calculations is outside the scope of this
README.  Importantly, the BestGuess statistics are not a substitute for using
other tools to analyze the raw data.  Numerical calculations are fraught when
implemented in a language like C, and they are provided for convenience, as are
the distribution plots and box plots. 

## Bug reports

Bug reports are welcome!  BestGuess is implemented in C because we need
low-level control over the details of how processes are launched and measured.
We use fork, exec, and wait carefully in order to obtain the best measurements
we can.

But with C, segfaults and errant memory accesses are always a possibility.  We
have attempted to implement a controlled _panic_ when we can detect a violation
of intended behavior.  If you see any kind of bug, including any output labeled
"panic", please let us know.

Open an issue with instructions on how we can reproduce the bug.  

Note: `make debug` builds BestGuess in debug mode, enabling assertions as well
as ASAN/UBSAN checks.  If you are curious about the root cause and want to
supply a patch, a debug build can provide helpful information.

## Contributing

If you are interested in contributing, get in touch!  My [personal
blog](https://jamiejennings.com) shows several ways to reach me.  (Don't use
Twitter/X, though, as I'm no longer there.)


## Authors and acknowledgment

It was me and I acted alone.  I did it because it needed to be done.  If it
works for you, then you're welcome.  And if it's broken, that's on me (let me
know).

Performance benchmarking is fraught.  We know that timing data is not normally
distributed, so why do commonly used tools produce statistics on the flawed
assumption that it is?

Timing how long it takes to perform an operation or run an entire program
produces one number, which is usually measured by summing the user and system
times as reported by the OS.  That one number can vary a lot.  Cache misses,
mispredicted branches, and throttling may affect any program.  When a process is
preemptively suspended by the OS or is waiting on I/O, it does not accrue
run time, but (we conjecture) the likelihood of cache misses and branch
mispredictions, i.e. things that slow execution, increases dramatically because
some other process ran on "our core" of the CPU for a bit.

Continuing to conjecture, it's possible that short-duration program measurements
(and microbenchmarks) will exhibit wide variation because the effects of a
single context switch might be of the same order of magnitude as the minimum
run time recorded.

And for long-duration measurements, we will want to know how often our program
was kicked off the processor so another could run.  The distribution of events
in time interval is typically modeled with a [Poisson
distribution](https://en.wikipedia.org/wiki/Poisson_distribution), and we should
determine whether the underlying assumptions justifying that model hold for our
work.

One hypothesis is that the distribution of performance measurements will be
bi-modal, and will correspond to the sum of two effects: CPU noise (cache misses
and branch mispredictions) and the compounding effects of context switches.

In any case, there is an ideal "best performance" on a given architecture,
during which all loads are from L1 and every branch is perfectly predicted.  The
distribution of run times cannot be normal, due to this lower limit.  It is
likely to be log-normal, as [Lemire
suggests](https://lemire.me/blog/2023/04/06/are-your-memory-bound-benchmarking-timings-normally-distributed/). 


In science, magic spells trouble.  We should avoid tools that seem to magically
know what is going on when they don't.  For example, some [excellent
work](https://arxiv.org/abs/1602.00602) concludes that "widely studied
microbenchmarks often fail to reach a steady state of peak performance".
Although our project is not meant for microbenchmarking, this work is relevant.
If some systems do not reach a steady state of high performance, then how do we
know that we are not measuring exactly such a system?  How do we know that
"warmup runs" should be discarded, or that they were necessary at all?

More science needs to be done on performance benchmarking on modern CPUs and
OSes.  And we need better tools to do it.


## License

MIT open source license.

## Project status

In active development.
