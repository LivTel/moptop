# $Header$
include ../../../Makefile.common
include ../../Makefile.common
include ../Makefile.common

INCDIR 		= $(CCD_SRC_HOME)/include
TESTDIR 	= test
BINDIR 		= $(CCD_BIN_HOME)/$(TESTDIR)/$(HOSTTYPE)
DOCSDIR 	= $(CCD_DOC_HOME)/$(TESTDIR)

LOGGING_CFLAGS	= -DLOGGING=10
#MUTEX_CFLAGS	= -DMBSTOWCS_MUTEXED
# -I$(ANDOR_INCDIR) $(MUTEX_CFLAGS) 
CFLAGS 		= -g -I$(INCDIR) $(LOGGING_CFLAGS) $(LOG_UDP_CFLAGS) $(SHARED_LIB_CFLAGS) 
LDFLAGS		= -L$(ANDOR_LIBDIR) $(ANDOR_LIBS)
DOCFLAGS 	= -static

SRCS 		= test_setup_startup.c test_temperature_control.c test_mbsrtowcs.c test_mbstowcs.c
OBJS 		= $(SRCS:%.c=$(BINDIR)/%.o)
PROGS 		= $(SRCS:%.c=$(BINDIR)/%)
DOCS 		= $(SRCS:%.c=$(DOCSDIR)/%.html)
SCRIPT_SRCS	= 
SCRIPT_BINS	= $(SCRIPT_SRCS:%=$(BINDIR)/%)

top: $(PROGS) scripts docs


$(BINDIR)/%: $(BINDIR)/%.o
	$(CC) -o $@ $< -L$(LT_LIB_HOME) $(TIMELIB) $(SOCKETLIB) -lm -lc -l$(CCD_LIBNAME) $(LD_FLAGS)

$(BINDIR)/%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@  

scripts: $(SCRIPT_BINS)

$(BINDIR)/%.csh:%.csh
	$(CP) $< $@

$(BINDIR)/%:%
	$(CP) $< $@

docs: $(DOCS)

$(DOCS): $(SRCS)
	-$(CDOC) -d $(DOCSDIR) -h $(INCDIR) $(DOCFLAGS) $(SRCS)

depend:
	makedepend $(MAKEDEPENDFLAGS) -- $(CFLAGS) -- $(SRCS)

clean:
	$(RM) $(RM_OPTIONS) $(OBJS) $(PROGS) $(TIDY_OPTIONS)

tidy:
	$(RM) $(RM_OPTIONS) $(TIDY_OPTIONS)

# DO NOT DELETE

