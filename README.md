# BestGuess

BestGuess is a tool for command-line benchmarking, a type sometimes called
"macro-benchmarking" because entire programs are measured.

BestGuess does these things:

1. Runs commands and captures run times, memory usage, and more data about their execution.
2. Saves the raw data, for record-keeping or for later analysis.
3. Optionally reports on various properties of the data distribution.
4. Ranks the benchmarked commands from fastest to slowest.

The default output contains a lot of information about the commands being
benchmarked:

``` 
$ bestguess -r 20 "ls -lR" "ls -l" "ps Aux"
Use -o <FILE> or --output <FILE> to write raw data to a file.

Command 1: ls -lR
                      Mode    ╭     Min      Q₁    Median      Q₃       Max   ╮
   Total CPU time    5.86 ms  │    5.79     5.86     5.92     6.26     9.77   │
        User time    2.18 ms  │    2.14     2.17     2.18     2.29     3.03   │
      System time    3.68 ms  │    3.65     3.69     3.75     3.97     6.73   │
       Wall clock    7.17 ms  │    7.14     7.22     7.34     7.65    12.55   │
          Max RSS    1.86 MB  │    1.64     1.83     1.86     1.88     2.06   │
       Context sw      13 ct  ╰      13       13       13       14       18   ╯

Command 2: ls -l
                      Mode    ╭     Min      Q₁    Median      Q₃       Max   ╮
   Total CPU time    2.63 ms  │    2.61     2.64     2.71     2.77     3.44   │
        User time    1.05 ms  │    1.00     1.01     1.03     1.05     1.19   │
      System time    1.62 ms  │    1.59     1.62     1.67     1.72     2.25   │
       Wall clock    3.87 ms  │    3.82     3.90     4.09     4.34     5.14   │
          Max RSS    1.73 MB  │    1.55     1.73     1.77     1.80     1.98   │
       Context sw      13 ct  ╰      13       13       13       14       14   ╯

Command 3: ps Aux
                      Mode    ╭     Min      Q₁    Median      Q₃       Max   ╮
   Total CPU time   34.10 ms  │   33.45    33.90    34.10    34.26    34.37   │
        User time    8.52 ms  │    8.48     8.51     8.53     8.57     8.63   │
      System time   25.62 ms  │   24.91    25.38    25.55    25.65    25.88   │
       Wall clock   45.18 ms  │   44.28    44.82    45.05    45.31    45.56   │
          Max RSS    2.94 MB  │    2.88     2.94     2.96     3.03     3.23   │
       Context sw      98 ct  ╰      96       98       98       98       99   ╯

Best guess ranking:

  ══════ Command ═══════════════════════════ Total time ═════ Slower by ════════
  ✻   2: ls -l                                  2.71 ms 
  ══════════════════════════════════════════════════════════════════════════════
      1: ls -lR                                 5.92 ms    3.22 ms   2.19x 
      3: ps Aux                                34.10 ms   31.39 ms  12.60x 
  ══════════════════════════════════════════════════════════════════════════════
$ 
```

