# $Header$
include ../../../Makefile.common
include ../../Makefile.common
include ../Makefile.common

INCDIR 		= $(FILTER_WHEEL_SRC_HOME)/include
TESTDIR 	= test
BINDIR 		= $(FILTER_WHEEL_BIN_HOME)/$(TESTDIR)/$(HOSTTYPE)
DOCSDIR 	= $(FILTER_WHEEL_DOC_HOME)/$(TESTDIR)

CFLAGS 		= -g -I$(INCDIR) $(LOG_UDP_CFLAGS)
CONFIG_LDFLAGS	= -L$(eSTAR_LIB_HOME) -lestar_config

DOCFLAGS 	= -static

SRCS 		= filter_wheel_test_move.c filter_wheel_test_get_position.c filter_wheel_test_get_filter_count.c
OBJS 		= $(SRCS:%.c=$(BINDIR)/%.o)
PROGS 		= $(SRCS:%.c=$(BINDIR)/%)
DOCS 		= $(SRCS:%.c=$(DOCSDIR)/%.html)
SCRIPT_SRCS	= 
SCRIPT_BINS	= $(SCRIPT_SRCS:%=$(BINDIR)/%)


top: $(PROGS) scripts docs

$(BINDIR)/%: $(BINDIR)/%.o
	$(CC) -o $@ $< -L$(LT_LIB_HOME) $(TIMELIB) $(SOCKETLIB) -lm -lc -l$(FILTER_WHEEL_LIBNAME) $(CONFIG_LDFLAGS)

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