test_setup_startup.o: /usr/include/stdio.h /usr/include/features.h
test_setup_startup.o: /usr/include/stdc-predef.h /usr/include/sys/cdefs.h
test_setup_startup.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
test_setup_startup.o: /usr/lib/gcc/x86_64-redhat-linux/4.8.2/include/stddef.h
test_setup_startup.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
test_setup_startup.o: /usr/include/libio.h /usr/include/_G_config.h
test_setup_startup.o: /usr/include/wchar.h
test_setup_startup.o: /usr/lib/gcc/x86_64-redhat-linux/4.8.2/include/stdarg.h
test_setup_startup.o: /usr/include/bits/stdio_lim.h
test_setup_startup.o: /usr/include/bits/sys_errlist.h /usr/include/string.h
test_setup_startup.o: /usr/include/xlocale.h /usr/include/time.h
test_setup_startup.o: /usr/include/bits/time.h
test_setup_startup.o: /home/dev/src/log_udp/include/log_udp.h
test_setup_startup.o: /home/dev/src/moptop/ccd/include/ccd_general.h
test_setup_startup.o: /home/dev/src/moptop/ccd/include/ccd_setup.h
test_temperature_control.o: /usr/include/stdio.h /usr/include/features.h
test_temperature_control.o: /usr/include/stdc-predef.h
test_temperature_control.o: /usr/include/sys/cdefs.h
test_temperature_control.o: /usr/include/bits/wordsize.h
test_temperature_control.o: /usr/include/gnu/stubs.h
test_temperature_control.o: /usr/lib/gcc/x86_64-redhat-linux/4.8.2/include/stddef.h
test_temperature_control.o: /usr/include/bits/types.h
test_temperature_control.o: /usr/include/bits/typesizes.h
test_temperature_control.o: /usr/include/libio.h /usr/include/_G_config.h
test_temperature_control.o: /usr/include/wchar.h
test_temperature_control.o: /usr/lib/gcc/x86_64-redhat-linux/4.8.2/include/stdarg.h
test_temperature_control.o: /usr/include/bits/stdio_lim.h
test_temperature_control.o: /usr/include/bits/sys_errlist.h
test_temperature_control.o: /usr/include/string.h /usr/include/xlocale.h
test_temperature_control.o: /usr/include/time.h /usr/include/bits/time.h
test_temperature_control.o: /home/dev/src/log_udp/include/log_udp.h
test_temperature_control.o: /home/dev/src/moptop/ccd/include/ccd_command.h
test_temperature_control.o: /home/dev/src/moptop/ccd/include/ccd_general.h
test_temperature_control.o: /home/dev/src/moptop/ccd/include/ccd_setup.h
test_mbsrtowcs.o: /usr/include/stdio.h /usr/include/features.h
test_mbsrtowcs.o: /usr/include/stdc-predef.h /usr/include/sys/cdefs.h
test_mbsrtowcs.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
test_mbsrtowcs.o: /usr/lib/gcc/x86_64-redhat-linux/4.8.2/include/stddef.h
test_mbsrtowcs.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
test_mbsrtowcs.o: /usr/include/libio.h /usr/include/_G_config.h
test_mbsrtowcs.o: /usr/include/wchar.h
test_mbsrtowcs.o: /usr/lib/gcc/x86_64-redhat-linux/4.8.2/include/stdarg.h
test_mbsrtowcs.o: /usr/include/bits/stdio_lim.h
test_mbsrtowcs.o: /usr/include/bits/sys_errlist.h /usr/include/stdlib.h
test_mbsrtowcs.o: /usr/include/bits/waitflags.h
test_mbsrtowcs.o: /usr/include/bits/waitstatus.h /usr/include/endian.h
test_mbsrtowcs.o: /usr/include/bits/endian.h /usr/include/bits/byteswap.h
test_mbsrtowcs.o: /usr/include/bits/byteswap-16.h /usr/include/sys/types.h
test_mbsrtowcs.o: /usr/include/time.h /usr/include/bits/time.h
test_mbsrtowcs.o: /usr/include/xlocale.h /usr/include/sys/select.h
test_mbsrtowcs.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
test_mbsrtowcs.o: /usr/include/sys/sysmacros.h
test_mbsrtowcs.o: /usr/include/bits/pthreadtypes.h /usr/include/alloca.h
test_mbsrtowcs.o: /usr/include/bits/stdlib-float.h /usr/include/string.h
test_mbstowcs.o: /usr/include/stdio.h /usr/include/features.h
test_mbstowcs.o: /usr/include/stdc-predef.h /usr/include/sys/cdefs.h
test_mbstowcs.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
test_mbstowcs.o: /usr/lib/gcc/x86_64-redhat-linux/4.8.2/include/stddef.h
test_mbstowcs.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
test_mbstowcs.o: /usr/include/libio.h /usr/include/_G_config.h
test_mbstowcs.o: /usr/include/wchar.h
test_mbstowcs.o: /usr/lib/gcc/x86_64-redhat-linux/4.8.2/include/stdarg.h
test_mbstowcs.o: /usr/include/bits/stdio_lim.h
test_mbstowcs.o: /usr/include/bits/sys_errlist.h /usr/include/stdlib.h
test_mbstowcs.o: /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h
test_mbstowcs.o: /usr/include/endian.h /usr/include/bits/endian.h
test_mbstowcs.o: /usr/include/bits/byteswap.h /usr/include/bits/byteswap-16.h
test_mbstowcs.o: /usr/include/sys/types.h /usr/include/time.h
test_mbstowcs.o: /usr/include/bits/time.h /usr/include/xlocale.h
test_mbstowcs.o: /usr/include/sys/select.h /usr/include/bits/select.h
test_mbstowcs.o: /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h
test_mbstowcs.o: /usr/include/bits/pthreadtypes.h /usr/include/alloca.h
test_mbstowcs.o: /usr/include/bits/stdlib-float.h