For a mini-report, use the `-M` option.  See [reporting options](#reporting-options) 
below.


## Installing

The implementation is C99 and runs on many Linux and BSD platforms, including macOS.

Dependencies: 
  * C compiler
  * `make`

Clone the repository, and in the top level directory, run `make`.  BestGuess is
now executable as `./bestguess`.  To re-analyze saved data, use `./bestreport`.

You can optionally install BestGuess system-wide.  The default installation
location is `/usr/local`.  Use `make install` to install `bestguess` and
`bestreport` there.

To install to custom location, e.g. `/some/path/bin`, run `make`
`DESTDIR=/some/path install`.  Note that the Makefile appends `bin`
automatically. 


## Examples

### Compare two ways of running ps

The default report shows CPU times (user, system, and total), wall clock time,
max RSS (memory), and the number of context switches.

The mode may be the most relevant, as it is the "typical" value, but the median
is also a good indication of central tendency for performance data.

The figures to the right show the conventional quartile figures, from the
minimum up to the maximum observation.  The starred command at the top of the
ranking ran the fastest, statistically.

```
$ bestguess -r=100 "ps" "ps Aux"
Use -o <FILE> or --output <FILE> to write raw data to a file.

Command 1: ps
                      Mode    ╭     Min      Q₁    Median      Q₃       Max   ╮
   Total CPU time    7.83 ms  │    7.76     8.69    11.18    14.68    27.40   │
        User time    1.38 ms  │    1.36     1.53     1.86     2.33     3.96   │
      System time    6.44 ms  │    6.39     7.16     9.38    12.21    23.49   │
       Wall clock    8.38 ms  │    8.35     9.64    13.22    17.49    45.47   │
          Max RSS    1.86 MB  │    1.73     1.86     2.08     2.38     2.77   │
       Context sw       1 ct  ╰       1        2       29       85      572   ╯

Command 2: ps Aux
                      Mode    ╭     Min      Q₁    Median      Q₃       Max   ╮
   Total CPU time   37.04 ms  │   36.57    36.98    37.45    38.82    86.70   │
        User time    9.37 ms  │    9.33     9.42     9.56     9.77    19.74   │
      System time   27.55 ms  │   27.15    27.57    27.88    29.15    66.97   │
       Wall clock   50.81 ms  │   49.44    50.81    51.30    52.39   115.32   │
          Max RSS    3.00 MB  │    2.97     3.08     3.62     3.95     5.33   │
       Context sw    0.12 K   ╰    0.12     0.12     0.13     0.15     1.07   ╯

Best guess ranking:

  ══════ Command ═══════════════════════════ Total time ═════ Slower by ════════
  ✻   1: ps                                    11.18 ms 
  ══════════════════════════════════════════════════════════════════════════════
      2: ps Aux                                37.45 ms   26.28 ms   3.35x 
  ══════════════════════════════════════════════════════════════════════════════
$ 
```

### Best practice: Save the raw data

Use `-o <FILE>` to save the raw data.  This silences the pedantic admonition to
use this option.  We can see in the example below that the raw data file has 76
lines: one header and 25 observations for each of 3 commands.  The first command
is empty, and is used to measure the shell startup time.

```
$ bestguess -o /tmp/data.csv -M -r=25 -s "/bin/bash -c" "" "ls -l" "ps Aux"
Command 1: (empty)
                      Mode    ╭     Min   Median      Max   ╮
   Total CPU time    8.61 ms  │    4.91     8.59    10.41   │
       Wall clock   12.58 ms  ╰    6.74    14.99    44.84   ╯

Command 2: ls -l
                      Mode    ╭     Min   Median      Max   ╮
   Total CPU time   16.96 ms  │    5.66    16.87    21.22   │
       Wall clock   30.26 ms  ╰    7.33    24.56    42.76   ╯

Command 3: ps Aux
                      Mode    ╭     Min   Median      Max   ╮
   Total CPU time   34.42 ms  │   32.02    35.31    45.86   │
       Wall clock   43.78 ms  ╰   42.29    46.54    58.38   ╯

Best guess ranking:

  ══════ Command ═══════════════════════════ Total time ═════ Slower by ════════
  ✻   1: (empty)                                8.59 ms 
  ══════════════════════════════════════════════════════════════════════════════
      2: ls -l                                 16.87 ms    8.28 ms   1.96x 
      3: ps Aux                                35.31 ms   26.72 ms   4.11x 
  ══════════════════════════════════════════════════════════════════════════════
$ wc -l /tmp/data.csv
      76 /tmp/data.csv
$ 
```

The accompanying program `bestreport` can read the raw data file (or many of
them) and reproduce any and all of the summary statistics and graphs:

``` 
$ bestreport -NB /tmp/data.csv
   5        10        16        21        27        32        37        43      
   ├─────────┼─────────┼─────────┼─────────┼─────────┼─────────┼─────────┼──────
        ┌─┬┐
 1:├┄┄┄┄┤ │├┄┤
        └─┴┘
                 ┌───────┬────┐
 2: ├┄┄┄┄┄┄┄┄┄┄┄┄┤       │    ├┄┄┤
                 └───────┴────┘
                                                         ┌─┬────────┐
 3:                                                  ├┄┄┄┤ │        ├┄┄┄┄┄┄┄┄┄┄┤
                                                         └─┴────────┘
   ├─────────┼─────────┼─────────┼─────────┼─────────┼─────────┼─────────┼──────
   5        10        16        21        27        32        37        43      

Box plot legend:
  1: (empty)
  2: ls -l
  3: ps Aux

Best guess ranking:

  ══════ Command ═══════════════════════════ Total time ═════ Slower by ════════
  ✻   1: (empty)                                8.59 ms 
  ══════════════════════════════════════════════════════════════════════════════
      2: ls -l                                 16.87 ms    8.28 ms   1.96x 
      3: ps Aux                                35.31 ms   26.72 ms   4.11x 
  ══════════════════════════════════════════════════════════════════════════════
$ 
```

### Save summary statistics

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

The BestGuess option `--hyperfine-csv <FILE>` writes summary statistics to a CSV
file in the same format used by Hyperfine, but with these key changes:

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

```
$ bestguess --export-csv /tmp/summary.csv -N -r=20 "ls -l" "ps Aux"
Use -o <FILE> or --output <FILE> to write raw data to a file.

Best guess ranking:

  ══════ Command ═══════════════════════════ Total time ═════ Slower by ════════
  ✻   1: ls -l                                  8.66 ms 
  ══════════════════════════════════════════════════════════════════════════════
      2: ps Aux                                42.58 ms   33.92 ms   4.92x 
  ══════════════════════════════════════════════════════════════════════════════
$
$ head -1 /tmp/summary.csv | cut -d, -f 1-6
Command,Shell,Runs (ct),Failed (ct),Total mode (μs),Total min (μs)
$ 
$ wc -l /tmp/summary.csv
       3 /tmp/summary.csv
$ 
```

In the example above, we see that the header of the summary statistics file
shows columns for the command that ran, the shell used (if any), how many runs
were executed, how many of those failed (exited with non-zero status), and more.
There are 3 lines in the file, one for the header and one for each of the two
commands. 


### Measure shell startup time

When running a command via a shell or other command runner, you may want to
measure the overhead of starting the shell.  Supplying an empty command string,
`""`, as one of the commands will run the shell with no command, thus measuring
the time it takes to launch the shell.

**Rationale\:** BestGuess does not compute shell startup time because it doesn't
know whether you want it measured or how.  (How many runs and warmups, for
instance.)  The worst thing that a tool like BestGuess could do is to estimate
shell startup time and subtract it from the measurements it reports.  Hidden
measurements and silent data manipulation are bad science.  If you want to
measure something, do so explicitly.

On my machine, as shown below, about 2.4ms is spent in the shell, out of the
5.2ms needed to run `ls -l`.

When reporting experimental results, we might want to subtract the bash startup
time from the run time of the other commands to estimate the net run time.

```
$ bestguess -M -w 5 -r 20 -s "/bin/bash -c" "" "ls -l"
Use -o <FILE> or --output <FILE> to write raw data to a file.

Command 1: (empty)
                      Mode    ╭     Min   Median      Max   ╮
   Total CPU time    2.39 ms  │    2.37     2.39     2.57   │
       Wall clock    3.15 ms  ╰    3.07     3.15     3.39   ╯

Command 2: ls -l
                      Mode    ╭     Min   Median      Max   ╮
   Total CPU time    5.21 ms  │    5.15     5.22     5.38   │
       Wall clock    6.75 ms  ╰    6.66     6.77     7.04   ╯

Best guess ranking:

  ══════ Command ═══════════════════════════ Total time ═════ Slower by ════════
  ✻   1: (empty)                                2.39 ms 
  ══════════════════════════════════════════════════════════════════════════════
      2: ls -l                                  5.22 ms    2.83 ms   2.18x 
  ══════════════════════════════════════════════════════════════════════════════
$ 
```

## About measurement quality

There is no good definition of accuracy for benchmarking tools.  Runtimes vary
for many reasons, ranging from low-level architectural effects (like the
behavior of instruction caches and branch prediction) to the high level (and
easily observed) makeup of other running processes.  A high system load and lots
of i/o are both known to interfere with the subject program, extending runtimes.

Putting aside accuracy, it is not clear how to explain that different tools
produce sometimes markedly different results for the same experiment run on the
same machine under the same conditions.

Here, BestGuess reports 2.7ms, while Hyperfine reports 9.3ms (the sum of user
and system times).  Neither is using a shell, both send command output to
`/dev/null`, and both are running 5 warmup runs.  It is not clear why the
difference is so large.

BestGuess uses times obtained via `wait4()`.  In other words, it has direct
access to the process accounting done by the OS.  And BestGuess does no extra
work after forking a new process to run each command, before executing the
command.  Hyperfine, which uses a Rust process management crate, may do
significant work in that gap between `fork` and `exec` -- work that will accrue
time to the measured command.

```
$ bestguess -M -w 5 -r 100 "/bin/ls -l"
Use -o <FILE> or --output <FILE> to write raw data to a file.

Command 1: /bin/ls -l
                      Mode    ╭     Min   Median      Max   ╮
   Total CPU time    2.72 ms  │    2.66     2.72     2.94   │
       Wall clock    3.57 ms  ╰    3.53     3.61     4.13   ╯

$ hyperfine --style basic -S "none" --output /dev/null -w 5 -r 100 "/bin/ls -l"
Benchmark 1: /bin/ls -l
  Time (mean ± σ):      11.6 ms ±   0.1 ms    [User: 6.0 ms, System: 3.3 ms]
  Range (min … max):    11.1 ms …  12.1 ms    100 runs
 
$ multitime -s 0 -n 100 /bin/ls -l >/dev/null
===> multitime results
1: /bin/ls -l
            Mean        Std.Dev.    Min         Median      Max
real        0.005       0.001       0.004       0.004       0.009 
user        0.001       0.000       0.001       0.001       0.002 
sys         0.002       0.000       0.002       0.002       0.004 
$ 
```

The multitime utility reports a median of 0.003s (3ms) as the sum of user and
system times for the same experiment.  Multitime uses `wait4()`, like BestGuess,
and reports a comparable result.

A rigorous comparison of these and other benchmarking tools is in order,
accompanied by a deep dive into why they differ.  This example is _not_ that,
though the results below are easy to replicate, and we have done so dozens of
times.


## Reporting options

### Mini stats

If the summary statistics included in the default report are more than you want
to see, use the "mini stats" option.

```
$ bestguess -M -r 20 "ps A" "ps Aux" "ps"
Use -o <FILE> or --output <FILE> to write raw data to a file.

Command 1: ps A
                      Mode    ╭     Min   Median      Max   ╮
   Total CPU time   23.20 ms  │   22.72    23.94    30.54   │
       Wall clock   23.98 ms  ╰   23.43    24.83    33.16   ╯

Command 2: ps Aux
                      Mode    ╭     Min   Median      Max   ╮
   Total CPU time   26.50 ms  │   26.39    27.08    29.69   │
       Wall clock   36.27 ms  ╰   35.72    36.45    41.68   ╯

Command 3: ps
                      Mode    ╭     Min   Median      Max   ╮
   Total CPU time    7.81 ms  │    7.79     7.85     8.88   │
       Wall clock    8.42 ms  ╰    8.38     8.46     9.60   ╯

Best guess ranking:

  ══════ Command ═══════════════════════════ Total time ═════ Slower by ════════
  ✻   3: ps                                     7.85 ms 
  ══════════════════════════════════════════════════════════════════════════════
      1: ps A                                  23.94 ms   16.09 ms   3.05x 
      2: ps Aux                                27.08 ms   19.24 ms   3.45x 
  ══════════════════════════════════════════════════════════════════════════════
$
```

### Bar graphs and box plots

There's a cheap (limited) but useful bar graph feature in BestGuess (`-G` or
`--graph`) that shows the total time taken for each iteration as a horizontal
bar.

