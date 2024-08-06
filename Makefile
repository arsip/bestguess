## -*- Mode: Makefile; -*-                                              
##
## The 'bestguess' project for command-line benchmarking
##

PROGRAM=bestguess
SRCDIR=src
TESTDIR=test

default: debug

debug: clean
	@$(MAKE) -C $(SRCDIR) RELEASE_MODE=false $(PROGRAM) && cp $(SRCDIR)/$(PROGRAM) .

release: clean
	@$(MAKE) -C $(SRCDIR) RELEASE_MODE=true $(PROGRAM) && cp $(SRCDIR)/$(PROGRAM) .

# ------------------------------------------------------------------

clean:
	-rm -f $(PROGRAM)
	@$(MAKE) -C $(SRCDIR) clean

install deps tags test config help:
	@$(MAKE) -C $(SRCDIR) $@

.PHONY: default debug release clean install deps tags test config help
