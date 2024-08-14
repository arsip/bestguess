# Best Guess

## Description

The goal, in short, is to improve on Hyperfine (a popular benchmarking tool).

## Build and run

To build in debug/development mode:

```
make
```

To build in release mode:

```
make release
```

To install to default location:

```
make install
```

To install to custom location `/some/path/bin`:

```
make DESTDIR=/some/path install
```

## If you already use Hyperfine

BestGuess does not support nearly all of the options that Hyperfine does.  And
BestGuess is tested only on Unix (macos) and Linux (several distros).

If you use Hyperfine like we used to, it's almost a drop-in replacement.  We
most often used the warmup, runs, shell, and CSV export options.  BestGuess can
produce a Hyperfine-compatible CSV file containing summary statistics, as
follows.

The BestGuess `--hyperfine-csv <FILE>` option is used wherever you would use
`--export-csv <FILE>` with Hyperfine to get essentially the same summary
statistics in CSV form.  Two important differences:
  1. BestGuess does not calculate arithmetic mean.  In the summary file, the
     `mean` column has been replaced by `mode`.  The mode value of total time
     is more representative than mean for skewed distributions.
  2. BestGuess does not calculate standard deviation because we do observe
     normal distributions.  Until we decide on a replacement measure of
     variation, the summary file will contain zero in this field.

These Hyperfine options are effectively the same in BestGuess:
  * `-w`, `--warmup`
  * `-r`, `--runs`
  * `-i`, `--ignore-failure`
  * `--show-output`

Key changes from Hyperfine:

  * Many options are _not_ supported.
    * Some are planned, like _prepare_ and _conclude_ and the ability to
      randomize the environment size.
	* Others, like the ones that automate parameter generations, are not in
      plan.  BestGuess can read commands from a file, and we think it's easier
      to generate a command file with the desired parameters.
  * One option works a little differently.
	* `-S`, `--shell <CMD>` For BestGuess, the default is none.  And when used,
	  it must be the entire shell command, e.g. `/bin/bash -c`.
  * Some options are _new_ because they support unique BestGuess features.
    * `-o`, `--output <FILE>` (save data for individual executions)
    * `-i`, `--input <FILE>` (read commands, or more commands, from a file)
    * `-b`, `--brief` (print a brief instead of full summary to the terminal)
    * `-f`, `--file` (read additional commands from a file)
    * `--groups` (summarize commands from file in groups separated by blank lines)

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
process CSV data, so we prefer it.

**Summary statistics:** BestGuess prints summary statistics to the terminal,
provided the raw data output is not directed there.  Since median values appear
to be more useful in characterizing skewed distributions, we display medians not
means.  Note that there is no reliable measure of standard deviation, although a
geometric coefficient of variation might be a suitable replacement.  Until we
know whether such a calculation is useful, we are not providing it.

**Measurements:** Hyperfine measures user and system time.  BestGuess measures
these plus the maximum RSS size, the number of Page Reclaims and Faults, and the
number of Context Switches.  In early testing, we are seeing an unsurprising
correlation between context switches and runtime.  It seems likely that each
context switch is polluting the CPU caches and its branch predictor, and maybe
some other things.  The more often this happens during a single run of the timed
command, the worse it performs.

## Examples

### Summary to terminal, no raw data output

Here, the empty command provides a good measure of shell startup time.  The 95th
and 99th percentile figures are not available because the number of runs is too
small.  You need at least 20 runs to get 95th and at least 100 runs to get 99th
percentile numbers.  These are of interest because they summarize statistically
what the "long tail" of high runtimes looks like.

Note that the Mode value may be the most relevant, as it is the "typical"
runtime.  The figures to the right represent the range from minimum (0th
percentile) through median (50th percentile) to the 95th, 99th, and maximum
measurements.