The bar is scaled to the maximum time needed for any iteration of command.  The
chart, therefore, is meant to show variation between iterations of the same
command.  Iteration 0 prints first.

The bar graph is meant to provide an easy way to estimate how many warmup runs
may be needed, but can also give some insight about whether performance settles
into a steady state or oscillates.

#### Use the graph to assess the performance pattern

The contrived example below measures shell startup time against the time to run
`ls` without a shell.  It looks like `bash` would could use a few warmup runs.
Interestingly, the performance of `ls` got better and then worse again.
Longer-running commands and more runs are recommended, of course.

```
$ bestguess -NG -r 10 /bin/bash ls
Use -o <FILE> or --output <FILE> to write raw data to a file.

Command 1: /bin/bash
0                                                                               max
│▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
│▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
│▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
│▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
│▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
│▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
│▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
│▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
│▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
│▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭

Command 2: ls
0                                                                               max
│▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
│▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
│▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
│▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
│▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
│▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
│▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
│▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
│▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
│▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭

Best guess ranking: The top 2 commands performed identically

  ══════ Command ═══════════════════════════ Total time ═════ Slower by ════════
  ✻   2: ls                                     2.48 ms 
  ✻   1: /bin/bash                              3.15 ms    0.67 ms   1.27x 
  ══════════════════════════════════════════════════════════════════════════════
$ 
```

