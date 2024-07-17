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

  * `-S`, `--shell <CMD>`: Default is none. If used, supply the entire shell
    command, e.g. `/bin/bash -c`.
  * `-o`, `--output <FILE>`	
  * `-i`, `--input <FILE>`	

**Shell usage:** For BestGuess, the default is to _not_ use a shell at all.  If
you supply a shell, you will need to give the entire command including the `-c`
that most shells require.  Rationale: BestGuess should not supply a missing `-c`
shell argument.  It doesn't know what shell you are using or how to invoke it.

**Shell startup time:** If you want to measure shell startup time, supply the
`-S` option and use `""` (empty string) for one of the commands.  This starts
the shell and then runs, as you would expect, no command.  Rationale: Hidden
measurements are bad science.  If you want to measure something, do so
explicitly.

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
$ ./bestguess -r=10 -S "/bin/bash -c" "" "ls -l" "ps Aux"
NOTE: Use '-o <FILE>' or '--output <FILE>' to write raw data to a file.
	A single dash '-' instead of a file name prints to stdout.

Command 1: (empty)
Warming up with 0 runs...
Timing 10 runs...
Summary:                 Range               Median
  User time      0.614  ms -  1.438  ms     0.745  ms
  System time    1.071  ms -  4.114  ms     1.306  ms
  Total time     1.685  ms -  5.552  ms     2.053  ms
  Max RSS        1.703 MiB -  1.859 MiB     1.703 MiB

Command 2: ls -l
Warming up with 0 runs...
Timing 10 runs...
Summary:                 Range               Median
  User time      0.884  ms -  1.556  ms     1.074  ms
  System time    1.445  ms -  2.766  ms     1.691  ms
  Total time     2.329  ms -  4.311  ms     2.765  ms
  Max RSS        1.781 MiB -  2.016 MiB     1.875 MiB

Command 3: ps Aux
Warming up with 0 runs...
Timing 10 runs...
Summary:                 Range               Median
  User time     10.548  ms - 10.964  ms    10.585  ms
  System time   33.794  ms - 35.765  ms    34.069  ms
  Total time    44.392  ms - 46.729  ms    44.645  ms
  Max RSS        3.000 MiB -  3.266 MiB     3.078 MiB

$ 
```

### Raw data output to terminal

Run 3 commands without using a shell, doing 5 warmup runs and 10 timed runs.
Output, in CSV format, will be printed to the terminal because the output
filename was given as `-`:

```shell
$ ./bestguess -o - -r=2 -S "/bin/bash -c" "" "ls -l" "ps Aux"
Command,Exit code,Shell,User time (us),System time (us),Max RSS (Bytes),Page Reclaims,Page Faults,Voluntary Context Switches,Involuntary Context Switches
"",0,"/bin/bash -c",1064,2936,1785856,225,6,4,2,
"",0,"/bin/bash -c",944,1599,1785856,225,6,0,2,
"ls -l",0,"/bin/bash -c",2039,3796,1900544,386,9,1,16,
"ls -l",0,"/bin/bash -c",1703,2810,1949696,395,3,0,15,
"ps Aux",0,"/bin/bash -c",14778,46327,3948544,1180,5,3,15,
"ps Aux",0,"/bin/bash -c",10989,34819,3194880,1133,5,0,16,
$ 
```

These option names are the same ones used by Hyperfine, and can be shortened to
`-w` and `-r`, respectively.


### Raw data output to file

Change to using `-o <FILE>` and you'll see summary statistics printed on the
terminal, and the raw timing data saved to the named file:

```shell
$ ./bestguess -o /tmp/data.csv -r=2 -S "/bin/bash -c" "" "ls -l" "ps Aux"
Command 1: (empty)
  Warming up with 0 runs...
  Timing 2 runs...
  Summary:                 Range               Median
    User time      0.596  ms -  0.831  ms     0.713  ms
    System time    1.065  ms -  2.227  ms     1.646  ms
    Total time     1.661  ms -  3.058  ms     2.359  ms
    Max RSS        1.703 MiB -  1.703 MiB     1.703 MiB

Command 2: ls -l
  Warming up with 0 runs...
  Timing 2 runs...
  Summary:                 Range               Median
    User time      1.315  ms -  1.564  ms     1.439  ms
    System time    2.187  ms -  2.953  ms     2.570  ms
    Total time     3.502  ms -  4.517  ms     4.009  ms
    Max RSS        1.875 MiB -  1.953 MiB     1.914 MiB

Command 3: ps Aux
  Warming up with 0 runs...
  Timing 2 runs...
  Summary:                 Range               Median
    User time     10.831  ms - 12.496  ms    11.663  ms
    System time   34.742  ms - 40.202  ms    37.472  ms
    Total time    45.573  ms - 52.698  ms    49.135  ms
    Max RSS        3.047 MiB -  3.219 MiB     3.133 MiB

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
Command,Exit code,Shell,User time (us),System time (us),Max RSS (KiByte),Page Reclaims,Page Faults,Voluntary Context Switches,Involuntary Context Switches
"ls -l",0,,765,3059,1949696,510,0,0,25
"ls -l",0,,702,1926,2129920,521,0,0,19
"ls -l",0,,681,3906,1933312,509,0,0,27
"/bin/ls -l",0,,772,1808,2146304,521,0,0,22
"/bin/ls -l",0,,844,1874,1851392,503,0,0,20
"/bin/ls -l",0,,743,1659,2080768,517,0,0,18
~/Projects/bestguess<main>$ 
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
Command,Exit code,Shell,User time (us),System time (us),Max RSS (KiByte),Page Reclaims,Page Faults,Voluntary Context Switches,Involuntary Context Switches
"",0,"/bin/bash -c",593,1438,1769472,489,3,0,2
"",0,"/bin/bash -c",551,1220,1769472,490,3,0,3
"",0,"/bin/bash -c",557,1219,1769472,489,3,0,3
"ls -l",0,"/bin/bash -c",1081,2124,1933312,655,3,0,16
"ls -l",0,"/bin/bash -c",1253,2579,2097152,665,3,0,16
"ls -l",0,"/bin/bash -c",1024,2132,1933312,655,3,0,16
```

## Contributing

If you are interested in contributing, get in touch!  My [personal
blog](https://jamiejennings.com) shows several ways to reach me.  (Don't use
Twitter, though, as I'm no longer there.)


## Authors and acknowledgment

It's all mine.  I did it because it needed to be done.  If it's broken, that's
on me.

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