```shell

$ bestguess -r=10 -S "/bin/bash -c" "" "ls -l" "ps Aux"
Use -o <FILE> or --output <FILE> to write raw data to a file.
A single dash '-' instead of a file name prints to stdout.

Command 1: (empty)
                   Mode     ┌    Min    Median    95th     99th      Max   ┐
    Total time    1.7 ms    │    1.6      1.7       -        -       2.0   │
     User time    0.5 ms    │    0.5      0.5       -        -       0.5   │
   System time    1.2 ms    │    1.1      1.2       -        -       1.4   │
       Max RSS    1.7 MiB   │    1.7      1.7       -        -       1.7   │
    Context sw      3 ct    └      2        3       -        -         6   ┘

Command 2: ls -l
                   Mode     ┌    Min    Median    95th     99th      Max   ┐
    Total time    2.9 ms    │    2.9      3.0       -        -       3.1   │
     User time    0.9 ms    │    0.9      0.9       -        -       1.0   │
   System time    2.0 ms    │    2.0      2.0       -        -       2.1   │
       Max RSS    1.8 MiB   │    1.8      1.9       -        -       2.0   │
    Context sw     16 ct    └     15       16       -        -        16   ┘

Command 3: ps Aux
                   Mode     ┌    Min    Median    95th     99th      Max   ┐
    Total time   41.5 ms    │   41.4     41.5       -        -      42.4   │
     User time   10.3 ms    │   10.3     10.3       -        -      10.4   │
   System time   31.2 ms    │   31.0     31.2       -        -      31.9   │
       Max RSS    3.0 MiB   │    3.0      3.0       -        -       3.2   │
    Context sw     15 ct    └     15       15       -        -        15   ┘

Best guess is
  (empty) ran
    1.75 times faster than ls -l
   24.61 times faster than ps Aux
$ 
```

### Brief output to terminal

```shell
$ bestguess -b -r=10 -S "/bin/bash -c" "" "ls -l" "ps Aux"
Command 1: (empty)
                   Mode          Min    Median     Max
    Total time    1.7 ms         1.7      1.7      2.0

Command 2: ls -l
                   Mode          Min    Median     Max
    Total time    3.0 ms         2.9      3.0      3.1

Command 3: ps Aux
                   Mode          Min    Median     Max
    Total time   41.3 ms        40.9     41.3     42.8

Best guess is
  (empty) ran
    1.75 times faster than ls -l
   24.33 times faster than ps Aux
$ 
```

### Raw data output to terminal

Run 3 commands without using a shell, with 2 warmup runs and 3 timed runs.  The
first command is blank, and gives a measure of shell startup time.  The
measurements for each individual timed run, in CSV format, are printed to the
terminal because the output filename was given as `-`:

```shell
$ bestguess -o - -w=2 -r=3 -S "/bin/bash -c" "" "ls -l" "ps Aux"
Command,Exit code,Shell,User time (us),System time (us),Total time (us),Max RSS (Bytes),Page Reclaims,Page Faults,Voluntary Context Switches,Involuntary Context Switches,Total Context Switches
"",0,"/bin/bash -c",542,1167,1769472,484,3,0,3
"",0,"/bin/bash -c",499,1147,1769472,486,3,0,3
"",0,"/bin/bash -c",518,1247,1769472,486,3,0,3
"ls -l",0,"/bin/bash -c",968,1990,1933312,655,3,0,16
"ls -l",0,"/bin/bash -c",924,2005,1916928,654,3,0,16
"ls -l",0,"/bin/bash -c",979,2062,1933312,656,3,0,16
"ps Aux",0,"/bin/bash -c",10220,30905,3145728,1364,4,0,15
"ps Aux",0,"/bin/bash -c",10250,31378,3178496,1366,4,0,15
"ps Aux",0,"/bin/bash -c",10319,32026,3178496,1366,4,0,15
$ 
```

### Raw data output to file

Change to using `-o <FILE>` and you'll see summary statistics printed on the
terminal, and the raw timing data saved to the named file:

```shell
$ bestguess -o /tmp/data.csv -w=2 -r=3 -S "/bin/bash -c" "" "ls -l" "ps Aux"
Command 1: (empty)
                   Mode     ┌    Min    Median    95th     99th      Max   ┐
    Total time    1.7 ms    │    1.6      1.7       -        -       1.7   │
     User time    0.5 ms    │    0.5      0.5       -        -       0.5   │
   System time    1.1 ms    │    1.1      1.2       -        -       1.2   │
       Max RSS    1.7 MiB   │    1.7      1.7       -        -       1.7   │
    Context sw      2 ct    └      2        2       -        -         3   ┘

Command 2: ls -l
                   Mode     ┌    Min    Median    95th     99th      Max   ┐
    Total time    2.9 ms    │    2.9      3.0       -        -       3.0   │
     User time    1.0 ms    │    0.9      0.9       -        -       1.0   │
   System time    2.0 ms    │    2.0      2.0       -        -       2.1   │
       Max RSS    1.8 MiB   │    1.8      1.8       -        -       1.8   │
    Context sw     16 ct    └     15       16       -        -        16   ┘

Command 3: ps Aux
                   Mode     ┌    Min    Median    95th     99th      Max   ┐
    Total time   41.7 ms    │   41.7     41.8       -        -      42.1   │
     User time   10.4 ms    │   10.4     10.4       -        -      10.4   │
   System time   31.4 ms    │   31.3     31.4       -        -      31.7   │
       Max RSS    3.0 MiB   │    3.0      3.0       -        -       3.2   │
    Context sw     15 ct    └     14       15       -        -        15   ┘

Best guess is
  (empty) ran
    1.71 times faster than ls -l
   24.22 times faster than ps Aux
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
  * minimum
  * median
  * 95th percentile, provided there were at least 20 runs
  * 99th percentile, provided there were at least 100 runs
  * maximum

Note that the BestGuess option `--hyperfine-csv <FILE>` writes summary statistics to a
CSV file in the same format used by Hyperfine, but with these differences:

1. The `mean` quantity has been replaced by the `mode` for total, user, and
   system times.
2. The standard deviation figure is omitted. 


## Measuring with BestGuess

### Know what you are measuring

As with Hyperfine, you can give BestGuess an ordinary command like `ls -l`.  The
_execvp()_ system call can run this command without a shell by searching the
PATH for the executable.  That cost is included in the system time measured.

For example, on my laptop, Hyperfine reports these times comparing `ls` with
`/bin/ls`:

```shell
$ hyperfine --style=basic -w=3 -r=10 -S none "ls -l" "/bin/ls -l"
Benchmark 1: ls -l
  Time (mean ± σ):       6.2 ms ±   0.8 ms    [User: 2.1 ms, System: 1.7 ms]
  Range (min … max):     5.5 ms …   7.8 ms    10 runs
 
Benchmark 2: /bin/ls -l
  Time (mean ± σ):       5.0 ms ±   0.2 ms    [User: 1.9 ms, System: 1.5 ms]
  Range (min … max):     4.9 ms …   5.5 ms    10 runs
 
Summary
  /bin/ls -l ran
    1.22 ± 0.16 times faster than ls -l
$ 
```

As expected, `ls` is slower to execute, presumably due to the extra cost of
finding `ls` in the PATH.

The BestGuess measurements confirm that indeed `ls` is slower than `/bin/ls`,
though we see a somewhat larger difference between the two commands.

Especially when measuring short duration commands, specifying the full
executable path for all commands will level the playing field.  (However,
configuring programs to run much longer than 1ms would seem to be a good
practice.)

Interestingly, the absolute run times reported by BestGuess are smaller than
those reported by Hyperfine:

```
$ bestguess -w=3 -r=10 "ls -l" "/bin/ls -l"
Use -o <FILE> or --output <FILE> to write raw data to a file.
A single dash '-' instead of a file name prints to stdout.

Command 1: ls -l
                   Mode     ┌    Min    Median    95th     99th      Max   ┐
    Total time    4.6 ms    │    2.7      4.6       -        -       4.7   │
     User time    0.6 ms    │    0.6      0.6       -        -       0.7   │
   System time    4.0 ms    │    2.1      3.9       -        -       4.0   │
       Max RSS    1.8 MiB   │    1.6      1.8       -        -       2.0   │
    Context sw     19 ct    └     19       20       -        -        22   ┘

Command 2: /bin/ls -l
                   Mode     ┌    Min    Median    95th     99th      Max   ┐
    Total time    2.1 ms    │    2.0      2.1       -        -       2.4   │
     User time    0.6 ms    │    0.6      0.6       -        -       0.7   │
   System time    1.5 ms    │    1.4      1.5       -        -       1.7   │
       Max RSS    2.0 MiB   │    1.6      1.8       -        -       2.0   │
    Context sw     15 ct    └     15       15       -        -        15   ┘

Best guess is
  /bin/ls -l ran
    2.20 times faster than ls -l
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

If you are using a shell (`-S <shell>` in BestGuess) to run your commands, then
you might want to separately measure the shell startup time.  You can do this by
specifying an empty command string.  BestGuess in this case is timing `/bin/bash
-c ""`, i.e. launching a shell exactly the same way it does to run each command,
except the command is empty.