#### Cheap box plots on the terminal

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
at least in this particular experiment, did not perform statistically
differently.

```
$ bestguess -NB -r 100 /bin/bash ls
Use -o <FILE> or --output <FILE> to write raw data to a file.

 2.0       2.1       2.3       2.4       2.5       2.7       2.8       2.9      
   ├─────────┼─────────┼─────────┼─────────┼─────────┼─────────┼─────────┼──────
                  ┌┬─┐
 1:           ├┄┄┄┤│ ├┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┤
                  └┴─┘
    ┌┬┐
 2:├┤│├┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┤
    └┴┘
   ├─────────┼─────────┼─────────┼─────────┼─────────┼─────────┼─────────┼──────
 2.0       2.1       2.3       2.4       2.5       2.7       2.8       2.9      

Box plot legend:
  1: /bin/bash
  2: ls

Best guess ranking: The top 2 commands performed identically

  ══════ Command ═══════════════════════════ Total time ═════ Slower by ════════
  ✻   2: ls                                     2.16 ms 
  ✻   1: /bin/bash                              2.38 ms    0.23 ms   1.10x 
  ══════════════════════════════════════════════════════════════════════════════
$ 
```

## Explanations of statistical reports

The statistical reports provided by BestGuess are:
  * Explanation of rankings, `-E`
  * Distribution analysis (normality), `-D`
  * Tail shape, `-T`

