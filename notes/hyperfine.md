# Comparing measurements made by BestGuess to Hyperfine

## Example: Simple unix commands

Comparing:

* "ps Aux" 
* "find /usr/local/lib \"*.dylib\"" 
* "ls -l"

In the transcript below, we change one argument, and add another so that
BestGuess knows where to save the raw timing data.  BestGuess gets
`--hyperfine-csv bg.csv` whereas Hyperfine gets `--export-csv hf.csv`.  The
entire transcript is shown here, and a discussion of the results follows.  The
files [bg.csv](bg.csv) and [hf.csv](hf.csv) are in this directory.

Note: The transcript below was obtained before BestGuess calculated the mode of
the runtimes.

```shell
$ bestguess --hyperfine-csv bg.csv -o data.csv -i -w=5 -r=30 -S "/bin/bash -c" "ps Aux" "find /usr/local/lib \"*.dylib\"" "ls -l"
Command 1: ps Aux
                      Median               Range
  Total time         41.8 ms        41.5 ms  -   45.4 ms 
  User time          10.2 ms        10.6 ms  -   10.2 ms 
  System time        31.3 ms        34.8 ms  -   31.6 ms 
  Max RSS             2.9 MiB        4.1 MiB -    3.0 MiB
  Context switches     14 count       14 cnt -     65 cnt

Command 2: find /usr/local/lib "*.dylib"
                      Median               Range
  Total time        130.6 ms       125.6 ms  -  131.6 ms 
  User time           7.4 ms         7.6 ms  -    7.5 ms 
  System time       118.0 ms       124.0 ms  -  123.1 ms 
  Max RSS             1.5 MiB        1.6 MiB -    1.5 MiB
  Context switches      3 count        3 cnt -      4 cnt

Command 3: ls -l
                      Median               Range
  Total time          2.0 ms         2.0 ms  -    2.7 ms 
  User time           0.7 ms         1.0 ms  -    0.7 ms 
  System time         1.2 ms         1.7 ms  -    1.3 ms 
  Max RSS             1.6 MiB        2.0 MiB -    1.8 MiB
  Context switches     15 count       15 cnt -     16 cnt

Summary
  ls -l ran
   20.73 times faster than ps Aux
   64.76 times faster than find /usr/local/lib "*.dylib"
$ 
$ hyperfine --export-csv hf.csv -i -w=5 -r=30 -S "/bin/bash -c" "ps Aux" "find /usr/local/lib \"*.dylib\"" "ls -l"
Benchmark 1: ps Aux
  Time (mean ± σ):      63.4 ms ±   0.7 ms    [User: 22.6 ms, System: 38.4 ms]
  Range (min … max):    62.4 ms …  65.2 ms    30 runs
 
Benchmark 2: find /usr/local/lib "*.dylib"
  Time (mean ± σ):     146.8 ms ±   2.8 ms    [User: 18.5 ms, System: 125.3 ms]
  Range (min … max):   144.4 ms … 158.7 ms    30 runs
 
  Warning: Ignoring non-zero exit code.
  Warning: Statistical outliers were detected. Consider re-running this benchmark on a quiet system without any interferences from other programs. It might help to use the '--warmup' or '--prepare' options.
 
Benchmark 3: ls -l
  Time (mean ± σ):       6.3 ms ±   0.6 ms    [User: 2.5 ms, System: 2.0 ms]
  Range (min … max):     5.1 ms …   7.7 ms    30 runs
 
Summary
  ls -l ran
   10.02 ± 0.95 times faster than ps Aux
   23.20 ± 2.23 times faster than find /usr/local/lib "*.dylib"
$
```

## Observations

First, we don't know the _actual_ runtimes because Hyperfine automatically
estimates the overhead of using a shell and subtracts it from the full runtime:

https://github.com/sharkdp/hyperfine/blob/master/src/benchmark/executor.rs#L27-L31

    /// Perform a calibration of this executor. For example,
    /// when running commands through a shell, we need to
    /// measure the shell spawning time separately in order
    /// to subtract it from the full runtime later.
    fn calibrate(&mut self) -> Result<()>;

Setting that aside, let's look at the data in the Hyperfine summary file and the
BestGuess summary file (in Hyperfine format).  Here we state some observations,
which we discuss in the next section.

_The BestGuess median times are lower than the Hyperfine median times._