On my machine, as shown by the data below, it takes about 1.8ms to launch bash,
only to have it see no command and exit.

Setting aside that this is a small set of measurements (and for a short duration
command), in general we might subtract the bash startup time from the runtime of
the other commands to obtain an estimate of the net runtime.

```shell

$ bestguess -b -w 3 -r 3 -S "/bin/bash -c" "" "ls -l"
Command 1: (empty)
                   Mode          Min    Median     Max
    Total time    1.7 ms         1.7      1.7      1.7

Command 2: ls -l
                   Mode          Min    Median     Max
    Total time    3.0 ms         2.9      3.0      3.0

Best guess is
  (empty) ran
    1.75 times faster than ls -l
$ 
```

## Bar graph

There's a "cheap" but useful bar graph feature in BestGuess (`-g` or `--graph`)
that shows the total time taken for each iteration as a horizontal bar.

The bar is scaled to the maximum time needed for any iteration of command.  The
chart, therefore, is meant to show variation between iterations of the same
command.  Iteration 0 prints first.

In the example below, we see modest variation in the execution times of `ls`,
and we get a sense of how many warmup runs would be useful -- at least one, but
perhaps a few more.

The variation in execution times of `find` is quite small.  And the first
iteration is no slower than the others.

```shell
$ bestguess -g -i -r=10 -S "/bin/bash -c" "ls -lh" "find /usr/local \"*.dylib\""
Use -o <FILE> or --output <FILE> to write raw data to a file.
A single dash '-' instead of a file name prints to stdout.

Command 1: ls -lh
                   Mode     ┌    Min    Median    95th     99th      Max   ┐
    Total time    3.0 ms    │    2.9      3.0       -        -       3.8   │
     User time    0.9 ms    │    0.9      1.0       -        -       1.1   │
   System time    2.0 ms    │    1.9      2.0       -        -       2.8   │
       Max RSS    1.8 MiB   │    1.8      1.8       -        -       2.0   │
    Context sw     16 ct    └     15       16       -        -        37   ┘
0                                                                             max
│▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
│▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
│▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
│▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
│▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
│▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
│▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
│▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
│▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
│▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭

Command 2: find /usr/local "*.dylib"
                   Mode     ┌    Min    Median    95th     99th      Max   ┐
    Total time   2.34 s     │   2.32     2.34       -        -      2.36   │
     User time   0.11 s     │   0.11     0.11       -        -      0.11   │
   System time   2.23 s     │   2.21     2.23       -        -      2.24   │
       Max RSS   3.27 MiB   │   3.22     3.28       -        -      3.47   │
    Context sw  40.44 Kct   └  40.38    40.45       -        -     40.59   ┘
0                                                                             max
│▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
│▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
│▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
│▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
│▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
│▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
│▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
│▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
│▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
│▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭

Best guess is
  ls -lh ran
  782.41 times faster than find /usr/local "*.dylib"
$ 
```


## Contributing

If you are interested in contributing, get in touch!  My [personal
blog](https://jamiejennings.com) shows several ways to reach me.  (Don't use
Twitter, though, as I'm no longer there.)


## Authors and acknowledgment

It was me and I acted alone.  I did it because it needed to be done.  If it
works for you, then you're welcome.  And if it's broken, that's on me as well
(let me know).

Performance benchmarking is fraught.  We know that timing data is not normally
distributed, so why do commonly used tools produce statistics on the flawed
assumption that it is?

Timing how long it takes to perform an operation or run an entire program
produces one number, which is usually measured by summing the user and system
times as reported by the OS.  That one number can vary a lot.  Cache misses,
mispredicted branches, and throttling may affect any program.  When a process is
preemptively suspended by the OS or is waiting on I/O, it does not accrue
runtime, but (we conjecture) the likelihood of cache misses and branch
mispredictions, i.e. things that slow execution, increases dramatically because
some other process ran on "our core" of the CPU for a bit.

Continuing to conjecture, it's possible that short-duration program measurements
(and microbenchmarks) will exhibit wide variation because the effects of a
single context switch might be of the same order of magnitude as the minimum
runtime recorded.

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
distribution of runtimes cannot be normal, due to this lower limit.  It is
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
