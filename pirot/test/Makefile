# $Header$
include ../../../Makefile.common
include ../../Makefile.common
include ../Makefile.common

INCDIR 		= $(PIROT_SRC_HOME)/include
TESTDIR 	= test
BINDIR 		= $(PIROT_BIN_HOME)/$(TESTDIR)/$(HOSTTYPE)
DOCSDIR 	= $(PIROT_DOC_HOME)/$(TESTDIR)

CFLAGS 		= -g -I$(INCDIR) -I$(PI_INCDIR) $(LOG_UDP_CFLAGS)

DOCFLAGS 	= -static

SRCS 		= test_command.c test_mov.c test_query_pos.c test_query_on_target.c test_setup_startup.c
OBJS 		= $(SRCS:%.c=$(BINDIR)/%.o)
PROGS 		= $(SRCS:%.c=$(BINDIR)/%)
DOCS 		= $(SRCS:%.c=$(DOCSDIR)/%.html)
SCRIPT_SRCS	= 
SCRIPT_BINS	= $(SCRIPT_SRCS:%=$(BINDIR)/%)

top: $(PROGS) scripts docs

$(BINDIR)/%: $(BINDIR)/%.o
	$(CC) -o $@ $< -L$(LT_LIB_HOME) -L$(PI_LIBDIR) $(TIMELIB) $(SOCKETLIB) -lm -lc -l$(PIROT_LIBNAME) $(PI_LIB)

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

test_command.o: /usr/include/stdio.h /usr/include/features.h
test_command.o: /usr/include/stdc-predef.h /usr/include/sys/cdefs.h
test_command.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
test_command.o: /usr/lib/gcc/x86_64-redhat-linux/4.8.2/include/stddef.h
test_command.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
test_command.o: /usr/include/libio.h /usr/include/_G_config.h
test_command.o: /usr/include/wchar.h
test_command.o: /usr/lib/gcc/x86_64-redhat-linux/4.8.2/include/stdarg.h
test_command.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
test_command.o: /usr/include/string.h /usr/include/xlocale.h
test_command.o: /usr/include/time.h /usr/include/bits/time.h
test_command.o: /home/dev/src/log_udp/include/log_udp.h
test_command.o: /home/dev/src/moptop/pirot/include/pirot_command.h
test_command.o: /home/dev/src/moptop/pirot/include/pirot_general.h
test_command.o: /home/dev/src/moptop/pirot/include/pirot_usb.h
test_mov.o: /usr/include/stdio.h /usr/include/features.h
test_mov.o: /usr/include/stdc-predef.h /usr/include/sys/cdefs.h
test_mov.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
test_mov.o: /usr/lib/gcc/x86_64-redhat-linux/4.8.2/include/stddef.h
test_mov.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
test_mov.o: /usr/include/libio.h /usr/include/_G_config.h
test_mov.o: /usr/include/wchar.h
test_mov.o: /usr/lib/gcc/x86_64-redhat-linux/4.8.2/include/stdarg.h
test_mov.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
test_mov.o: /usr/include/string.h /usr/include/xlocale.h /usr/include/time.h
test_mov.o: /usr/include/bits/time.h /home/dev/src/log_udp/include/log_udp.h
test_mov.o: /home/dev/src/moptop/pirot/include/pirot_command.h
test_mov.o: /home/dev/src/moptop/pirot/include/pirot_general.h
test_mov.o: /home/dev/src/moptop/pirot/include/pirot_usb.h
test_query_pos.o: /usr/include/stdio.h /usr/include/features.h
test_query_pos.o: /usr/include/stdc-predef.h /usr/include/sys/cdefs.h
test_query_pos.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
test_query_pos.o: /usr/lib/gcc/x86_64-redhat-linux/4.8.2/include/stddef.h
test_query_pos.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
test_query_pos.o: /usr/include/libio.h /usr/include/_G_config.h
test_query_pos.o: /usr/include/wchar.h
test_query_pos.o: /usr/lib/gcc/x86_64-redhat-linux/4.8.2/include/stdarg.h
test_query_pos.o: /usr/include/bits/stdio_lim.h
test_query_pos.o: /usr/include/bits/sys_errlist.h /usr/include/string.h
test_query_pos.o: /usr/include/xlocale.h /usr/include/time.h
test_query_pos.o: /usr/include/bits/time.h
test_query_pos.o: /home/dev/src/log_udp/include/log_udp.h
test_query_pos.o: /home/dev/src/moptop/pirot/include/pirot_command.h
test_query_pos.o: /home/dev/src/moptop/pirot/include/pirot_general.h
test_query_pos.o: /home/dev/src/moptop/pirot/include/pirot_usb.h
test_query_on_target.o: /usr/include/stdio.h /usr/include/features.h
test_query_on_target.o: /usr/include/stdc-predef.h /usr/include/sys/cdefs.h
test_query_on_target.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
test_query_on_target.o: /usr/lib/gcc/x86_64-redhat-linux/4.8.2/include/stddef.h
test_query_on_target.o: /usr/include/bits/types.h
test_query_on_target.o: /usr/include/bits/typesizes.h /usr/include/libio.h
test_query_on_target.o: /usr/include/_G_config.h /usr/include/wchar.h
test_query_on_target.o: /usr/lib/gcc/x86_64-redhat-linux/4.8.2/include/stdarg.h
test_query_on_target.o: /usr/include/bits/stdio_lim.h
test_query_on_target.o: /usr/include/bits/sys_errlist.h /usr/include/string.h
test_query_on_target.o: /usr/include/xlocale.h /usr/include/time.h
test_query_on_target.o: /usr/include/bits/time.h
test_query_on_target.o: /home/dev/src/log_udp/include/log_udp.h
test_query_on_target.o: /home/dev/src/moptop/pirot/include/pirot_command.h
test_query_on_target.o: /home/dev/src/moptop/pirot/include/pirot_general.h
test_query_on_target.o: /home/dev/src/moptop/pirot/include/pirot_usb.h
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
test_setup_startup.o: /home/dev/src/moptop/pirot/include/pirot_command.h
test_setup_startup.o: /home/dev/src/moptop/pirot/include/pirot_general.h
test_setup_startup.o: /home/dev/src/moptop/pirot/include/pirot_setup.h
test_setup_startup.o: /home/dev/src/moptop/pirot/include/pirot_usb.h
