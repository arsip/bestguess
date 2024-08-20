
- [X] Have install script report errors clearly if cannot copy executable

- [X] Max RSS can exceed 1000 MiB, so convert to larger unit in that case

- [X] Context sw total can exceed 1000 Kct, so convert to larger unit in that
      case
	  
- [X] Capitalize the 's' in "Context Sw"

- [X] Show wall clock time as well (useful for multi-threaded programs)

- [ ] Programs that use pthreads do not have thread resources accrue to the
      "parent" process.  Is there any reasonable way to measure them, short of
      introducing a shim library for pthreads?

- [ ] Maybe box plots?

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