**Caveat emptor!** The BestGuess statistics are not a substitute for using
proper statistical tools to analyze the raw data.  We believe that our
calculations are accurate, but numerical calculations are fraught, especially in
hand-rolled code.  (We implemented the calculations ourselves to avoid
dependencies.)  Statistics are calculated by BestGuess for convenience, like the
runtime graphs and box plots.  Having these features built-in facilitates
experimentation by shortening the experiment-export-analyze cycle.

### Ranking

Re-running the box plot example above, we get different results, with `ls`
coming out on top.  This is unsurprising for programs that take very little time
to complete.  Unlike before, here we asked for an explanation (`-E`), and we can
see why, in this experiment, these two commands performed differently:

  1. The p-value from a Mann-Whitney test is lower than the configured threshold
     of α = 0.05, indicating the significance of the median shift estimate.
  1. The Hodges-Lehmann median shift, Δ, was 1.62 ms, which is larger than the
     configured minimum of 500 μs (0.50 ms).  The minimum effect size threshold
     exists because when the median difference is very small, it is easy to get
     a different ranking in a second experiment.
  2. The confidence interval for the median difference is 95.00% and the
     corresponding interval is (1.49ms, 1.87ms).  Experience suggests that we
     should test whether a confidence interval contains zero using an ε
     parameter, and ours defaults to 250 μs.  The interval (1.49ms, 1.87ms) does
     not include zero even when each endpoint is considered ± ε.
  3. The probability of superiority reveals that a randomly chosen observation
     from the (slower) `bash` runtimes will be faster than a randomly chosen
     `ls` runtime with probability 0.03, or 3%.  This is far below the
     configured threshold of 33%, suggesting that these two distributions
     overlap only to a limited degree.

```
$ bestguess -N -BE -r 100 /bin/bash ls
Use -o <FILE> or --output <FILE> to write raw data to a file.

 2.0       2.9       3.8       4.8       5.7       6.6       7.5       8.4 
   ├─────────┼─────────┼─────────┼─────────┼─────────┼─────────┼─────────┼──────
                     ┌───┬────────────────────┐
 1:    ├┄┄┄┄┄┄┄┄┄┄┄┄┄┤   │                    ├┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┤
                     └───┴────────────────────┘
    ┌─┬─┐
 2:├┤ │ ├┄┄┄┄┄┄┄┄┄┄┤
    └─┴─┘
   ├─────────┼─────────┼─────────┼─────────┼─────────┼─────────┼─────────┼──────
 2.0       2.9       3.8       4.8       5.7       6.6       7.5       8.4 

Box plot legend:
  1: /bin/bash
  2: ls

Best guess inferential statistics:

  ╭────────────────────────────────────────────────────────────────────────────╮
  │  Parameter:                                    Settings: (modify with -x)  │
  │    Minimum effect size (H.L. median shift)       effect   500 μs           │
  │    Significance level, α                         alpha    0.05             │
  │    C.I. ± ε contains zero                        epsilon  250 μs           │
  │    Probability of superiority                    super    0.33             │
  ╰────────────────────────────────────────────────────────────────────────────╯

Best guess ranking:

  ══════ Command ═══════════════════════════ Total time ═════ Slower by ════════
  ✻   2: ls                                     2.39 ms 
 
  ══════ Command ═══════════════════════════ Total time ═════ Slower by ════════
      1: /bin/bash                              4.04 ms    1.65 ms   1.69x 
 
      Timed observations      N = 100 
      Mann-Whitney            W = 5353 
      p-value (adjusted)      p < 0.001  (< 0.001) 
      Hodges-Lehmann          Δ = 1.62 ms 
      Confidence interval     95.00% (1.49, 1.87) ms 
      Prob. of superiority    Â = 0.03 
  ══════════════════════════════════════════════════════════════════════════════
$
```