A possible explanation is that BestGuess and Hyperfine use different techniques
to compute the runtimes.  The Hyperfine measurement includes work done by
Hyperfine itself, which can only increase the runtimes.
  
_The Hyperfine mean times are sometimes lower than the median, and sometimes
higher._

When using Hyperfine in our work, we have not observed a mean less
than the median value, so it's possible that the very short runtime of `ls -l`
(around 2ms including shell startup) helped produce that uncommon result.  When
we look at our distributions, we generally see two peaks and a long rightward
tail, as in [these](pexl_email.html) [histograms](rust_email.html).
  
_Basic measures of variance do not apply._

Applying the Anderson-Darling normality test to sets of measured runtimes has
shown (in all of our experience doing so) that runtimes are far from normally
distributed.  Indeed, as mentioned above, we can see this clearly when looking
at frequency plots (histograms) of the data, where there are often two distinct
peaks and almost universally a long rightward tail of high duration
measurements.  The formula for standard deviation of a normal distribution is
not useful here, as it has no meaning.

We note that Hyperfine calculates the median absolute deviation (MAD) in order
to print a warning about outlier measurements.  While MAD is a robust measure
(considered insensitive to distribution), we have two concerns.

The first concern is that the [actual
calculation](https://github.com/sharkdp/hyperfine/blob/master/src/outlier_detection.rs#L10-L13)
scales the MAD figure so that it may be used to estimate the standard deviation
of a normal distribution.  Emphatically we repeat that performance measurements
do not typically follow a normal distribution.  Thus the MAD-based outlier
warning is at best misleading and at worst not a useful measure of anything.

The second concern is more fundamental.  What is the nature of an outlier?  If
each data point is a valid (correctly performed) measurement, then each point is
as important as the others.  We would never want to throw away such data, so a
warning about outliers is improper.  Of course, if our goal is to identify the
_fastest_ possible execution for a program, then we will discard all points
above the minimum measurement.  Here, outlier detection is irrelevant.  On the
other hand, our goal may be to capture a notion of _typical_ runtime.  In that
case, we certainly would not use the mean value, and even the median may not be
sufficiently representative.  In a possibly-mixed distribution with multiple
peaks and a long tail, the modal value(s) may be the most useful
characterization.  By definition, the mode represents the most frequently
occurring runtime. 

Our goal might be neither of the above.  In the kind of performance engineering
often seen in industry, analyzing applications running on servers, the long tail
_is the problem_.  The longest runtimes may represent unsatisfactory customer
experiences or poor server utilization, for instance.  Here, the goal is usually
to identify the 99th percentile figure (where 99/100 executions took less time).
By then profiling the executions in the 99th percentile, changes to code,
configuration, or runtime environment can be tested to see if a reduction in the
99th percentile figure is obtainable.

Regarding outlier detection, we conclude that it is not an appropriate method no
matter the goal of benchmarking.


## Detailed discussion

- **Differences in measurements** The BestGuess median times are lower than the
  Hyperfine medians.  A possible explanation is that BestGuess and Hyperfine use
  different techniques to compute the runtimes.
  - BestGuess is getting the user and system time from the OS using
    [wait4()](https://linux.die.net/man/2/wait4), via which the OS reports
    process statistics as of the moment the child exited.
  - Hyperfine calls `getrusage(RUSAGE_CHILDREN, &mut buf)` before and after
    starting a child process to run the command.  Between the two calls to
    `getrusage`, some Rust code must execute: `command.spawn()` and
    `child.wait()`.  See [the actual
    code](https://github.com/sharkdp/hyperfine/blob/master/src/timer/mod.rs#L82-L109)
    for details.
<!-- 
	let cpu_timer = self::unix_timer::CPUTimer::start();
    let mut child = command.spawn()?;
    let status = child.wait()?;
    let (time_user, time_system) = cpu_timer.stop(); 
-->
  - The Hyperfine runtime measurement includes the time needed for those Rust
    routines, plus the time to `fork()` and `exec()` (on Unix).  Also, any
    context switches in the parent process before the fork and after the wait
    have the potential to pollute CPU caches and other components, making the
    (timed) code that runs afterward take more cycles.
  - Example: When timing a very fast command, like `ls -l`, BestGuess reports
    the runtime range as _2.4 ms - 3.5 ms_ on my machine with 1000
    runs.  Hyperfine reports _5.2 ms … 10.1 ms_.  This is a significant
    difference, one that may be due to the measurement issues mentioned above. 
  - Hyperfine reports that `ls -l` took _6.0 ms ± 0.7 ms_.  Ignoring the
    irrelevant standard deviation figure, the mean is at 16% of the range
    interval, whereas the BestGuess mode (and median as it turns out) is at the
    9% mark, i.e. closer to the minimum.  Repeated experiments with runs between
    100 and 1000 produce some variation in the range with BestGuess, but the mode
    remains stubbornly at 2.5ms.
  - BestGuess avoids many sources of error by using the system call provided for
    obtaining a snapshot of exactly what the OS can tell us about the child
    process that ran our command.  The reported statistics are all robust,
    avoiding measures suited only to normal distributions.

- **Measure of central tendency** To characterize a distribution using a single
  number, we choose a measure such as one of the many forms of mean, or the
  median or mode.  We know that runtime distributions are skewed, possibly
  mixed, and certainly not normal.  We do not believe the arithmetic mean is a
  useful characterization of such a distribution.  Median is better, as it is
  less influenced by the tail of high runtimes.  However, the mode may be the
  best single characterization, particularly if the goal is to assess what is a
  "typical" runtime.
  - We recommend that Hyperfine users ignore the mean runtime value and use the
    median instead.  Even better, if the goal is to find a typical runtime, is
    to output the raw data (possibly only via `--export-json`) and estimate the
    mode(s). 
  - Note that the Hyperfine reports of user and system time are mean values, and
    should be ignored.
  - BestGuess [estimates the mode](https://arxiv.org/abs/math/0505419) in a
    distribution-independent manner.
  - The range of runtimes for each benchmarked command can be illuminating.  The
    minimum, of course, may be a good characterization of best possible
    performance.  The maximum is a strong hint about how bad things could get,
    and may prompt close examination of the long tail of high runtimes.
    Profiling could reveal opportunities to reduce the tail.

- **Measures of variance** There are no widely accepted measures of variance for
  the kinds of distributions we are seeing.  Runtime data may follow a mixed
  distribution, as suggested by the presence of multiple peaks.  It certainly is
  skewed, with a long rightward tail.
  - BestGuess reports the range of measured runtimes, as no robust variance
    measure is known.  Faith in the formula for the standard deviation of a
    normal distribution is misplaced here.
  - The 99th percentile runtime, particularly for a large sample, can be very
    useful for performance tuning.  Profiling can sometimes result in changes
    that reduce the 99th percentile runtime value.  This can yield better server
    utilization.  In situations where a person is waiting on a result, lower
    99th percentile figures typically increase user satisfaction, and this can
    be true even when the median or modal wait time increases.  (Humans seem to
    prefer predictable intervals over erratic ones, even at the cost of a longer
    typical wait time.)

## About the long tail

Some notes follow.

Because we are measuring user and system time, not real time, we are getting the
operating system's report about processor time expended on our program.  The
time that elapses when our program is not actually running is not included.
Why, then, do we get such an interesting distribution of runtimes?
  
- A treatise on sources of interference will have to wait.  And, it's been done.
  Below we mention data and instruction caches as well as the branch predictor.
  There are also clock speed variations, loop buffers, and other CPU features
  that can slow down the resumption of a process that has been suspended.

- Interference in the form of cache pollution and branch misprediction can make
  runtimes longer.  In most modern CPUs, each core has a private data cache,
  instruction cache, and branch predictor.  These are not usually under program
  control and operate automatically.  When the OS scheduler suspends our process
  so that another can run on that core, the caches and branch prediction tables
  end up containing what that process needs.  When our process resumes, it may
  well find that memory fetches, for both instructions and data) take longer, at
  least initially.  And branches that were successfully predicted earlier may
  now be for a time mispredicted.  A process suspension (and resumption of
  another) is called a _context switch_.

- When a benchmarked command runs very briefly, there are few opportunities for
  any context switches.  However, should some occur, they may significantly
  increase the benchmarked runtime because an OS scheduler runs each process for
  some minimum amount of time, perhaps in the range of 0.5ms to 5ms.
  Benchmarking a command that takes an ideal 5ms, we may also measure times near
  10ms or 15ms if 1 or 2 context switches at 5ms each occurred.  Of course,
  another process may not use its entire time quantum, so we are likely to see
  quite a range of runtimes.

- When a benchmarked command runs for a long time, there are more opportunities
  for context switches.  Each one may induce more or less interference of the
  types suggested.  Only actual measurements can shed light on the effects of
  context switches.  BestGuess reports the number of voluntary and involuntary
  context switches that occurred while a benchmarked command executed.  Because
  this number is reported by the OS, we will have some confidence in it until we
  learn otherwise.  When high runtimes correlate with context switches, we have
  a possible culprit in interference.  Profiling should shed light on what is
  actually happening.


## Further investigation

A thorough investigation is needed into these topics.  An illuminating example
follows, in which `ls -l` is measured by both BestGuess and Hyperfine without
using a shell.  Note that 10,000 runs were executed.

```shell 
$ bestguess -r=10000 "ls -l"
Use -o <FILE> or --output <FILE> to write raw data to a file.
A single dash '-' instead of a file name prints to stdout.

Command 1: ls -l
                         Mode       Median              Range
    Total time          1.7 ms       1.7 ms       1.6 ms  -    3.4 ms 
    User time           0.5 ms       0.6 ms       0.5 ms  -    1.0 ms 
    System time         1.2 ms       1.2 ms       1.1 ms  -    2.5 ms 
    Max RSS             1.9 MiB      1.9 MiB      1.6 MiB -    2.5 MiB
    Context switches     20 ct        22 ct        14 ct  -     64 ct

~/Projects/bestguess<main>$ hyperfine --style=basic -S=none -r=10000 "ls -l"
Benchmark 1: ls -l
  Time (mean ± σ):       6.1 ms ±   0.4 ms    [User: 2.3 ms, System: 1.7 ms]
  Range (min … max):     5.6 ms …  17.5 ms    10000 runs
 
  Warning: The first benchmarking run for this command was significantly slower than the rest (9.0 ms). This could be caused by (filesystem) caches that were not filled until after the first run. You should consider using the '--warmup' option to fill those caches before the actual benchmark. Alternatively, use the '--prepare' option to clear the caches before each timing run.
 
$ bestguess -v
bestguess 0.2.0
$ hyperfine -V
hyperfine 1.18.0
$ 
```

Observations:

1. The time measured by Hyperfine is much larger at _6.1 ms_ than the time
   measured by BestGuess, _1.7 ms_.  This is a typical result in head-to-head
   trials, and it would be edifying to know precisely why it is so.  (We
   speculate it is measurement error in Hyperfine, but we do not know.)
2. The range produced by Hyperfine is large as well, at 3 times the minimum
   value.  The range observed by BestGuess is smaller, at 2 times the min.
   Again, we would like to understand precisely the source of the difference.
   The much larger user time figure suggests that the Rust code execution
   included in Hyperfine measurement may be a factor.
3. In the BestGuess graph of individual runtimes shown below, we see clearly
   that the first execution took much longer than the others.  This is why
   warmup runs are used when assessing typical performance.  Looking at the rest
   of the graph, we see oscillations.  Even something as simple as `ls -l` run
   repeatedly on the same directory does not settle into near-optimal
   performance.  Are we seeing the same phenomenon Laurie Tratt observed when
   measuring virtual machine performance?  More investigation is needed.

```shell 
$ bestguess -g -r=30 "ls -l"
Use -o <FILE> or --output <FILE> to write raw data to a file.
A single dash '-' instead of a file name prints to stdout.

Command 1: ls -l
                         Mode       Median              Range
    Total time          2.2 ms       2.1 ms       1.7 ms  -    3.4 ms 
    User time           0.7 ms       0.7 ms       0.6 ms  -    0.8 ms 
    System time         1.5 ms       1.5 ms       1.2 ms  -    2.5 ms 
    Max RSS             1.9 MiB      1.9 MiB      1.8 MiB -    2.0 MiB
    Context switches     25 ct        25 ct        19 ct  -     29 ct
0                                                                             max
|▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
|▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
|▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
|▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
|▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
|▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
|▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
|▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
|▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
|▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
|▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
|▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
|▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
|▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
|▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
|▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
|▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
|▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
|▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
|▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
|▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
|▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
|▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
|▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
|▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
|▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
|▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
|▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
|▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭
|▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭▭

$ 
```