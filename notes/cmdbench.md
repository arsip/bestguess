# Comparing with cmdbench

## Installing

Last updated on November 6, 2020, cmdbench is still being
[recommended](https://www.linuxlinks.com/BenchmarkTools/) as of October 7, 2023.
Giant caveat: That site looks like a bot-produced scraping of other sources.

It installed without errors via `pip install cmdbench`.

The README disclaims that the tool is primarily for Linux but supports Windows
and macos as well.  However, like many projects implemented in Python, it is not
very resilient.  Below, we see that it fails on macOS 14.4.1 (Friday, August 2,
2024). 

Note: cmdbench pulls down many dependencies, including the following.

> Click-8.1.7 beeprint-2.4.11 cmdbench-0.1.13 colorama-0.4.6 contourpy-1.2.1
> cycler-0.12.1 fonttools-4.53.1 kiwisolver-1.4.5 matplotlib-3.9.1 numpy-2.0.1
> packaging-24.1 pillow-10.4.0 psutil-6.0.0 pyparsing-3.1.2
> python-dateutil-2.9.0.post0 six-1.16.0 tqdm-4.66.4 typing-extensions-4.12.2
> urwid-2.6.15 wcwidth-0.2.13


## Not working with Python 3.12

```shell 
$ cmdbench
Traceback (most recent call last):
  File "/opt/homebrew/bin/cmdbench", line 5, in <module>
    from cmdbench.cli import benchmark
  File "/opt/homebrew/lib/python3.12/site-packages/cmdbench/cli.py", line 8, in <module>
    import pkg_resources
ModuleNotFoundError: No module named 'pkg_resources'
```

## Not working with Python 3.9

Variations of the command below, including `ls -l` (no shell needed) and 
`bash -c "ls -l >/tmp/ls-l"` (shell explicitly included) also fail.


```shell 
$ cmdbench --iterations=1 "ls -l >/tmp/ls-l" 

Benchmarking started..
  0%|                                                                                                    | 0/1 [00:00<?, ?it/s]Process Process-2:
Process Process-3:
Traceback (most recent call last):
Traceback (most recent call last):
  File "/usr/local/lib/python3.9/site-packages/psutil/_psosx.py", line 349, in wrapper
    return fun(self, *args, **kwargs)
  File "/usr/local/lib/python3.9/site-packages/psutil/_common.py", line 508, in wrapper
    raise raise_from(err, None)
  File "<string>", line 3, in raise_from
  File "/usr/local/lib/python3.9/site-packages/psutil/_psosx.py", line 349, in wrapper
    return fun(self, *args, **kwargs)
  File "/usr/local/lib/python3.9/site-packages/psutil/_common.py", line 506, in wrapper
    return fun(self)
  File "/usr/local/lib/python3.9/site-packages/psutil/_common.py", line 508, in wrapper
    raise raise_from(err, None)
  File "/usr/local/lib/python3.9/site-packages/psutil/_psosx.py", line 375, in _get_kinfo_proc
    ret = cext.proc_kinfo_oneshot(self.pid)
  File "<string>", line 3, in raise_from
  File "/usr/local/lib/python3.9/site-packages/psutil/_common.py", line 506, in wrapper
    return fun(self)
  File "/usr/local/lib/python3.9/site-packages/psutil/_psosx.py", line 375, in _get_kinfo_proc
    ret = cext.proc_kinfo_oneshot(self.pid)
ProcessLookupError: [Errno 3] assume no such process (originated from sysctl(kinfo_proc), len == 0)

During handling of the above exception, another exception occurred:

ProcessLookupError: [Errno 3] assume no such process (originated from sysctl(kinfo_proc), len == 0)
Traceback (most recent call last):

During handling of the above exception, another exception occurred:

Traceback (most recent call last):
  File "/usr/local/lib/python3.9/site-packages/psutil/__init__.py", line 355, in _init
    self.create_time()
  File "/usr/local/lib/python3.9/site-packages/psutil/__init__.py", line 757, in create_time
    self._create_time = self._proc.create_time()
  File "/usr/local/lib/python3.9/site-packages/psutil/_psosx.py", line 349, in wrapper
    return fun(self, *args, **kwargs)
  File "/usr/local/lib/python3.9/site-packages/psutil/__init__.py", line 355, in _init
    self.create_time()
  File "/usr/local/lib/python3.9/site-packages/psutil/_psosx.py", line 477, in create_time
    return self._get_kinfo_proc()[kinfo_proc_map['ctime']]
  File "/usr/local/lib/python3.9/site-packages/psutil/__init__.py", line 757, in create_time
    self._create_time = self._proc.create_time()
  File "/usr/local/lib/python3.9/site-packages/psutil/_psosx.py", line 354, in wrapper
    raise NoSuchProcess(self.pid, self._name)
  File "/usr/local/lib/python3.9/site-packages/psutil/_psosx.py", line 349, in wrapper
    return fun(self, *args, **kwargs)
  File "/usr/local/lib/python3.9/site-packages/psutil/_psosx.py", line 477, in create_time
    return self._get_kinfo_proc()[kinfo_proc_map['ctime']]
  File "/usr/local/lib/python3.9/site-packages/psutil/_psosx.py", line 354, in wrapper
    raise NoSuchProcess(self.pid, self._name)
psutil.NoSuchProcess: process no longer exists (pid=6528)
psutil.NoSuchProcess: process no longer exists (pid=6528)

During handling of the above exception, another exception occurred:


During handling of the above exception, another exception occurred:

Traceback (most recent call last):
Traceback (most recent call last):
  File "/usr/local/Cellar/python@3.9/3.9.4/Frameworks/Python.framework/Versions/3.9/lib/python3.9/multiprocessing/process.py", line 315, in _bootstrap
    self.run()
  File "/usr/local/Cellar/python@3.9/3.9.4/Frameworks/Python.framework/Versions/3.9/lib/python3.9/multiprocessing/process.py", line 108, in run
    self._target(*self._args, **self._kwargs)
  File "/usr/local/lib/python3.9/site-packages/cmdbench/core.py", line 188, in collect_time_series
    p = psutil.Process(shared_process_dict["target_process_pid"])
  File "/usr/local/Cellar/python@3.9/3.9.4/Frameworks/Python.framework/Versions/3.9/lib/python3.9/multiprocessing/process.py", line 315, in _bootstrap
    self.run()
  File "/usr/local/lib/python3.9/site-packages/psutil/__init__.py", line 319, in __init__
    self._init(pid)
  File "/usr/local/Cellar/python@3.9/3.9.4/Frameworks/Python.framework/Versions/3.9/lib/python3.9/multiprocessing/process.py", line 108, in run
    self._target(*self._args, **self._kwargs)
  File "/usr/local/lib/python3.9/site-packages/psutil/__init__.py", line 368, in _init
    raise NoSuchProcess(pid, msg=msg)
  File "/usr/local/lib/python3.9/site-packages/cmdbench/core.py", line 126, in collect_fixed_data
    p = psutil.Process(shared_process_dict["target_process_pid"])
  File "/usr/local/lib/python3.9/site-packages/psutil/__init__.py", line 319, in __init__
    self._init(pid)
  File "/usr/local/lib/python3.9/site-packages/psutil/__init__.py", line 368, in _init
    raise NoSuchProcess(pid, msg=msg)
psutil.NoSuchProcess: process PID not found (pid=6528)
psutil.NoSuchProcess: process PID not found (pid=6528)
Last runtime:  0.003 seconds: 100%|████████████████████████████████████████████████████████████████| 1/1 [00:01<00:00,  1.52s/it]
Benchmarking done.

====> First Iteration <====

Process: 
    Stdout: ''
    Stderr: 'ls: >/tmp/ls-l: No such file or directory\n'
    Runtime: 0.003 second(s)
    Exit code: 1

CPU (seconds): 
    User time: 0
    System time: 0
    Total time: 0

Memory (bytes): 
    Maximum: 0
    Maximum per process: 0

Time series: 
    Sampling milliseconds: array([], dtype=float64)
    CPU (percentages): array([], dtype=float64)
    Memory (bytes): array([], dtype=float64)


Done.
$ 
```

