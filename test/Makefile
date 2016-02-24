include ../Makefile_common.mk

all: test_dirs

test_dirs:
	@for dir in $(TESTDIRS); do \
		$(MAKE) -C $$dir $(NODIRECTORIES); \
	done

clean:
	@for dir in $(TESTDIRS); do \
		$(MAKE) -C $$dir clean $(NODIRECTORIES); \
	done

distclean:
	@for dir in $(TESTDIRS); do \
		$(MAKE) -C $$dir distclean $(NODIRECTORIES); \
	done
