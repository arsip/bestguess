# Best Guess

## Description

The goal, in short, is to improve on Hyperfine (a popular benchmarking tool).

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


## Examples

### Output to terminal

Run 3 commands without using a shell, doing 5 warmup runs and 10 timed runs.
Output, in CSV format, will be printed to the terminal because no output
filename was given:

```shell
$ bestguess --warmup 5 --runs 10 \
"./run pexl run findAll longword 10" \
"./run lpeg run captureAll longword 10" \
"./run rosie run captureAll longword 10"
```

These option names are the same ones used by Hyperfine, and can be shortened to
`-w` and `-r`, respectively.


### Output to file

Add `-o filename` to the above command to get:

```shell
$ bestguess --warmup 5 --runs 10 \
-o data.csv \
"./run pexl run findAll longword 10" \
"./run lpeg run captureAll longword 10" \
"./run rosie run captureAll longword 10"
```

A similar option for Hyperfine is `--export-csv` which writes summary statistics
to a CSV file.  (Hyperfine will not export the raw timing data in any format
except JSON.)


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
$ bestguess -r 3 "ls -l" "/bin/ls -l"
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
$ bestguess -r 3 -S "/bin/bash -c" "" "ls -l"
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

It's all mine.  I did it because it needed to be done.

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
