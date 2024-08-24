## -*- Mode: Makefile; -*-                                              
##
## The 'bestguess' project for command-line benchmarking
##

export PROGRAM=bestguess
export REPORTPROGRAM=bestreport
SRCDIR=src
TESTDIR=test

default: debug

debug: clean
	@$(MAKE) -C $(SRCDIR) RELEASE_MODE=false $(PROGRAM) && \
	ln -s $(SRCDIR)/$(PROGRAM) $(PROGRAM) && \
	ln -s $(SRCDIR)/$(PROGRAM) $(REPORTPROGRAM)


release: clean
	@$(MAKE) -C $(SRCDIR) RELEASE_MODE=true $(PROGRAM) && \
	ln -s $(SRCDIR)/$(PROGRAM) $(PROGRAM) && \
	ln -s $(SRCDIR)/$(PROGRAM) $(REPORTPROGRAM)


# ------------------------------------------------------------------


clean:
	-rm -f $(PROGRAM) $(REPORTPROGRAM)
	@$(MAKE) -C $(SRCDIR) clean

install deps tags config help:
	@$(MAKE) -C $(SRCDIR) $@

test:
	@$(MAKE) -C $(TESTDIR) $@

.PHONY: default debug release clean install deps tags test config help
