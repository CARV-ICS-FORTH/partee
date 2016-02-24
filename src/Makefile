include ../Makefile_common.mk
include ../makefile.colors

#rules
all: tpclib regionslib

tpclib:	$(LIBTPCOBJS)
	@$(MKDIR) $(LIBDIR)
	@echo -e '[$(BYELLOW)ARC$(RESET)]' $(TPCLIBRARY)
	@$(AR) $(TPCLIBRARY) $(LIBTPCOBJS)

regionslib:	$(LIBREGIONSOBJS)
	@$(MKDIR) $(LIBDIR)
	@echo -e '[$(BYELLOW)ARC$(RESET)]' $(REGIONSLIBRARY)
	@$(AR) $(REGIONSLIBRARY) $(LIBREGIONSOBJS)

%.o:	%.c
	@echo -e '[$(BGREEN)GCC$(RESET)]' $<
	@$(CCOMPILER) $(CFLAGS) $< $(OFLAG) $@

%.o:    $(LIBHEADERS)

clean:
	@echo -e '[$(BRED)RM$(RESET)]' $(LIBTPCOBJS)
	@$(RM) $(LIBTPCOBJS)
	@echo -e '[$(BRED)RM$(RESET)]' $(LIBREGIONSOBJS)
	@$(RM) $(LIBREGIONSOBJS)

distclean: clean
	@echo -e '[$(BRED)RM$(RESET)]' $(TPCLIBRARY)
	@$(RM) $(TPCLIBRARY)
	@echo -e '[$(BRED)RM$(RESET)]' $(REGIONSLIBRARY)
	@$(RM) $(REGIONSLIBRARY)
	@echo -e '[$(BRED)RM$(RESET)]' $(LIBDIR)
	@$(RM) $(LIBDIR)

clear:  distclean
	#@$(RM) *.*~ *~ .*.*~ .*~ $(INCLUDEDIR)/*.*~ $(INCLUDEDIR)/*~ $(INCLUDEDIR).*.*~ $(INCLUDEDIR)/.*~
