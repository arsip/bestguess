## -*- Mode: Makefile; -*-                                              
##
## The 'bestguess' project
##

PROGRAM= bestguess
OBJECTS= args.o table.o

default: debug

help:
	@echo "Useful makefile targets (src directory)"
	@echo "  release Builds in release mode"
	@echo "  debug   Builds in debug mode (ASAN, assertions, etc.)"
	@echo "  clean   Deletes old compilation files"
	@echo "  deps    Rebuilds dependency info in Makefile.depends"
	@echo "  tags    Rebuilds tag files e.g. for Emacs"
	@echo "  config  Prints the important settings"

debug:
	@echo "NOTE: Building in debug mode"; \
	$(MAKE) RELEASE_MODE=false $(PROGRAM)

release:
	@echo "NOTE: Building in RELEASE MODE"; \
	$(MAKE) RELEASE_MODE=true $(PROGRAM)

# -----------------------------------------------------------------------------
RELEASE_MODE ?= false
include Makefile.defines
include Makefile.depends

# Log levels: 0=confess, 1=warn, 2=info, 3=trace
LOGLEVEL ?= 1
COPT = -O2 -DNDEBUG -DLOGLEVEL=$(LOGLEVEL)

CWARNS = -Wall -Wextra \
	 -Wcast-align \
	 -Wcast-qual \
	 -Wdisabled-optimization \
	 -Wpointer-arith \
	 -Wshadow \
	 -Wsign-compare \
	 -Wundef \
	 -Wwrite-strings \
	 -Wbad-function-cast \
	 -Wmissing-prototypes \
	 -Wnested-externs \
	 -Wstrict-prototypes \
         -Wunreachable-code \
         -Wno-missing-declarations \
	 -Wno-variadic-macros

# ------------------------------------------------------------------

CFLAGS= $(CWARNS) $(COPT) -std=c99 -fPIC $(SYSCFLAGS) $(ASAN_FLAGS)

# -----------------------------------------------------------------------------

$(PROGRAM): TAGS $(OBJECTS)
	$(CC) $(CFLAGS) -o $(PROGRAM) $(PROGRAM).c $(OBJECTS) 

# ------------------------------------------------------------------

install: $(PROGRAM)
	cp "$(PROGRAM)" "$(DESTDIR)/bin/$(PROGRAM)"

# ------------------------------------------------------------------

deps:
	printf '# Automatically generated by "make deps"\n' >Makefile.depends && \
	$(CC) -MM $(OBJECTS:.o=.c) $(PROGRAM).c >>Makefile.depends

clean:
	-rm -f $(PROGRAM)
	-rm -f *.o *.gcda *.gcov *.gcno

tags TAGS: *.[ch]
	@if ! type "etags" > /dev/null 2>&1; then \
	echo "INFO: etags not found, skipping TAGS file update"; \
	else \
	  echo "INFO: running etags to update TAGS file"; \
	  etags -o TAGS *.[ch]; \
	fi

.PHONY: default debug release clean deps install help
