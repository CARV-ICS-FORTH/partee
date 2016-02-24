# @file Makefile_common.mk
# @author Nikos Papakonstantinou <nikpapac@ics.forth.gr>

#Library path
PREFIX := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

#library directories
SRCDIR = $(PREFIX)/src
LIBDIR = $(PREFIX)/lib
INCLUDEDIR = $(PREFIX)/include
TESTDIR = $(PREFIX)/test
TESTDIRS = test1 test2 test3 test4 fibonacci

#depended files
LIBTPCOBJS = runtime.o queue.o atomic.o stats.o list.o balloc.o deps.o trie.o stack.o
TPCLIBRARY = $(LIBDIR)/libtpc.a
LIBREGIONSOBJS = regions.o profile.o
REGIONSLIBRARY = $(LIBDIR)/libregions.a
LIBHEADERS = $(INCLUDEDIR)/config.h $(INCLUDEDIR)/nesting_header.h  \
						 $(INCLUDEDIR)/queue.h $(INCLUDEDIR)/atomic.h \
						 $(INCLUDEDIR)/stats.h $(INCLUDEDIR)/thread_data.h \
						 $(INCLUDEDIR)/list.h $(INCLUDEDIR)/trie.h \
						 $(INCLUDEDIR)/stack.h $(INCLUDEDIR)/regions.h

#C compiler
CCOMPILER = gcc

#flags
BINFLAG = -c
DEBUGFLAG = -ggdb3
OFLAG = -o
WALLFLAG = -Wall -Wno-unknown-pragmas
PTHREADFLAG = -lpthread
NUMAFLAG = -lnuma
OPTIMZEFLAG = -O3
INCLUDEFLAG = -I
PROFILEFLAG = #-pg
MATHFLAG = -lm
FFTFLAG = -lfftw3
LAPACKFLAGS = -llapack -lblas
REGIONSFLAGS = -DWITH_REGIONS -Ddeletes="" -DNMEMDEBUG
ASSERTFLAG = -DNDEBUG
OMPFLAG = -fopenmp
LIKWIDFLAG = #-DLIKWID_PERFMON
LIKWIDFLAGLD = #-llikwid

CFLAGS = $(DEBUGFLAG) $(BINFLAG) $(OPTIMZEFLAG) $(INCLUDEFLAG) $(INCLUDEDIR) \
$(WALLFLAG) $(ASSERTFLAG) $(REGIONSFLAGS) $(LIKWIDFLAG) $(PROFILEFLAG)
#CFLAGS = $(BINFLAG) $(OPTIMZEFLAG) $(INCLUDEFLAG) $(INCLUDEDIR)
LDFLAGS = $(PTHREADFLAG) $(NUMAFLAG) $(LIKWIDFLAGLD) $(PROFILEFLAG)

#SCOOP Options
SCOOP = scoop
RUNTIMEOPT = --runtime
OUTNAMEOPT = --out-name
TPCINCLUDEOPT = --tpcIncludePath
EXTRAOPTS = --pragma=css
#EXTRAOPTS = --pragma=css --disable-sdam

#runtime
RUNTIME = partee

#commands
RM = rm -fr
AR = ar -ru
MKDIR = mkdir -p

# Makefile Flag
NODIRECTORY = --no-print-directory

#debug files
DEBUG_FILES = debug_file_*.log

#csv file
CSV_FILE = stats.csv
