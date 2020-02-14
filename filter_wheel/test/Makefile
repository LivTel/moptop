# $Header$
include ../../../Makefile.common
include ../../Makefile.common
include ../Makefile.common

INCDIR 		= $(FILTER_WHEEL_SRC_HOME)/include
TESTDIR 	= test
BINDIR 		= $(FILTER_WHEEL_BIN_HOME)/$(TESTDIR)/$(HOSTTYPE)
DOCSDIR 	= $(FILTER_WHEEL_DOC_HOME)/$(TESTDIR)

CFLAGS 		= -g -I$(INCDIR) $(LOG_UDP_CFLAGS)

DOCFLAGS 	= -static

SRCS 		= filter_wheel_test_move.c filter_wheel_test_get_position.c 
OBJS 		= $(SRCS:%.c=%.o)
PROGS 		= $(SRCS:%.c=$(BINDIR)/%)
DOCS 		= $(SRCS:%.c=$(DOCSDIR)/%.html)
SCRIPT_SRCS	= 
SCRIPT_BINS	= $(SCRIPT_SRCS:%=$(BINDIR)/%)


top: $(PROGS) scripts docs

$(BINDIR)/%: %.o
	$(CC) -o $@ $< -L$(LT_LIB_HOME) $(TIMELIB) $(SOCKETLIB) -lm -lc -l$(FILTER_WHEEL_LIBNAME)

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

filter_wheel_test_move.o: /usr/include/stdio.h /usr/include/features.h
filter_wheel_test_move.o: /usr/include/stdc-predef.h /usr/include/sys/cdefs.h
filter_wheel_test_move.o: /usr/include/bits/wordsize.h
filter_wheel_test_move.o: /usr/include/gnu/stubs.h
filter_wheel_test_move.o: /usr/lib/gcc/x86_64-redhat-linux/4.8.2/include/stddef.h
filter_wheel_test_move.o: /usr/include/bits/types.h
filter_wheel_test_move.o: /usr/include/bits/typesizes.h /usr/include/libio.h
filter_wheel_test_move.o: /usr/include/_G_config.h /usr/include/wchar.h
filter_wheel_test_move.o: /usr/lib/gcc/x86_64-redhat-linux/4.8.2/include/stdarg.h
filter_wheel_test_move.o: /usr/include/bits/stdio_lim.h
filter_wheel_test_move.o: /usr/include/bits/sys_errlist.h
filter_wheel_test_move.o: /usr/include/string.h /usr/include/xlocale.h
filter_wheel_test_move.o: /usr/include/time.h /usr/include/bits/time.h
filter_wheel_test_move.o: /home/dev/src/log_udp/include/log_udp.h
filter_wheel_test_move.o: /home/dev/src/moptop/filter_wheel/include/filter_wheel_command.h
filter_wheel_test_move.o: /home/dev/src/moptop/filter_wheel/include/filter_wheel_general.h
filter_wheel_test_get_position.o: /usr/include/stdio.h
filter_wheel_test_get_position.o: /usr/include/features.h
filter_wheel_test_get_position.o: /usr/include/stdc-predef.h
filter_wheel_test_get_position.o: /usr/include/sys/cdefs.h
filter_wheel_test_get_position.o: /usr/include/bits/wordsize.h
filter_wheel_test_get_position.o: /usr/include/gnu/stubs.h
filter_wheel_test_get_position.o: /usr/lib/gcc/x86_64-redhat-linux/4.8.2/include/stddef.h
filter_wheel_test_get_position.o: /usr/include/bits/types.h
filter_wheel_test_get_position.o: /usr/include/bits/typesizes.h
filter_wheel_test_get_position.o: /usr/include/libio.h
filter_wheel_test_get_position.o: /usr/include/_G_config.h
filter_wheel_test_get_position.o: /usr/include/wchar.h
filter_wheel_test_get_position.o: /usr/lib/gcc/x86_64-redhat-linux/4.8.2/include/stdarg.h
filter_wheel_test_get_position.o: /usr/include/bits/stdio_lim.h
filter_wheel_test_get_position.o: /usr/include/bits/sys_errlist.h
filter_wheel_test_get_position.o: /usr/include/string.h
filter_wheel_test_get_position.o: /usr/include/xlocale.h /usr/include/time.h
filter_wheel_test_get_position.o: /usr/include/bits/time.h
filter_wheel_test_get_position.o: /home/dev/src/log_udp/include/log_udp.h
filter_wheel_test_get_position.o: /home/dev/src/moptop/filter_wheel/include/filter_wheel_command.h
filter_wheel_test_get_position.o: /home/dev/src/moptop/filter_wheel/include/filter_wheel_general.h
