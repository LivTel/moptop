# $Header$

include ../../../Makefile.common
include ../../Makefile.common
include ../Makefile.common

INCDIR 		= $(FILTER_WHEEL_SRC_HOME)/include
BINDIR		= $(FILTER_WHEEL_BIN_HOME)/c/$(HOSTTYPE)
DOCSDIR 	= $(FILTER_WHEEL_DOC_HOME)/cdocs

LOGGING_CFLAGS	= -DLOGGING=10
MUTEX_CFLAGS	= -DMUTEXED
CFLAGS 		= -g -I$(INCDIR) $(LOGGING_CFLAGS) $(MUTEX_CFLAGS) $(LOG_UDP_CFLAGS) $(SHARED_LIB_CFLAGS) 
LDFLAGS		= 
DOCFLAGS 	= -static

SRCS 		= filter_wheel_general.c filter_wheel_command.c
HEADERS		= $(SRCS:%.c=%.h)
OBJS 		= $(SRCS:%.c=$(BINDIR)/%.o)
DOCS 		= $(SRCS:%.c=$(DOCSDIR)/%.html)

top: shared docs

shared: $(LT_LIB_HOME)/lib$(FILTER_WHEEL_LIBNAME).so
$(LT_LIB_HOME)/lib$(FILTER_WHEEL_LIBNAME).so: $(OBJS)
	$(CC) $(CCSHAREDFLAG) $(LDFLAGS) $(OBJS) -o $@

$(BINDIR)/%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@  

docs: $(DOCS)

$(DOCS): $(SRCS)
	-$(CDOC) -d $(DOCSDIR) -h $(INCDIR) $(DOCFLAGS) $(SRCS)

depend:
	makedepend $(MAKEDEPENDFLAGS) -- $(CFLAGS) -- $(SRCS)

clean:
	$(RM) $(RM_OPTIONS) $(OBJS) $(LT_LIB_HOME)/lib$(FILTER_WHEEL_LIBNAME).so $(TIDY_OPTIONS)

tidy:
	$(RM) $(RM_OPTIONS) $(TIDY_OPTIONS)
	-(cd $(INCDIR); $(RM) $(RM_OPTIONS) $(TIDY_OPTIONS);)

# DO NOT DELETE

