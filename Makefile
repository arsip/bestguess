## -*- Mode: Makefile; -*-                                              
##
## The 'bestguess' project for command-line benchmarking
##

PROGRAM=bestguess
SRCDIR=src
TESTDIR=test

default: debug

debug: clean
	@$(MAKE) -C $(SRCDIR) RELEASE_MODE=false $(PROGRAM) && mv $(SRCDIR)/$(PROGRAM) .

release: clean
	@$(MAKE) -C $(SRCDIR) RELEASE_MODE=true $(PROGRAM) && mv $(SRCDIR)/$(PROGRAM) .

# ------------------------------------------------------------------

clean:
	-rm -f $(PROGRAM)
	@$(MAKE) -C $(SRCDIR) clean

install:
	@$(MAKE) -C $(SRCDIR) install

deps:
	@$(MAKE) -C $(SRCDIR) deps

tags:
	@$(MAKE) -C $(SRCDIR) tags

test: 
	$(MAKE) -C $(TESTDIR) test

config:
	$(MAKE) -C $(SRCDIR) config

help:
	@$(MAKE) -C $(SRCDIR) help

.PHONY: default debug release clean install deps tags test config help
