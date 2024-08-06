
- [ ] Have install script report errors clearly if cannot copy executable

- [ ] Max RSS can exceed 1000 MiB, so convert to larger unit in that case

- [ ] Context sw total can exceed 1000 Kct, so convert to larger unit in that
      case
	  
- [ ] Capitalize the 's' in "Context Sw"

- [ ] Show wall clock time as well (useful for multi-threaded programs)

- [ ] Programs that use pthreads do not have thread resources accrue to the
      "parent" process.  Is there any reasonable way to measure them, short of
      introducing a shim library for pthreads?