Notice that BestGuess reported in this experiment that `ls` ran faster than
`bash` with statistical significance, while in the experiment shown in the
[previous section](#cheap-box-plots-on-the-terminal), the two commands were
indistinguishable.  Several factors are at work here.  

First, there is a limit to how accurately any tool can measure CPU time, and it
is related to the OS scheduling quantum and its process accounting.  (The
quantum could be, e.g. 10ms, 20ms, 60ms, or larger.  Some OS configurations may
set it under 10ms.)

Second, we did not examine what other processes were running during the
experiment.  Perhaps some other activity interfered more with `bash` for some
unknown reason.

Third, we had to run the experiment above more than 10 times to get this result,
so we could illustrate this phenomenon.  Running many experiments, or increasing
the number of runs in a single experiment, may make the above result much more
unlikely.  But it is the nature of sampling that unlikely samples are
occasionally encountered.


## Distribution statistics

Our experience suggests that most of the time, the distribution of total CPU
time is _not normal_.  This may not be your experience, and BestGuess can
illuminate the issue by calculating several descriptive statistics on each
sample.  The "distribution statistics report" contains a summary including the
number of observations, their range, and the interquartile range.  Following
this summary are several statistics that can be used to compare the distribution
shape to a normal distribution.

The "AD normality" figure is the Anderson-Darling test for normality.  Higher
values indicate that the distribution looks more different from normal.  The
p-value (significance) for the calculation is shown.

Skew is a measure of asymmetry.  Normal distributions are symmetric, so a high
skew value suggests a substantial deviation from normal.

Excess kurtosis is a measure of distribution shape.  It can indicate that the
distribution has much "fatter tails" or much "thinner tails" than would a normal
distribution. 

BestGuess does not use these measures in other calculations.  They are presented
to allow the user to see, statistically, how close or far are their empirical
distributions of total CPU time from normal.

**Rationale\:** When distributions are not normal, the mean is not a good
measure of central tendency.  BestGuess does not show means, for this reason.
The median is used instead.  Similarly, the standard deviation is not a useful
measure of spread, so the IQR is used instead.  Lastly, a t-test is not an
appropriate way to compare the performance of two programs when their runtime
distributions are far from normal.  BestGuess uses non-parametric measures,
including the Hodges-Lehmann measure of _distribution shift_ and the
Mann-Whitney rank-sum test to indicate significance of the shift.

```
$ bestreport -M -D test/raw100.csv
Command 1: ls -l
                      Mode    ╭     Min   Median      Max   ╮
   Total CPU time    1.57 ms  │    1.54     1.70     5.32   │
       Wall clock    2.50 ms  ╰    2.44     2.68     8.21   ╯

  ╭────────────────────────────────────────────────────────────────────────────╮
  │                        Total CPU Time Distribution                         │
  │                                                                            │
  │  N (observations)             100 ct                                       │
  │            Median             1.7 ms                                       │
  │             Range       1.5 … 5.3 ms                                       │
  │                               3.8 ms                                       │
  │               IQR       1.6 … 2.1 ms                                       │
  │                               0.5 ms (13.2% of range)                      │
  │                                                                            │
  │      AD normality           10.82 p < 0.001 (signif., α = 0.05) Not normal │
  │              Skew            2.94 Substantial deviation from normal        │
  │   Excess kurtosis            9.83 Substantial deviation from normal        │
  ╰────────────────────────────────────────────────────────────────────────────╯

Command 2: ps Aux
                      Mode    ╭     Min   Median      Max   ╮
   Total CPU time   31.04 ms  │   30.87    31.08    34.23   │
       Wall clock   32.47 ms  ╰   32.28    32.51    35.97   ╯

  ╭────────────────────────────────────────────────────────────────────────────╮
  │                        Total CPU Time Distribution                         │
  │                                                                            │
  │  N (observations)             100 ct                                       │
  │            Median            31.1 ms                                       │
  │             Range     30.9 … 34.2 ms                                       │
  │                               3.4 ms                                       │
  │               IQR     31.0 … 31.2 ms                                       │
  │                               0.2 ms (5.9% of range)                       │
  │                                                                            │
  │      AD normality           15.97 p < 0.001 (signif., α = 0.05) Not normal │
  │              Skew            3.93 Substantial deviation from normal        │
  │   Excess kurtosis           15.94 Substantial deviation from normal        │
  ╰────────────────────────────────────────────────────────────────────────────╯

Best guess ranking:

  ══════ Command ═══════════════════════════ Total time ═════ Slower by ════════
  ✻   1: ls -l                                  1.70 ms 
  ══════════════════════════════════════════════════════════════════════════════
      2: ps Aux                                31.08 ms   29.38 ms  18.34x 
  ══════════════════════════════════════════════════════════════════════════════
$
```

### Tail statistics

When investigating performance issues in a production system, we want to know
whether long latencies occur.  We find out by examining the (right) tail of the
sample distribution.  The "tail statistics report" summarizes statistically what
the (possibly) "long tail" of high run times looks like.

BestGuess provides a (modest) description of the tail by showing the 95th and
99th percentile figures in the context of the quartile figures: minimum (Q₀, 0th
percentile), first quartile (Q₁, 25th percentile), median (Q₂, 50th percentile),
third quartile (Q₃, 75th percentile), and maximum (Q₄, 100th percentile).

You need at least 20 runs to get 95th and at least 100 runs to get 99th
percentile numbers.

```
$ bestreport -NT test/raw100.csv
Command 1: ls -l
  ╭────────────────────────────────────────────────────────────────────────────╮
  │                      Total CPU Time Distribution Tail                      │
  │                                                                            │
  │  Tail shape     Q₀      Q₁      Q₂      Q₃      95      99      Q₄         │
  │        (ms)    1.54    1.59    1.70    2.14    3.67    5.32    5.32        │
  ╰────────────────────────────────────────────────────────────────────────────╯

Command 2: ps Aux
  ╭────────────────────────────────────────────────────────────────────────────╮
  │                      Total CPU Time Distribution Tail                      │
  │                                                                            │
  │  Tail shape     Q₀      Q₁      Q₂      Q₃      95      99      Q₄         │
  │        (ms)   30.87   31.02   31.08   31.20   32.20   34.23   34.23        │
  ╰────────────────────────────────────────────────────────────────────────────╯

Best guess ranking:

  ══════ Command ═══════════════════════════ Total time ═════ Slower by ════════
  ✻   1: ls -l                                  1.70 ms 
  ══════════════════════════════════════════════════════════════════════════════
      2: ps Aux                                31.08 ms   29.38 ms  18.34x 
  ══════════════════════════════════════════════════════════════════════════════
$
```

## BestGuess option summary

An _experiment_ is when BestGuess runs and measures one or more programs.  An
array of options control how BestGuess conducts experiments.  Another set of
options control how BestGuess reports experimental results.

Provided you use the `-o <filename>` option to save the raw data, BestGuess can
be invoked as `bestreport <filename>` to produce any of its reports or graphs.

### Experiment options

Commonly used options include:
  * `-r <N>`, `--runs <N>` (run each program N times; defaults to 1)
  * `-w <K>`, `--warmup <K>` (perform K unmeasured warmup runs; defaults to 0)
  * `-o <FILE>`, `--output <FILE>` (save raw data for individual executions)
  * `-f <FILE>`, `--file <FILE>` (read commands, or _more_ commands, from a file)
  * `-s <CMD>`, `--shell <CMD>` (pass the program and its arguments to CMD to run)
  * `-i`, `--ignore-failure` (ignore non-zero exit codes)

There are also commands, some implemented and some forthcoming, for controlling
the sequence of timed executions, and for interleaving pre- and post-benchmark
commands. 

**Notes\:**

A best practice is to save raw measurement data (which includes CPU times, max
RSS, and context switch counts) using `-o`.  That data can be later re-analyzed,
using `bestreport` or with proper statistical software.

The ability to read commands from a file, via `-f`, is one we use often in my
research group.  We generate command files for BestGuess using small scripts.
This way, we can easily test programs using a range of input parameter values.
Even better, the contents of command files are not processed by the shell (as
are program invocations supplied on the command line), so no awkward escaping of
quotation marks and other syntax is needed there.

If you are used to Hyperfine, note that BestGuess requires a complete shell
command, such as `/bin/bash -c`.  The `-c`, while common, is not universal to
every shell, so BestGuess does not insert it.  Shell is not quite the right
term, as the general concept is "something that launches commands" and can
include utilities like `/usr/bin/env` (which requires `-S` not `-c`).


### Report options

Commonly used options include:
  * `-M`, `--mini-stats` (show only wall clock and total CPU time stats)
  * `-N`, `--no-stats` (show no summary stats; only the ranking is reported)
  * `-G`, `--graph` (show graph of one command's total CPU time)
  * `-B`, `--boxplot` (show rough box plots of total CPU time on terminal)
  * `-E`, `--explain` (explain the ranking by showing inferential statistics)
  * `-D`, `--dist-stats` (describes a distribution's relation to normal)
  * `-T`, `--tail-stats` (describes tail of a distribution)

**Notes\:**

The default report is one showing CPU times (user, system, and their sum), wall
clock time, max RSS (peak memory), and counts of page faults and context
switches.  The default report shows the quartiles of the distribution of each of
these metrics, as well as an estimate of the mode (the most frequent
measurement).

When an experiment is meant to measure warmed-up performance, we specify a
number of warmup runs with `-w`.  There is no universal guidance about how many
warmups are needed.  In fact, performance may oscillate between bad and good,
and so warmup runs may not bring the measured program into a steady state of
good performance.

The graph option of BestGuess outputs a bar graph of limited resolution, indexed
by the program iteration (i.e. by time).  While designing an experiment, the
graph gives immediate feedback as to whether performance reaches a steady state,
and how long it takes to do so.

The box plot option shows a comparison (of limited resolution) of the total CPU
time for each command.  Like the graph option, the box plot is a convenience,
giving limited but immediate information about experiment results.

Research suggests that total CPU time measurements do _not_ typically follow a
normal distribution.  You can see how well your own data fits a normal
distribution using the BestGuess `-D` option, which analyzes the distribution of
CPU times.

The `-E` (explain) option produces a detailed explanation of the performance
rankings.  BestGuess sorts the benchmarked commands by their median CPU time,
and then compares each command to the fastest one.  Performance that is
indistinguishable from the fastest command yields a tie.  BestGuess uses several
measures of similarity:
  * The Hodges-Lehmann (HL) measure of _distribution shift_ is a non-parametric
    estimate of how much the median of one distribution differs from another.
    If the shift is small, it could be due to measurement noise, measurement
    precision, or a property of the environment (e.g. high system load).
  * The Mann-Whitney W statistic, when the shapes of two distributions are
    similar, tells us whether the HL median shift measure is statistically
    different from zero.  BestGuess uses it to provide a confidence level and
    interval for the median shift.
  * The probability of superiority measures the chance that a randomly chosen
    observation (a single runtime) from one distribution is better (faster) than
    a randomly chosen observation from another (from the fastest one, in our
    case). 

The `-T` (tail statistics) option produces a basic report on the rightward tail
of the distribution of CPU times, showing the minimum, median, 95th and 99th
percentiles, and the maximum.


## For Hyperfine users

BestGuess does not support all of the options that Hyperfine does.  And
BestGuess is currently tested only on Unix (macos) and Linux (several distros).

If you use Hyperfine like we used to do, it's nearly a drop-in replacement.  We
most often used the warmup, runs, shell, and CSV export options.  BestGuess can
produce a Hyperfine-compatible CSV file containing summary statistics, as well
as its own format.

The BestGuess `--hyperfine-csv <FILE>` option is used wherever you would use
`--export-csv <FILE>` with Hyperfine to get essentially the same summary
statistics in CSV form.  There are two important differences, due to the fact
that performance distributions are rarely normal.  (BestGuess will analyze the
empirical distributions of total CPU time to indicate when this is the case.)
Without a normal distribution, the mean is not a very useful measure of central
tendency, and the standard deviation is not a good measure of spread.  These
fields in the Hyperfine-compatible summary statistics file are replaced:
  1. In the file produced by `--hyperfine-csv`, the `mean` column has been
     replaced by `mode`.  The median and mode values of total CPU time are more
     representative than mean for skewed and heavy-tailed distributions.
  2. Instead of the standard deviation, the interquartile range is used.

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
  * Many options are _new_ because they support unique BestGuess features.  See
    the [BestGuess options section](#bestguess-option-summary).


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

**Note\:** `make debug` builds BestGuess in debug mode, enabling assertions as
well as ASAN/UBSAN checks.  If you are curious about the root cause and want to
supply a patch, a debug build can provide helpful information.


## Contributing

If you are interested in contributing, get in touch!  My [personal
blog](https://jamiejennings.com) shows several ways to reach me.  (Don't use
Twitter/X, though, as I'm no longer there.)


## Authors and acknowledgments

It was me and I acted alone.  I did it because it needed to be done.  If it
works for you, then you're welcome.  And if it's broken, that's on me (let me
know).


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
  - There are _no outliers_.  A long run time is just as valid a data point as a
    short one.  The proper way to deal with the occasional long runtime
    measurement is _not_ to rerun an experiment until it does not produce any.
    Simply citing the median or mode run time of the actual measurements
    collected will do.


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
and branch mispredictions) and the compounding effects of context switches.  The
latter are due mainly to high workloads forcing the OS to suspend the program we
are measuring.  More suspensions means more opportunities for other programs to
pollute "our" caches and mislead "our" branch predictor, among other effects.

Importantly, there is an ideal "best performance" on a given architecture,
during which all loads are from L1 cache and all branches are correctly
predicted.  The distribution of run times in general cannot be normal, due to
this lower limit.  Under circumstances that favor good performance, we may see a
majority of run times clustered near the ideal value.  And conversely, when
conditions are bad, the benchmarked program may be suspended after every
scheduling quantum (or more often), and may face something like "cold start"
conditions each time it is resumed.

Run time distributions may be log-normal, as [Lemire
suggests](https://lemire.me/blog/2023/04/06/are-your-memory-bound-benchmarking-timings-normally-distributed/).
We are not seeing that, but it is worth investigating.

Tratt and company, in [fascinating work](https://arxiv.org/abs/1602.00602),
concludes that "widely studied microbenchmarks often fail to reach a steady
state of peak performance".  Although BestGuess is not a tool for
microbenchmarking, this work is relevant.  Perhaps some programs we benchmark at
the command line also do not reach a steady state of high performance.  How do
we know that "warmup runs" are necessary at all, and when they are, how many
should we use?

The same applies to the number of runs.  The number to use depends very much on
the program you are benchmarking and what you want to learn from your
experiment. 

It is, for example, highly arbitrary to decide that a command should run for 3
seconds or 10 iterations, whichever comes first.  This is not good experiment
design.

In science, magic spells trouble.  Benchmark automation is needed, but not
"automagic".  We should avoid tools that operate as though they magically know
what the user is trying to achieve with an experiment, and which massage the
data before presenting it.

More science needs to be done on performance benchmarking on modern CPUs and
OSes, which together present a very complex dynamic environment for code.  And
we need better tools to do it.


## License

MIT open source license.

## Project status

In active development.
