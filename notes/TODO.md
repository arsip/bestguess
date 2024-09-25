## Open questions about experimentation

- [ ] We should systematically compare results between:
	  - `cmdbench` (https://github.com/manzik/cmdbench pip3 install cmdbench)
	  - Hyperfine (https://github.com/sharkdp/hyperfine)
	  - bash built-in time command
	  - /usr/bin/time

- [ ] Is there a correlation between the number of data points collected and the
      long tail (as typified, perhaps, by the 95th or 99th percentile value)?
	  - Hypothesis: The longest run times are produced when the program being
        measured is suspended by the OS, possibly multiple times.
	  - With enough samples, we are more likely to encounter such long run
        times, provided that each experiment is long relative to the OS
        scheduling quantum.
	  - For experiments that take around the OS quantum time (or less), we may
        not see run times resulting from multiple suspensions simply because
        there is no opportunity for that to occur.  However, short duration
        experiments may show a high variance because a single interruption
        could multiply the run time. E.g. the run time would double if an
        uninterrupted experiment took one OS quantum and we measured an
        execution that was interrupted once.

- [ ] We should make clear our notion of common use cases and how BestGuess can
	  be used to achieve them.  Primary use cases:
	  1. Which of a set of programs is typically fastest?
	  2. What is the typical cold start time for each program?
	  3. How much variability is observed when a program is run repeatedly?
	  4. Does the performance of a program over repeated executions tend to get
         faster or slower?  Does it settle into a high-performance state or
         some other state?  Does it oscillate?  Does it seem chaotic?

- [ ] When characterizing a sample distribution, we usually assume that each
      data point in a sample is independent.
	  - The difference between cold and warm starts violates that assumption, as
        the warmup runs prime all manner of caches and other
        performance-boosting gizmos.
	  - We can recover the independence property by executing some warmup runs
        if we assume that subsequent timed runs will be independent.
	  - However, repeated executions of compute-intensive programs can trigger
        actions that affect performance.
		- [Apple suggests](https://developer.apple.com/news/?id=vk3m204o) that
          the load imposed by an application is monitored and an appropriate
          core is selected for it, as newer Apple CPUs have both performance and
          efficiency cores.
	    - Even if the OS does not actively move a process between unequal cores,
          other performance-affecting events can occur.
	    - Most current Intel CPUs have dynamically frequency scaling, and will
          increase the clock frequency as needed.  This may be most noticeable
          when a program is single-threaded and compute-intensive.  Depending on
          the precise dynamics of this scaling, it could (in theory) mean that
          execution N of a program begins at one clock frequency and finishes at
          a higher one, and execution N+1 runs entirely at the higher clock
          frequency.  This is a dependence of sorts between samples.
	    - Finally, modern CPUs throttle down when high CPU temperatures are
          detected.  Perhaps servers experience such throttling in practice as
          well, though active cooling and densely-packed equipment racks could
          conceivably moderate the degree of throttling.  In any case, modeling
          the effect of thermal throttling is going to be complicated, involving
          not only the physics of heat transfer, but also the properties of
          thermal sensors and details of the control algorithms used by CPUs to
          react to high temperatures.

- [ ] Natalie proposed what seems like a very useful taxonomy of factors that
      affect software performance.  My paraphrasing of it is:
	  1. Type I factors are those over which the software developer has no
         practical control.  For example, at compile time, we have no control
         over nor insight into the nature of the system load at run time.  We
         may gain tremendous insight from testing our code in a setting where we
         can control the other system loads, and we may optimize based on
         expected run time conditions, but we do not directly control those
         conditions.  Benchmarking to understand the effect of Type I factors
         requires laboratory-like conditions.  The `Krun` tool built by Barrett
         et al. was needed to conduct the experiments described in [1] due to
         the desire to control Type I factors.
	  2. Type II factors are characterized by (1) their ability to be
         controlled, and (2) the variability of their effect across platforms.
         E.g. controlling the link order of objects that produce an executable
         may affect performance on only some platforms, perhaps due to
         instruction cache size (a CPU property) and program load address (an OS
         property).  We focus on Type II factors when we want to tune a piece of
         software for a particular platform.
	  3. Type III factors are under developer control and their effect on
         performance is largely platform-independent.  A well-recognized example
         here is the choice of a cache-friendly data structure.  Constraints on
         time and effort that can be devoted to performance tuning may result in
         attention paid only to Type III factors.  If our code performs well on
         some reasonable variety of platforms, we declare success and move on.
         When reliably good performance is important, we may tune for one or
         more platforms by adding Type II factors to our experiments.  Finally,
         when it's crucial to _predict_ performance well, we also perform
         controlled experiments to understand the variability induced by those
         factors and perhaps to issue warnings about runtime conditions that may
         negatively impact performance.  (E.g. we may discover that a feature
         like hyper-threading should be disabled to maintain predictable good
         performance.)

- [ ] The OS scheduling quantum would seem to be relevant.  
	  - As of Linux kernel v6.11, the "minimal preemption granularity for
        CPU-bound tasks" defaults to 0.75 msec * (1 + ilog(ncpus)).  The integer
        log base 2 of 4 cores is 2, so on a 4-core CPU, a process that does not
        get preempted will run for 0.75ms * (1 + 2) = 2.25ms.  That number seems
        quite low, but it appears this is an initial value that grows.
		The algorithm described as the Completely Fair Scheduler in
        https://docs.kernel.org/scheduler/sched-design-CFS.html uses a priority
        queue sorted by lowest quantum.  The shortest task is dequeued, run for
        its quantum, then the quantum is enlarged and it is re-queued.
	  - Another description of the Completely Fair Scheduler says "Instead of an
        absolute time, the CFS algorithm assigns a proportion of the processor,
        so the amount of processor time depends on the current load."
        https://notes.eddyerburgh.me/operating-systems/linux/process-scheduling#timeslice
        cites _Linux Kernel Development (Developer’s Library), 3rd
        ed. Addison-Wesley Professional, 2010_ for this.
	  - Another reference, in addition to numerous blog posts:
        https://elixir.bootlin.com/linux/v6.11/source/kernel/sched/fair.c#L71-L75 
      - Linux also has a real-time scheduler, as does macOS and Windows.
        Benchmarking real-time systems is out of scope, so the only apparent
        relevance of the real-time capability is that on a system where such
        processes are executing, the interference they generate with our
        benchmark may well have a different character.  However, we are not
        addressing (yet) how to interpret benchmarks made on a heavily loaded
        system. 
	  - Importantly, processes preempted are often given a priority boost so
        that they run sooner (once they are ready).  If we are measuring
        CPU-bound processes, this may be less of a factor, as presumably
        preemption does not occur.  Benchmarking on a heavily loaded system
        would, of course, increase the likelihood of such preemption.
	  - On macOS, the preemption rate is reportedly the `hz` value of
	    `kern.clockrate`, which is (often? always?) 100HZ.  The quantum is then
	    10ms. (See https://flylib.com/books/en/3.126.1.80/1/ for more.)
	      ```shell
		  $ sysctl kern.clockrate
		  kern.clockrate: { hz = 100, tick = 10000, tickadj = 0, profhz = 100, stathz = 100 }
		  $ 
		  ```
	  - Reportedly, the Windows desktop quantum is 20ms unless the process "owns
        the foreground window" in which case it's 60ms.  The source _Windows
        Internals by Solomon, Russinovich, et al._ is cited by 
		https://superuser.com/questions/1326252/changing-windows-thread-sheduler-timeslice
	  - A post on Medium claims the Windows Server quantum is fixed at 120ms,
        which is entirely believable, though I cannot readily find a source.


REFERENCES

[1] E. Barrett, C. F. Bolz-Tereick, R. Killick, S. Mount, L. Tratt, _Virtual
Machine Warmup Blows Hot and Cold_.

[2] T. Kalibera, L. Bulej, P. Tuma, _Benchmark Precision and Random Initial
State_.



## Measurement techniques

- [ ] Provide configurable environment randomization.  (Really this is just
      creating an environment variable of a random size before launching the
      process to be benchmarked.)

- [ ] Programs that use pthreads do not have thread resources accrue to the
      "parent" process.  Is there any reasonable way to measure them, short of
      introducing a shim library for pthreads?

- [ ] Repeated experiments or large experiments?  Suppose we run an experiment
      that yields 300 observations each of program A and program B.  If we had
      run 30 experiments of 10 observations each, the 30 sample medians for A
      should be normally distributed (same for B).  We can use a t-test to gauge
      whether A is faster than B.  Given the nature of a typical experimental
      setup (ad hoc perhaps, with other processes running, network and user
      activity), would we get the same results from the one large experiment?
      What if we sliced up the 300 observations into 30 samples of 10 each, and
      pretended they were separate experiments?  What makes them NOT separate,
      i.e. what is different about 30 sets of 10?


## BestGuess features

- [ ] Raw data files should have a "batch number" in every row.  That way, users
      can benchmark the same command more than once (or combine results files
      that include the same command) and the reporting can use the batch numbers
      to tell them apart.

- [ ] The Hyperfine ability to let the user name their commands is another good
      one from that project.  It should make reports much easier to read.

- [ ] We need a way to set parameters like terminal width, minimum effect size,
      and others from the command line.  Currently, we have `--width` but this
      should be unified with other settings so we don't have to have a unique
      flag for each.  Java uses `-XX:` for non-standard options, but that seems
      like the wrong vibe.
	  - [ ] Maybe `-c` / `--config`?
	  - [ ] Value would have the form _name=value_, e.g. `-c width=60`
	  - [ ] Could allow environment variables, e.g. `-c width=$COLUMNS` if we
            can (1) find no security issues and (2) have a syntax that does not
            prevent a literal string that starts with `$` to be the value of a
            setting. 

- [ ] Set defaults (e.g. warmups, runs, shell) and settings from a `.bestrc`
      file. 

- [ ] Our reporting is becoming so comprehensive that we may want to support
      reporting on a subset of samples found in raw data files.  Assuming each
      sample has a "batch number" unique to its data file, we might allow users
      to report on certain ones, e.g. `--batch 2,3-5,10`.

- [ ] Consider an `--explain` option that prints explanations of descriptive and
      inferential statistics.  E.g. `--explain infer` or maybe more granular
      like `--explain skew`.

- [ ] We can compute Mann-Whitney U values for sample sizes under 20 if we use a
	  table of critical U values instead of the normal-based approximation to
	  p-value calculation we support today.  See,
	  e.g. https://sphweb.bumc.bu.edu/otlt/MPH-Modules/BS/BS704_Nonparametric/BS704_Nonparametric4.html

- [ ] A log feature might be useful.  It would log actions such as warmup runs
      and timed runs, to make the overall behavior explicit.  This feature could
      be used by people who want to better understand the operation of BestGuess
      as well as scientists who want to record an experiment log.
	  - Including some information about the platform at the start of the log
        could be useful.  OS and hardware (CPU, memory)?
	  - Writing timestamps and measures of system load (and temperature?) into
        the log file between timed executions would be nice.
		
	    Function: int getloadavg (double loadavg[], int nelem)
		This function gets the 1, 5 and 15 minute load averages of the system.

- [-] Maybe compute medcouple, a non-parametric measure of skewness.

- [ ] There are several conventions for drawing box plots.  Should we adopt the
      one where the whiskers extend at most 1.5 x IQR, with observations beyond
      shown as dots?  That form is described as "box plot with outliers", but
      performance data often has some very observations that should not be
      considered outliers -- they are actual measurements, indicating that the
      measured program *can* take that long.

- [X] Have install script report errors clearly if cannot copy executable

- [X] Max RSS can exceed 1000 MiB, so convert to larger unit in that case

- [X] Context sw total can exceed 1000 Kct, so convert to larger unit in that
      case
	  
- [X] Capitalize the 's' in "Context Sw"

- [X] Show wall clock time as well (useful for multi-threaded programs)

- [X] Maybe box plots

	   Example
	   ┌───┬──┐
   ├╌╌╌┤   │  ├╌╌╌╌╌╌╌╌┤
	   └───┴──┘
			 1         2
   012345678901234567890
	  IQR=11-4+1=8
 WL=4-0=4  |   WR=20-11=9
	   MEDIAN=8

   When MED=Q1:
	   ╓───┬──┐
   ├╌╌╌╢   │  ├╌╌╌╌╌╌╌╌┤
	   ╙───┴──┘

   When MED=Q3:
	   ┌─────╖
   ├╌╌╌┤     ╟╌╌╌╌╌╌╌╌┤
	   └─────╜


	Smallest possible with box still visible
	  Box: IQR = 2; MED - Q1 = 1; Q3 - MED = 1
	  Whiskers: Q1 - MIN = 1; MAX - Q3 = 1
	 ┌┬┐
	├┤│├┤
	 └┴┘
	01234

	Smallest box showing mean, smallest whiskers: 
	  Box: IQR = 2; MED - Q1 = 1; Q3 - MED = 1
	  Whiskers: Q1 - MIN = 0; MAX - Q3 = 0
	┌┬┐
	┤│├
	└┴┘
	012

	Smallest box, smallest whisker: 
	  Box: IQR = 1; MAX = MIN + 1; MED = MIN or MED = MAX
	  Whiskers: Q1 - MIN = 0; MAX - Q3 = 0
	┌┐
	┤├
	└┘
	01

	Smallest everything: 
	MAX = MIN; IQR = 0

	┼

	0


