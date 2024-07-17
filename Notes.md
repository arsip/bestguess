# Rationale

After using hyperfine (https://github.com/sharkdp/hyperfine), I've concluded it
has the wrong approach.  There are two problems and some design limitations.

A proper critique of Hyperfine would include (at length) a description of its
strengths.  This is not such a critique, though in fairness to Hyperfine it is
an excellent tool overall.  Aside from the statistics it calculates, the tool is
well thought out.  It works on many platforms and automates many aspects of
benchmarking well.  The output is very well displayed, showing careful attention
to detail in presenting result summaries.

**Problem 1:** Hyperfine assumes a normal distribution when calculating all of
the stats, and performance measurements are not normally distributed.  There is
a minimum time that may be approached, but which can never be surpassed because
there is a certain number of cycles needed to execute a deterministic algorithm.

Performance data may well follow a log-normal distribution.  In any case, we
have used an Anderson-Darling test to confirm that the data we are seeing (in a
variety of settings across a variety of platforms) is astoundingly far from
being normally distributed.

Because all of Hyperfine's statistics are calculated assuming a normal
distribution, we cannot rely on its (1) standard deviation, (2) outlier
detection, or (3) mean values.  (Because performance can get almost arbitrarily
bad, but can only get so good, high runtimes skew the mean.)

**Problem 2:** There is no basis (best practice or scientific evidence) to
suggest that any of the following default features are appropriate and work
correctly:

- Measuring shell spawning time by taking the mean of 50 iterations
- Correcting for shell spawning time (or determining when that measurement is unreliable)
- Running a command repeatedly for at 3 seconds and at least 10 timed runs
- Detecting statistical outliers using a normal distribution model

**Design issue 1:** The only way to get all of the individual timed values out
of Hyperfine is in JSON.  Unix tools are built for text, so a CSV file would be
preferable.  Hyperfine has a CSV output option, but it does not include all of
the data that the JSON format does.

**Design issue 2:** Many other benchmarking systems use one program to collect
the data and another to analyze it.  This is conducive to better science,
because the raw data is always saved, apart from any analysis.  Saving the
timing data allows several kinds of analysis to be done, even much later after
the timing runs are complete.  Other people can analyze the data in different
ways.

**Design issue 3:** Admittedly a minor issue, and certainly a personal
preference: Python is not a desirable dependency for any project.  The data
analysis scripts that come with Hyperfine are Python programs.  Maintaining a
working Python environment is laborious and error-prone, requiring
Python-specific virtual environments of one type or another.  Hyperfine users
must be able to compile Rust, so why not roll the optional analyses done by the
Python scripts into the Hyperfine code, or at least into a companion program?

Much of Hyperfine's functionality might better fit in an external program.
Certainly the statistical calculations are in that category, but it is also
simple to generate a list of commands that iterate through parameter ranges.


# Enhancements to consider

    --prepare CMD    execute CMD before each timed run
    --conclude CMD   execute CMD after each timed run



# If we want to gather system information

From sysctl (command-line) on macos 14.1.1:

	kern.osproductversion: 14.4.1
	kern.osreleasetype: User

	hw.ncpu: 8
	hw.byteorder: 1234
	hw.memsize: 17179869184
	hw.activecpu: 8
	hw.physicalcpu: 8
	hw.physicalcpu_max: 8
	hw.logicalcpu: 8
	hw.logicalcpu_max: 8
	hw.cputype: 16777228
	hw.cpusubtype: 2
	hw.cpu64bit_capable: 1
	hw.cpufamily: 458787763
	hw.cpusubfamily: 2
	hw.cacheconfig: 8 1 4 0 0 0 0 0 0 0
	hw.cachesize: 3499048960 65536 4194304 0 0 0 0 0 0 0
	hw.pagesize: 16384
	hw.pagesize32: 16384
	hw.cachelinesize: 128
	hw.l1icachesize: 131072
	hw.l1dcachesize: 65536
	hw.l2cachesize: 4194304


# If we want to clear caches

On Linux with systemd:

To clear PageCache, dentries and inodes, use this command:

    $ sudo sysctl vm.drop_caches=3

On Linux without systemd, run these commands as root:

	# sync; echo 1 > /proc/sys/vm/drop_caches # clear PageCache
	# sync; echo 2 > /proc/sys/vm/drop_caches # clear dentries and inodes
	# sync; echo 3 > /proc/sys/vm/drop_caches # clear all 3

On macos:

	$ sync && sudo purge

# Regarding I/O operations reported in rusage

The macos kernel does not appear to populate the i/o block
operation counters in r_usage, at least not on a 2020 MacBook Air
(SSD) running macos 14.4.1.  The current linux kernels do report
block operation counts, but (1) only for actual disk reads, not for
files served from cache, and (2) it is not simple to determine the
block size.  E.g. One of my machines reports a block size of 4092
for its SSD (via 'blockdev'), but the effective size appears to be
8192, probably due to the filesystem or mount options.

Conclusion: We will not record values for "Input ops" (`ru_inblock`)
or "Output ops" (`ru_oublock`).