filter_wheel_general.o: /usr/include/errno.h /usr/include/features.h
filter_wheel_general.o: /usr/include/stdc-predef.h /usr/include/sys/cdefs.h
filter_wheel_general.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
filter_wheel_general.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
filter_wheel_general.o: /usr/include/asm/errno.h
filter_wheel_general.o: /usr/include/asm-generic/errno.h
filter_wheel_general.o: /usr/include/asm-generic/errno-base.h
filter_wheel_general.o: /usr/include/stdio.h
filter_wheel_general.o: /usr/lib/gcc/x86_64-redhat-linux/4.8.2/include/stddef.h
filter_wheel_general.o: /usr/include/bits/types.h
filter_wheel_general.o: /usr/include/bits/typesizes.h /usr/include/libio.h
filter_wheel_general.o: /usr/include/_G_config.h /usr/include/wchar.h
filter_wheel_general.o: /usr/lib/gcc/x86_64-redhat-linux/4.8.2/include/stdarg.h
filter_wheel_general.o: /usr/include/bits/stdio_lim.h
filter_wheel_general.o: /usr/include/bits/sys_errlist.h /usr/include/stdlib.h
filter_wheel_general.o: /usr/include/bits/stdlib-float.h
filter_wheel_general.o: /usr/include/string.h /usr/include/time.h
filter_wheel_general.o: /usr/include/bits/time.h /usr/include/unistd.h
filter_wheel_general.o: /usr/include/bits/posix_opt.h
filter_wheel_general.o: /usr/include/bits/confname.h /usr/include/getopt.h
filter_wheel_general.o: /usr/include/pthread.h /usr/include/endian.h
filter_wheel_general.o: /usr/include/bits/endian.h /usr/include/sched.h
filter_wheel_general.o: /usr/include/bits/sched.h
filter_wheel_general.o: /usr/include/bits/pthreadtypes.h
filter_wheel_general.o: /usr/include/bits/setjmp.h
filter_wheel_general.o: /home/dev/src/moptop/filter_wheel/include/filter_wheel_general.h
filter_wheel_general.o: /home/dev/src/moptop/filter_wheel/include/filter_wheel_command.h
filter_wheel_command.o: /usr/include/errno.h /usr/include/features.h
filter_wheel_command.o: /usr/include/stdc-predef.h /usr/include/sys/cdefs.h
filter_wheel_command.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
filter_wheel_command.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
filter_wheel_command.o: /usr/include/asm/errno.h
filter_wheel_command.o: /usr/include/asm-generic/errno.h
filter_wheel_command.o: /usr/include/asm-generic/errno-base.h
filter_wheel_command.o: /usr/include/fcntl.h /usr/include/bits/types.h
filter_wheel_command.o: /usr/include/bits/typesizes.h
filter_wheel_command.o: /usr/include/bits/fcntl.h
filter_wheel_command.o: /usr/include/bits/fcntl-linux.h /usr/include/stdio.h
filter_wheel_command.o: /usr/lib/gcc/x86_64-redhat-linux/4.8.2/include/stddef.h
filter_wheel_command.o: /usr/include/libio.h /usr/include/_G_config.h
filter_wheel_command.o: /usr/include/wchar.h
filter_wheel_command.o: /usr/lib/gcc/x86_64-redhat-linux/4.8.2/include/stdarg.h
filter_wheel_command.o: /usr/include/bits/stdio_lim.h
filter_wheel_command.o: /usr/include/bits/sys_errlist.h /usr/include/stdlib.h
filter_wheel_command.o: /usr/include/bits/stdlib-float.h
filter_wheel_command.o: /usr/include/string.h /usr/include/time.h
filter_wheel_command.o: /usr/include/bits/time.h /usr/include/unistd.h
filter_wheel_command.o: /usr/include/bits/posix_opt.h
filter_wheel_command.o: /usr/include/bits/confname.h /usr/include/getopt.h
filter_wheel_command.o: /usr/include/sys/ioctl.h /usr/include/bits/ioctls.h
filter_wheel_command.o: /usr/include/asm/ioctls.h
filter_wheel_command.o: /usr/include/asm-generic/ioctls.h
filter_wheel_command.o: /usr/include/linux/ioctl.h /usr/include/asm/ioctl.h
filter_wheel_command.o: /usr/include/asm-generic/ioctl.h
filter_wheel_command.o: /usr/include/bits/ioctl-types.h
filter_wheel_command.o: /usr/include/sys/ttydefaults.h
filter_wheel_command.o: /usr/include/sys/stat.h /usr/include/bits/stat.h
filter_wheel_command.o: /usr/include/sys/types.h /usr/include/linux/hidraw.h
filter_wheel_command.o: /usr/include/linux/hid.h /usr/include/linux/types.h
filter_wheel_command.o: /usr/include/asm/types.h
filter_wheel_command.o: /usr/include/asm-generic/types.h
filter_wheel_command.o: /usr/include/asm-generic/int-ll64.h
filter_wheel_command.o: /usr/include/asm/bitsperlong.h
filter_wheel_command.o: /usr/include/asm-generic/bitsperlong.h
filter_wheel_command.o: /usr/include/linux/posix_types.h
filter_wheel_command.o: /usr/include/linux/stddef.h
filter_wheel_command.o: /usr/include/asm/posix_types.h
filter_wheel_command.o: /usr/include/asm/posix_types_64.h
filter_wheel_command.o: /usr/include/asm-generic/posix_types.h
filter_wheel_command.o: /home/dev/src/log_udp/include/log_udp.h
filter_wheel_command.o: /home/dev/src/moptop/filter_wheel/include/filter_wheel_general.h
filter_wheel_command.o: /home/dev/src/moptop/filter_wheel/include/filter_wheel_command.h
