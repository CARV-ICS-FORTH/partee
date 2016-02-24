#@file Makefile
#@author Nikos Papakonstantinou <nikpapac@ics.forth.gr>

include ./Makefile_common.mk
include ./makefile.colors

.PHONY: help all lib benchs test clean clean_all distclean distclean_all

default help:
	@echo "Usage:"
	@echo "  make all             Build the previous targets (app and Apps)."
	@echo "  make lib             Build the basic library."
	@echo "  make benchs          Build all benchmarks in ./bechmark_suite directory."
	@echo "  make tests           Build all tests in ./test directory."
	@echo "  make clean           Remove object file of the basic Library."
	@echo "  make clean_all       Remove object files both from the basic library benchmarks and tests."
	@echo "  make distclean       Remove the basic library."
	@echo "  make distclean_all   Remove the basic library, benchmarks and tests."
	@echo "  make help            Prints this message."

lib: libr

libr:
	@echo -e '[$(BBLUE)MKDIR$(RESET)]' $(LIBDIR)
	@$(MAKE) -C $(SRCDIR) all $(NODIRECTORY)

tests:
	@$(MAKE) -C $(TESTDIR) all $(NODIRECTORY)

all: libr tests benchs


clean:
	@$(MAKE) -C $(SRCDIR) clean $(NODIRECTORY)

clean_all: clean
	@$(MAKE) -C $(TESTDIR) clean $(NODIRECTORY)

distclean: clean
	@$(MAKE) -C $(SRCDIR) distclean $(NODIRECTORY)

distclean_all: distclean
	@$(MAKE) -C $(TESTDIR) distclean $(NODIRECTORY)
