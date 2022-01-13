# $Header$

include ../../../Makefile.common
include ../../Makefile.common
include ../Makefile.common

# estar_config
CONFIG_CFLAGS	= -I$(eSTAR_SRC_HOME)/config/include/
CONFIG_LDFLAGS	= -L$(eSTAR_LIB_HOME) -lestar_config

INCDIR 		= $(FILTER_WHEEL_SRC_HOME)/include
BINDIR		= $(FILTER_WHEEL_BIN_HOME)/c/$(HOSTTYPE)
DOCSDIR 	= $(FILTER_WHEEL_DOC_HOME)/cdocs

LOGGING_CFLAGS	= -DLOGGING=10
MUTEX_CFLAGS	= -DMUTEXED
CFLAGS 		= -g -I$(INCDIR) $(LOGGING_CFLAGS) $(MUTEX_CFLAGS) $(LOG_UDP_CFLAGS) $(CONFIG_CFLAGS) \
		$(SHARED_LIB_CFLAGS) 
LDFLAGS		= $(CONFIG_LDFLAGS)
DOCFLAGS 	= -static

SRCS 		= filter_wheel_general.c filter_wheel_command.c filter_wheel_config.c
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
filter_wheel_general.o: /usr/include/stdc-predef.h
filter_wheel_general.o: /usr/include/x86_64-linux-gnu/sys/cdefs.h
filter_wheel_general.o: /usr/include/x86_64-linux-gnu/bits/wordsize.h
filter_wheel_general.o: /usr/include/x86_64-linux-gnu/bits/long-double.h
filter_wheel_general.o: /usr/include/x86_64-linux-gnu/gnu/stubs.h
filter_wheel_general.o: /usr/include/x86_64-linux-gnu/bits/errno.h
filter_wheel_general.o: /usr/include/linux/errno.h
filter_wheel_general.o: /usr/include/x86_64-linux-gnu/asm/errno.h
filter_wheel_general.o: /usr/include/asm-generic/errno.h
filter_wheel_general.o: /usr/include/asm-generic/errno-base.h
filter_wheel_general.o: /usr/include/stdio.h
filter_wheel_general.o: /usr/include/x86_64-linux-gnu/bits/libc-header-start.h
filter_wheel_general.o: /usr/lib/gcc/x86_64-linux-gnu/7.5.0/include/stddef.h
filter_wheel_general.o: /usr/include/x86_64-linux-gnu/bits/types.h
filter_wheel_general.o: /usr/include/x86_64-linux-gnu/bits/typesizes.h
filter_wheel_general.o: /usr/include/x86_64-linux-gnu/bits/types/__FILE.h
filter_wheel_general.o: /usr/include/x86_64-linux-gnu/bits/types/FILE.h
filter_wheel_general.o: /usr/include/x86_64-linux-gnu/bits/libio.h
filter_wheel_general.o: /usr/include/x86_64-linux-gnu/bits/_G_config.h
filter_wheel_general.o: /usr/include/x86_64-linux-gnu/bits/types/__mbstate_t.h
filter_wheel_general.o: /usr/lib/gcc/x86_64-linux-gnu/7.5.0/include/stdarg.h
filter_wheel_general.o: /usr/include/x86_64-linux-gnu/bits/stdio_lim.h
filter_wheel_general.o: /usr/include/x86_64-linux-gnu/bits/sys_errlist.h
filter_wheel_general.o: /usr/include/stdlib.h
filter_wheel_general.o: /usr/include/x86_64-linux-gnu/bits/floatn.h
filter_wheel_general.o: /usr/include/x86_64-linux-gnu/bits/floatn-common.h
filter_wheel_general.o: /usr/include/x86_64-linux-gnu/bits/stdlib-float.h
filter_wheel_general.o: /usr/include/string.h /usr/include/time.h
filter_wheel_general.o: /usr/include/x86_64-linux-gnu/bits/time.h
filter_wheel_general.o: /usr/include/x86_64-linux-gnu/bits/types/clock_t.h
filter_wheel_general.o: /usr/include/x86_64-linux-gnu/bits/types/time_t.h
filter_wheel_general.o: /usr/include/x86_64-linux-gnu/bits/types/struct_tm.h
filter_wheel_general.o: /usr/include/x86_64-linux-gnu/bits/types/struct_timespec.h
filter_wheel_general.o: /usr/include/x86_64-linux-gnu/bits/types/clockid_t.h
filter_wheel_general.o: /usr/include/x86_64-linux-gnu/bits/types/timer_t.h
filter_wheel_general.o: /usr/include/x86_64-linux-gnu/bits/types/struct_itimerspec.h
filter_wheel_general.o: /usr/include/unistd.h
filter_wheel_general.o: /usr/include/x86_64-linux-gnu/bits/posix_opt.h
filter_wheel_general.o: /usr/include/x86_64-linux-gnu/bits/confname.h
filter_wheel_general.o: /usr/include/x86_64-linux-gnu/bits/getopt_posix.h
filter_wheel_general.o: /usr/include/x86_64-linux-gnu/bits/getopt_core.h
filter_wheel_general.o: /usr/include/pthread.h /usr/include/endian.h
filter_wheel_general.o: /usr/include/x86_64-linux-gnu/bits/endian.h
filter_wheel_general.o: /usr/include/sched.h
filter_wheel_general.o: /usr/include/x86_64-linux-gnu/bits/sched.h
filter_wheel_general.o: /usr/include/x86_64-linux-gnu/bits/cpu-set.h
filter_wheel_general.o: /usr/include/x86_64-linux-gnu/bits/pthreadtypes.h
filter_wheel_general.o: /usr/include/x86_64-linux-gnu/bits/thread-shared-types.h
filter_wheel_general.o: /usr/include/x86_64-linux-gnu/bits/pthreadtypes-arch.h
filter_wheel_general.o: /usr/include/x86_64-linux-gnu/bits/setjmp.h
filter_wheel_general.o: /home/dev/src/moptop/filter_wheel/include/filter_wheel_general.h
filter_wheel_general.o: /home/dev/src/moptop/filter_wheel/include/filter_wheel_command.h
filter_wheel_command.o: /usr/include/errno.h /usr/include/features.h
filter_wheel_command.o: /usr/include/stdc-predef.h
filter_wheel_command.o: /usr/include/x86_64-linux-gnu/sys/cdefs.h
filter_wheel_command.o: /usr/include/x86_64-linux-gnu/bits/wordsize.h
filter_wheel_command.o: /usr/include/x86_64-linux-gnu/bits/long-double.h
filter_wheel_command.o: /usr/include/x86_64-linux-gnu/gnu/stubs.h
filter_wheel_command.o: /usr/include/x86_64-linux-gnu/bits/errno.h
filter_wheel_command.o: /usr/include/linux/errno.h
filter_wheel_command.o: /usr/include/x86_64-linux-gnu/asm/errno.h
filter_wheel_command.o: /usr/include/asm-generic/errno.h
filter_wheel_command.o: /usr/include/asm-generic/errno-base.h
filter_wheel_command.o: /usr/include/fcntl.h
filter_wheel_command.o: /usr/include/x86_64-linux-gnu/bits/types.h
filter_wheel_command.o: /usr/include/x86_64-linux-gnu/bits/typesizes.h
filter_wheel_command.o: /usr/include/x86_64-linux-gnu/bits/fcntl.h
filter_wheel_command.o: /usr/include/x86_64-linux-gnu/bits/fcntl-linux.h
filter_wheel_command.o: /usr/include/stdio.h
filter_wheel_command.o: /usr/include/x86_64-linux-gnu/bits/libc-header-start.h
filter_wheel_command.o: /usr/lib/gcc/x86_64-linux-gnu/7.5.0/include/stddef.h
filter_wheel_command.o: /usr/include/x86_64-linux-gnu/bits/types/__FILE.h
filter_wheel_command.o: /usr/include/x86_64-linux-gnu/bits/types/FILE.h
filter_wheel_command.o: /usr/include/x86_64-linux-gnu/bits/libio.h
filter_wheel_command.o: /usr/include/x86_64-linux-gnu/bits/_G_config.h
filter_wheel_command.o: /usr/include/x86_64-linux-gnu/bits/types/__mbstate_t.h
filter_wheel_command.o: /usr/lib/gcc/x86_64-linux-gnu/7.5.0/include/stdarg.h
filter_wheel_command.o: /usr/include/x86_64-linux-gnu/bits/stdio_lim.h
filter_wheel_command.o: /usr/include/x86_64-linux-gnu/bits/sys_errlist.h
filter_wheel_command.o: /usr/include/stdlib.h
filter_wheel_command.o: /usr/include/x86_64-linux-gnu/bits/floatn.h
filter_wheel_command.o: /usr/include/x86_64-linux-gnu/bits/floatn-common.h
filter_wheel_command.o: /usr/include/x86_64-linux-gnu/bits/stdlib-float.h
filter_wheel_command.o: /usr/include/string.h /usr/include/time.h
filter_wheel_command.o: /usr/include/x86_64-linux-gnu/bits/time.h
filter_wheel_command.o: /usr/include/x86_64-linux-gnu/bits/types/clock_t.h
filter_wheel_command.o: /usr/include/x86_64-linux-gnu/bits/types/time_t.h
filter_wheel_command.o: /usr/include/x86_64-linux-gnu/bits/types/struct_tm.h
filter_wheel_command.o: /usr/include/x86_64-linux-gnu/bits/types/struct_timespec.h
filter_wheel_command.o: /usr/include/x86_64-linux-gnu/bits/types/clockid_t.h
filter_wheel_command.o: /usr/include/x86_64-linux-gnu/bits/types/timer_t.h
filter_wheel_command.o: /usr/include/x86_64-linux-gnu/bits/types/struct_itimerspec.h
filter_wheel_command.o: /usr/include/unistd.h
filter_wheel_command.o: /usr/include/x86_64-linux-gnu/bits/posix_opt.h
filter_wheel_command.o: /usr/include/x86_64-linux-gnu/bits/confname.h
filter_wheel_command.o: /usr/include/x86_64-linux-gnu/bits/getopt_posix.h
filter_wheel_command.o: /usr/include/x86_64-linux-gnu/bits/getopt_core.h
filter_wheel_command.o: /usr/include/x86_64-linux-gnu/sys/ioctl.h
filter_wheel_command.o: /usr/include/x86_64-linux-gnu/bits/ioctls.h
filter_wheel_command.o: /usr/include/x86_64-linux-gnu/asm/ioctls.h
filter_wheel_command.o: /usr/include/asm-generic/ioctls.h
filter_wheel_command.o: /usr/include/linux/ioctl.h
filter_wheel_command.o: /usr/include/x86_64-linux-gnu/asm/ioctl.h
filter_wheel_command.o: /usr/include/asm-generic/ioctl.h
filter_wheel_command.o: /usr/include/x86_64-linux-gnu/bits/ioctl-types.h
filter_wheel_command.o: /usr/include/x86_64-linux-gnu/sys/ttydefaults.h
filter_wheel_command.o: /usr/include/x86_64-linux-gnu/sys/stat.h
filter_wheel_command.o: /usr/include/x86_64-linux-gnu/bits/stat.h
filter_wheel_command.o: /usr/include/x86_64-linux-gnu/sys/types.h
filter_wheel_command.o: /usr/include/x86_64-linux-gnu/bits/stdint-intn.h
filter_wheel_command.o: /usr/include/linux/hidraw.h /usr/include/linux/hid.h
filter_wheel_command.o: /usr/include/linux/types.h
filter_wheel_command.o: /usr/include/x86_64-linux-gnu/asm/types.h
filter_wheel_command.o: /usr/include/asm-generic/types.h
filter_wheel_command.o: /usr/include/asm-generic/int-ll64.h
filter_wheel_command.o: /usr/include/x86_64-linux-gnu/asm/bitsperlong.h
filter_wheel_command.o: /usr/include/asm-generic/bitsperlong.h
filter_wheel_command.o: /usr/include/linux/posix_types.h
filter_wheel_command.o: /usr/include/linux/stddef.h
filter_wheel_command.o: /usr/include/x86_64-linux-gnu/asm/posix_types.h
filter_wheel_command.o: /usr/include/x86_64-linux-gnu/asm/posix_types_64.h
filter_wheel_command.o: /usr/include/asm-generic/posix_types.h
filter_wheel_command.o: /home/dev/src/log_udp/include/log_udp.h
filter_wheel_command.o: /home/dev/src/moptop/filter_wheel/include/filter_wheel_general.h
filter_wheel_command.o: /home/dev/src/moptop/filter_wheel/include/filter_wheel_command.h
filter_wheel_config.o: /usr/include/errno.h /usr/include/features.h
filter_wheel_config.o: /usr/include/stdc-predef.h
filter_wheel_config.o: /usr/include/x86_64-linux-gnu/sys/cdefs.h
filter_wheel_config.o: /usr/include/x86_64-linux-gnu/bits/wordsize.h
filter_wheel_config.o: /usr/include/x86_64-linux-gnu/bits/long-double.h
filter_wheel_config.o: /usr/include/x86_64-linux-gnu/gnu/stubs.h
filter_wheel_config.o: /usr/include/x86_64-linux-gnu/bits/errno.h
filter_wheel_config.o: /usr/include/linux/errno.h
filter_wheel_config.o: /usr/include/x86_64-linux-gnu/asm/errno.h
filter_wheel_config.o: /usr/include/asm-generic/errno.h
filter_wheel_config.o: /usr/include/asm-generic/errno-base.h
filter_wheel_config.o: /usr/include/stdio.h
filter_wheel_config.o: /usr/include/x86_64-linux-gnu/bits/libc-header-start.h
filter_wheel_config.o: /usr/lib/gcc/x86_64-linux-gnu/7.5.0/include/stddef.h
filter_wheel_config.o: /usr/include/x86_64-linux-gnu/bits/types.h
filter_wheel_config.o: /usr/include/x86_64-linux-gnu/bits/typesizes.h
filter_wheel_config.o: /usr/include/x86_64-linux-gnu/bits/types/__FILE.h
filter_wheel_config.o: /usr/include/x86_64-linux-gnu/bits/types/FILE.h
filter_wheel_config.o: /usr/include/x86_64-linux-gnu/bits/libio.h
filter_wheel_config.o: /usr/include/x86_64-linux-gnu/bits/_G_config.h
filter_wheel_config.o: /usr/include/x86_64-linux-gnu/bits/types/__mbstate_t.h
filter_wheel_config.o: /usr/lib/gcc/x86_64-linux-gnu/7.5.0/include/stdarg.h
filter_wheel_config.o: /usr/include/x86_64-linux-gnu/bits/stdio_lim.h
filter_wheel_config.o: /usr/include/x86_64-linux-gnu/bits/sys_errlist.h
filter_wheel_config.o: /usr/include/stdlib.h
filter_wheel_config.o: /usr/include/x86_64-linux-gnu/bits/floatn.h
filter_wheel_config.o: /usr/include/x86_64-linux-gnu/bits/floatn-common.h
filter_wheel_config.o: /usr/include/x86_64-linux-gnu/bits/stdlib-float.h
filter_wheel_config.o: /usr/include/string.h /usr/include/time.h
filter_wheel_config.o: /usr/include/x86_64-linux-gnu/bits/time.h
filter_wheel_config.o: /usr/include/x86_64-linux-gnu/bits/types/clock_t.h
filter_wheel_config.o: /usr/include/x86_64-linux-gnu/bits/types/time_t.h
filter_wheel_config.o: /usr/include/x86_64-linux-gnu/bits/types/struct_tm.h
filter_wheel_config.o: /usr/include/x86_64-linux-gnu/bits/types/struct_timespec.h
filter_wheel_config.o: /usr/include/x86_64-linux-gnu/bits/types/clockid_t.h
filter_wheel_config.o: /usr/include/x86_64-linux-gnu/bits/types/timer_t.h
filter_wheel_config.o: /usr/include/x86_64-linux-gnu/bits/types/struct_itimerspec.h
filter_wheel_config.o: /home/dev/src/log_udp/include/log_udp.h
filter_wheel_config.o: /home/dev/src/moptop/filter_wheel/include/filter_wheel_general.h
filter_wheel_config.o: /home/dev/src/moptop/filter_wheel/include/filter_wheel_command.h
filter_wheel_config.o: /home/dev/src/moptop/filter_wheel/include/filter_wheel_config.h
filter_wheel_config.o: /home/dev/src/estar/config/include/estar_config.h
