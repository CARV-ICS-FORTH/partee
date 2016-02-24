include ../../Makefile_common.mk
include ../../makefile.colors

#depended files
EXECUTABLE = test1.bin
SOURCES ?= src/test1.c
SCOOP_SOURCE = src/test1_scoop
OBJS = src/test1_scoop.o
DEPENDENTOBJS = $(OBJS) $(TPCLIBRARY) $(REGIONSLIBRARY)

LDFLAGS +=

#rules
.PHONY: all partee seq clean distclean

all: partee seq

partee: $(EXECUTABLE)

$(EXECUTABLE): $(DEPENDENTOBJS)
	@echo -e '[$(BYELLOW)LNK$(RESET)]' $<
	@$(CCOMPILER) $(DEPENDENTOBJS) $(OFLAG) $(EXECUTABLE) $(LDFLAGS)

$(SCOOP_SOURCE).c: _scoop

%.o :	%.c
	@echo -e '[$(BGREEN)GCC$(RESET)]' $<
	@$(CCOMPILER) $(CFLAGS) $< $(OFLAG) $@

_scoop:	$(SOURCES)
	@echo -e '[$(BCYAN)SCOOP$(RESET)]' $<
	@$(SCOOP) $(RUNTIMEOPT)=$(RUNTIME) $(OUTNAMEOPT)=$(SCOOP_SOURCE) $(TPCINCLUDEOPT)=${INCLUDEDIR} $(INCLUDEFLAG)${INCLUDEDIR} $^ $(EXTRAOPTS)

clean_debug:
	@echo -e '[$(BRED)RM$(RESET)]' $(DEBUG_FILES)
	@$(RM) $(DEBUG_FILES)

distclean: clean
	@echo -e '[$(BRED)RM$(RESET)]' $(EXECUTABLE)
	@$(RM) $(EXECUTABLE)

clean:
	@echo -e '[$(BRED)RM$(RESET)]' $(OBJS)
	@$(RM) $(OBJS)
	@echo -e '[$(BRED)RM$(RESET)]' $(SCOOP_SOURCE).c
	@$(RM) $(SCOOP_SOURCE).c
	@echo -e '[$(BRED)RM$(RESET)]' $(CSV_FILE)
	@$(RM) $(CSV_FILE)
	@$(RM) *.dot
	@$(MAKE) clean_debug $(NODIRECTORY)
