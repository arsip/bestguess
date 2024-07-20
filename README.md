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
     `mean` and `median` columns contain the same value, which is the median of
     the total (user + system) time.
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
    * Some, like _prepare_ and _conclude_ are planned.
	* Others, like the ones that automate parameter generations, are not in
      plan.  BestGuess can read commands from a file, and we think it's easier
      to generate a command file with the desired parameters.
  * `-S`, `--shell <CMD>`: Default is none. If used, supply the entire shell
    command, e.g. `/bin/bash -c`.
  * `-o`, `--output <FILE>`	(added by BestGuess)
  * `-i`, `--input <FILE>`	(added by BestGuess)
  * `-b`, `--brief`         (added by BestGuess)
  * `-g`, `--graph`         (added by BestGuess)

**Shell usage:** For BestGuess, the default is to _not_ use a shell at all.  If
you supply a shell, you will need to give the entire command including the `-c`
that most shells require.  Rationale: BestGuess should not supply a missing `-c`
shell argument.  It doesn't know what shell you are using or how to invoke it.

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
find more tools, from command line tools to spreadsheets, are able to process
CSV data and thus prefer it.

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

```shell
$ bestguess -r=10 -S "/bin/bash -c" "" "ls -l" "ps Aux"
Use -o <FILE> or --output <FILE> to write raw data to a file.
A single dash '-' instead of a file name prints to stdout.

Command 1: (empty)
                      Median               Range
  Total time          1.4 ms         1.3 ms  -    2.0 ms 
  User time           0.5 ms         0.7 ms  -    0.5 ms 
  System time         0.8 ms         1.4 ms  -    0.9 ms 
  Max RSS             1.7 MiB        1.7 MiB -    1.7 MiB
  Context switches      2 count        2 cnt -      2 cnt

Command 2: ls -l
                      Median               Range
  Total time          2.5 ms         2.2 ms  -    3.6 ms 
  User time           0.8 ms         1.3 ms  -    0.9 ms 
  System time         1.4 ms         2.3 ms  -    1.5 ms 
  Max RSS             1.7 MiB        2.0 MiB -    1.8 MiB
  Context switches     15 count       15 cnt -     16 cnt

Command 3: ps Aux
                      Median               Range
  Total time         42.9 ms        42.7 ms  -   44.5 ms 
  User time          10.3 ms        10.5 ms  -   10.3 ms 
  System time        32.4 ms        34.0 ms  -   32.6 ms 
  Max RSS             2.7 MiB        3.2 MiB -    3.0 MiB
  Context switches     14 count       14 cnt -     18 cnt

Summary
   ran
    1.79 times faster than ls -l
   30.87 times faster than ps Aux
$ 
```

### Brief output to terminal

```shell
$ bestguess -b -r=10 -S "/bin/bash -c" "" "ls -l" "ps Aux"
Command 1: (empty)
  Median time         2.0 ms         1.7 ms  -    3.2 ms 

Command 2: ls -l
  Median time         3.0 ms         2.5 ms  -    4.0 ms 

Command 3: ps Aux
  Median time        42.7 ms        42.6 ms  -   44.8 ms 

Summary
   ran
    1.48 times faster than ls -l
   21.14 times faster than ps Aux
$ 
```

### Raw data output to terminal

Run 3 commands without using a shell, doing 5 warmup runs and 10 timed runs.
Output, in CSV format, will be printed to the terminal because the output
filename was given as `-`:

```shell
$ bestguess -o - -r=2 -S "/bin/bash -c" "" "ls -l" "ps Aux"
Command,Exit code,Shell,User time (us),System time (us),Max RSS (Bytes),Page Reclaims,Page Faults,Voluntary Context Switches,Involuntary Context Switches
"",0,"/bin/bash -c",703,1420,1769472,228,3,0,2
"",0,"/bin/bash -c",679,1208,1769472,228,3,0,2
"ls -l",0,"/bin/bash -c",1646,2812,1916928,393,3,0,15
"ls -l",0,"/bin/bash -c",1467,2415,1835008,388,3,0,15
"ps Aux",0,"/bin/bash -c",13412,41285,3817472,1143,4,0,14
"ps Aux",0,"/bin/bash -c",10617,33690,3145728,1107,4,0,14
$ 
```

These option names are the same ones used by Hyperfine, and can be shortened to
`-w` and `-r`, respectively.


### Raw data output to file

Change to using `-o <FILE>` and you'll see summary statistics printed on the
terminal, and the raw timing data saved to the named file:

```shell
$ bestguess -o /tmp/data.csv -r=2 -S "/bin/bash -c" "" "ls -l" "ps Aux"
Command 1: (empty)
                      Median               Range
  Total time          2.8 ms         2.5 ms  -    3.2 ms 
  User time           0.9 ms         1.1 ms  -    1.0 ms 
  System time         1.6 ms         2.0 ms  -    1.8 ms 
  Max RSS             1.7 MiB        1.7 MiB -    1.7 MiB
  Context switches      2 count        2 cnt -      2 cnt

Command 2: ls -l
                      Median               Range
  Total time          5.3 ms         4.8 ms  -    5.8 ms 
  User time           1.8 ms         2.1 ms  -    2.0 ms 
  System time         3.0 ms         3.7 ms  -    3.3 ms 
  Max RSS             1.7 MiB        1.8 MiB -    1.7 MiB
  Context switches     15 count       15 cnt -     15 cnt

Command 3: ps Aux
                      Median               Range
  Total time         52.4 ms        44.1 ms  -   60.6 ms 
  User time          10.6 ms        14.9 ms  -   12.8 ms 
  System time        33.5 ms        45.7 ms  -   39.6 ms 
  Max RSS             2.8 MiB        3.5 MiB -    3.1 MiB
  Context switches     14 count       14 cnt -     14 cnt

Summary
   ran
    1.88 times faster than ls -l
   18.58 times faster than ps Aux
$ 
```