filter_wheel_test_move.o: /usr/include/stdio.h
filter_wheel_test_move.o: /usr/include/x86_64-linux-gnu/bits/libc-header-start.h
filter_wheel_test_move.o: /usr/include/features.h /usr/include/stdc-predef.h
filter_wheel_test_move.o: /usr/include/x86_64-linux-gnu/sys/cdefs.h
filter_wheel_test_move.o: /usr/include/x86_64-linux-gnu/bits/wordsize.h
filter_wheel_test_move.o: /usr/include/x86_64-linux-gnu/bits/long-double.h
filter_wheel_test_move.o: /usr/include/x86_64-linux-gnu/gnu/stubs.h
filter_wheel_test_move.o: /usr/lib/gcc/x86_64-linux-gnu/7.5.0/include/stddef.h
filter_wheel_test_move.o: /usr/include/x86_64-linux-gnu/bits/types.h
filter_wheel_test_move.o: /usr/include/x86_64-linux-gnu/bits/typesizes.h
filter_wheel_test_move.o: /usr/include/x86_64-linux-gnu/bits/types/__FILE.h
filter_wheel_test_move.o: /usr/include/x86_64-linux-gnu/bits/types/FILE.h
filter_wheel_test_move.o: /usr/include/x86_64-linux-gnu/bits/libio.h
filter_wheel_test_move.o: /usr/include/x86_64-linux-gnu/bits/_G_config.h
filter_wheel_test_move.o: /usr/include/x86_64-linux-gnu/bits/types/__mbstate_t.h
filter_wheel_test_move.o: /usr/lib/gcc/x86_64-linux-gnu/7.5.0/include/stdarg.h
filter_wheel_test_move.o: /usr/include/x86_64-linux-gnu/bits/stdio_lim.h
filter_wheel_test_move.o: /usr/include/x86_64-linux-gnu/bits/sys_errlist.h
filter_wheel_test_move.o: /usr/include/string.h
filter_wheel_test_move.o: /usr/include/x86_64-linux-gnu/bits/types/locale_t.h
filter_wheel_test_move.o: /usr/include/x86_64-linux-gnu/bits/types/__locale_t.h
filter_wheel_test_move.o: /usr/include/strings.h /usr/include/time.h
filter_wheel_test_move.o: /usr/include/x86_64-linux-gnu/bits/time.h
filter_wheel_test_move.o: /usr/include/x86_64-linux-gnu/bits/types/clock_t.h
filter_wheel_test_move.o: /usr/include/x86_64-linux-gnu/bits/types/time_t.h
filter_wheel_test_move.o: /usr/include/x86_64-linux-gnu/bits/types/struct_tm.h
filter_wheel_test_move.o: /usr/include/x86_64-linux-gnu/bits/types/struct_timespec.h
filter_wheel_test_move.o: /usr/include/x86_64-linux-gnu/bits/types/clockid_t.h
filter_wheel_test_move.o: /usr/include/x86_64-linux-gnu/bits/types/timer_t.h
filter_wheel_test_move.o: /usr/include/x86_64-linux-gnu/bits/types/struct_itimerspec.h
filter_wheel_test_move.o: /home/dev/src/log_udp/include/log_udp.h
filter_wheel_test_move.o: /home/dev/src/moptop/filter_wheel/include/filter_wheel_command.h
filter_wheel_test_move.o: /home/dev/src/moptop/filter_wheel/include/filter_wheel_general.h
filter_wheel_test_get_position.o: /usr/include/stdio.h
filter_wheel_test_get_position.o: /usr/include/x86_64-linux-gnu/bits/libc-header-start.h
filter_wheel_test_get_position.o: /usr/include/features.h
filter_wheel_test_get_position.o: /usr/include/stdc-predef.h
filter_wheel_test_get_position.o: /usr/include/x86_64-linux-gnu/sys/cdefs.h
filter_wheel_test_get_position.o: /usr/include/x86_64-linux-gnu/bits/wordsize.h
filter_wheel_test_get_position.o: /usr/include/x86_64-linux-gnu/bits/long-double.h
filter_wheel_test_get_position.o: /usr/include/x86_64-linux-gnu/gnu/stubs.h
filter_wheel_test_get_position.o: /usr/lib/gcc/x86_64-linux-gnu/7.5.0/include/stddef.h
filter_wheel_test_get_position.o: /usr/include/x86_64-linux-gnu/bits/types.h
filter_wheel_test_get_position.o: /usr/include/x86_64-linux-gnu/bits/typesizes.h
filter_wheel_test_get_position.o: /usr/include/x86_64-linux-gnu/bits/types/__FILE.h
filter_wheel_test_get_position.o: /usr/include/x86_64-linux-gnu/bits/types/FILE.h
filter_wheel_test_get_position.o: /usr/include/x86_64-linux-gnu/bits/libio.h
filter_wheel_test_get_position.o: /usr/include/x86_64-linux-gnu/bits/_G_config.h
filter_wheel_test_get_position.o: /usr/include/x86_64-linux-gnu/bits/types/__mbstate_t.h
filter_wheel_test_get_position.o: /usr/lib/gcc/x86_64-linux-gnu/7.5.0/include/stdarg.h
filter_wheel_test_get_position.o: /usr/include/x86_64-linux-gnu/bits/stdio_lim.h
filter_wheel_test_get_position.o: /usr/include/x86_64-linux-gnu/bits/sys_errlist.h
filter_wheel_test_get_position.o: /usr/include/string.h
filter_wheel_test_get_position.o: /usr/include/x86_64-linux-gnu/bits/types/locale_t.h
filter_wheel_test_get_position.o: /usr/include/x86_64-linux-gnu/bits/types/__locale_t.h
filter_wheel_test_get_position.o: /usr/include/strings.h /usr/include/time.h
filter_wheel_test_get_position.o: /usr/include/x86_64-linux-gnu/bits/time.h
filter_wheel_test_get_position.o: /usr/include/x86_64-linux-gnu/bits/types/clock_t.h
filter_wheel_test_get_position.o: /usr/include/x86_64-linux-gnu/bits/types/time_t.h
filter_wheel_test_get_position.o: /usr/include/x86_64-linux-gnu/bits/types/struct_tm.h
filter_wheel_test_get_position.o: /usr/include/x86_64-linux-gnu/bits/types/struct_timespec.h
filter_wheel_test_get_position.o: /usr/include/x86_64-linux-gnu/bits/types/clockid_t.h
filter_wheel_test_get_position.o: /usr/include/x86_64-linux-gnu/bits/types/timer_t.h
filter_wheel_test_get_position.o: /usr/include/x86_64-linux-gnu/bits/types/struct_itimerspec.h
filter_wheel_test_get_position.o: /home/dev/src/log_udp/include/log_udp.h
filter_wheel_test_get_position.o: /home/dev/src/moptop/filter_wheel/include/filter_wheel_command.h
filter_wheel_test_get_position.o: /home/dev/src/moptop/filter_wheel/include/filter_wheel_general.h