Note that the BestGuess option `--hyperfine-csv` writes summary statistics to a
CSV file in the same format used by Hyperfine.


## Measuring with BestGuess

### Know what you are measuring

As with Hyperfine, you can give BestGuess an ordinary command like `ls -l`.  The
_execvp()_ system call can run this command without a shell by searching the
PATH for the executable.  That cost is included in the system time measured.

E.g. `hyperfine -r 3 -S none "ls -l" "/bin/ls -l"` reports:

	Summary
	  /bin/ls -l ran
		1.48 ± 0.43 times faster than ls -l

The output of BestGuess shows a similar performance difference (see below) where
the mean time for `/bin/ls -l` is 1.43 times faster than `ls -l`.

The time difference is around 1ms on my machine.  Especially when measuring
short duration commands, specifying the full executable path for all commands
will level the playing field.  (However, configuring programs to run much longer
than 1ms would seem to be a good practice.)


```shell
$ bestguess -o - -r 3 "ls -l" "/bin/ls -l"
Command,Exit code,Shell,User time (us),System time (us),Max RSS (Bytes),Page Reclaims,Page Faults,Voluntary Context Switches,Involuntary Context Switches
"ls -l",0,,1727,4565,1835008,234,0,0,14
"ls -l",0,,1332,3389,1949696,241,0,0,15
"ls -l",0,,1203,2898,1867776,237,0,0,14
"/bin/ls -l",0,,1037,1723,1916928,240,0,0,14
"/bin/ls -l",0,,988,1706,1818624,233,0,0,14
"/bin/ls -l",0,,838,1443,1818624,233,0,0,14
$ 
```

### You can measure the shell startup time

If you are using a shell (`-S <shell>` in BestGuess) to run your commands, then
you might want to separately measure the shell startup time.  You can do this by
specifying an empty command string.  BestGuess in this case is timing `/bin/bash
-c ""`, i.e. launching a shell exactly the same way it does to run each command,
except the command is empty.

On my machine, as shown by the data below, it takes about 1.8ms to launch bash,
only to have it see no command and exit.

Setting aside that this is a small amount of data for a short duration command,
it might be reasonable to subtract the bash startup time from the runtime of the
other commands (`ls -l` in the example below).

```shell
$ bestguess -o - -r 3 -S "/bin/bash -c" "" "ls -l"
Command,Exit code,Shell,User time (us),System time (us),Max RSS (Bytes),Page Reclaims,Page Faults,Voluntary Context Switches,Involuntary Context Switches
"",0,"/bin/bash -c",1299,2511,1769472,228,3,0,4
"",0,"/bin/bash -c",971,1748,1769472,230,3,0,2
"",0,"/bin/bash -c",866,1587,1769472,230,3,0,2
"ls -l",0,"/bin/bash -c",2034,3582,1949696,397,3,0,15
"ls -l",0,"/bin/bash -c",1805,3050,1835008,390,3,0,15
"ls -l",0,"/bin/bash -c",1608,2772,1835008,393,3,0,15
$ 
```

## Bar graph

There's a "cheap" but useful bar graph feature in BestGuess (`-g` or `--graph`)
that shows the total time taken for each iteration as a horizontal bar.

The bar is scaled to the maximum time needed for any iteration of command.  The
chart, therefore, is meant to show variation between iterations of the same
command.  Iteration 0 prints first.

In the example below, we see lots of variation in execution time of `ps`, and we
get a sense of how many warmup runs would be useful -- at least one, perhaps 3
or even 5.

By contrast, the variation across iterations of `find` was much less.  And only
the first iteration appears worse than the others, though perhaps the difference
is not significant.

```shell
$ bestguess -g -i -r=10 -S "/bin/bash -c" "ps Aux" "find /usr \"*.dylib\"" "ls -l"
Use -o <FILE> or --output <FILE> to write raw data to a file.
A single dash '-' instead of a file name prints to stdout.

Command 1: ps Aux
                      Median               Range
  Total time          2.9 ms         2.1 ms  -    3.9 ms 
  User time           0.0 ms         2.6 ms  -    1.6 ms 
  System time         0.0 ms         3.2 ms  -    1.1 ms 
  Max RSS             0.0 MiB        0.0 MiB -    0.0 MiB
  Context switches      1 count        1 cnt -      2 cnt
0                                                                           max
|▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
|▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
|▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
|▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
|▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
|▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
|▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
|▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
|▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
|▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭

Command 2: find /usr "*.dylib"
                      Median               Range
  Total time        120.8 ms       119.3 ms  -  124.7 ms 
  User time          25.6 ms        40.5 ms  -   35.3 ms 
  System time        80.0 ms        94.7 ms  -   86.3 ms 
  Max RSS             0.0 MiB        0.0 MiB -    0.0 MiB
  Context switches     30 count       29 cnt -     32 cnt
0                                                                           max
|▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
|▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
|▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
|▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
|▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
|▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
|▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
|▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
|▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
|▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭

Command 3: ls -l
                      Median               Range
  Total time          1.1 ms         1.0 ms  -    1.4 ms 
  User time           0.0 ms         1.2 ms  -    0.0 ms 
  System time         0.0 ms         1.4 ms  -    1.1 ms 
  Max RSS             0.0 MiB        0.0 MiB -    0.0 MiB
  Context switches      1 count        1 cnt -      1 cnt
0                                                                           max
|time exceeds plot size: 1375 us
|▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
|▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
|▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
|▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
|▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
|▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
|▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
|▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
|▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭

Summary
  ls -l ran
    2.58 times faster than ps Aux
  108.07 times faster than find /usr "*.dylib"
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
