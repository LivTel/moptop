# $Header$

include ../../../Makefile.common
include ../../Makefile.common
include ../Makefile.common

INCDIR 		= $(CCD_SRC_HOME)/include
BINDIR		= $(CCD_BIN_HOME)/c/$(HOSTTYPE)
DOCSDIR 	= $(CCD_DOC_HOME)/cdocs

# Do we wish to link with CFITSIO routines?
FITSCFLAGS = -DCFITSIO=1 -I$(CFITSIOINCDIR) 
CFITSIOLIB = -L$(LT_LIB_HOME) -lcfitsio

LOGGING_CFLAGS	= -DLOGGING=10
MUTEX_CFLAGS	= -DMBSTOWCS_MUTEXED
CFLAGS 		= -g -I$(INCDIR) -I$(ANDOR_INCDIR) $(FITSCFLAGS) $(LOGGING_CFLAGS) $(MUTEX_CFLAGS) \
		$(LOG_UDP_CFLAGS) $(SHARED_LIB_CFLAGS) 
LDFLAGS		= -L$(ANDOR_LIBDIR) $(ANDOR_LIBS) $(CFITSIOLIB)
DOCFLAGS 	= -static

SRCS 		= ccd_general.c ccd_command.c ccd_setup.c ccd_temperature.c ccd_buffer.c ccd_exposure.c \
		ccd_fits_filename.c ccd_fits_header.c
HEADERS		= $(SRCS:%.c=$(INCDIR)/%.h)
OBJS 		= $(SRCS:%.c=$(BINDIR)/%.o)
DOCS 		= $(SRCS:%.c=$(DOCSDIR)/%.html)

top: shared docs

shared: $(LT_LIB_HOME)/lib$(CCD_LIBNAME).so
$(LT_LIB_HOME)/lib$(CCD_LIBNAME).so: $(OBJS)
	$(CC) $(CCSHAREDFLAG) $(LDFLAGS) $(OBJS) -o $@

$(BINDIR)/%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@  

docs: $(DOCS)

$(DOCS): $(SRCS) $(HEADERS)
	-$(CDOC) -d $(DOCSDIR) -h $(INCDIR) $(DOCFLAGS) $(SRCS)

depend:
	makedepend $(MAKEDEPENDFLAGS) -- $(CFLAGS) -- $(SRCS)

clean:
	$(RM) $(RM_OPTIONS) $(OBJS) $(LT_LIB_HOME)/lib$(FILTER_WHEEL_LIBNAME).so $(TIDY_OPTIONS)

tidy:
	$(RM) $(RM_OPTIONS) $(TIDY_OPTIONS)
	-(cd $(INCDIR); $(RM) $(RM_OPTIONS) $(TIDY_OPTIONS);)

# DO NOT DELETE

ccd_general.o: /usr/include/errno.h /usr/include/features.h
ccd_general.o: /usr/include/stdc-predef.h /usr/include/sys/cdefs.h
ccd_general.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
ccd_general.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
ccd_general.o: /usr/include/asm/errno.h /usr/include/asm-generic/errno.h
ccd_general.o: /usr/include/asm-generic/errno-base.h /usr/include/stdio.h
ccd_general.o: /usr/lib/gcc/x86_64-redhat-linux/4.8.2/include/stddef.h
ccd_general.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
ccd_general.o: /usr/include/libio.h /usr/include/_G_config.h
ccd_general.o: /usr/include/wchar.h
ccd_general.o: /usr/lib/gcc/x86_64-redhat-linux/4.8.2/include/stdarg.h
ccd_general.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
ccd_general.o: /usr/include/stdlib.h /usr/include/bits/stdlib-float.h
ccd_general.o: /usr/include/string.h /usr/include/time.h
ccd_general.o: /usr/include/bits/time.h /usr/include/unistd.h
ccd_general.o: /usr/include/bits/posix_opt.h /usr/include/bits/confname.h
ccd_general.o: /usr/include/getopt.h
ccd_general.o: /home/dev/src/moptop/ccd/include/ccd_general.h
ccd_general.o: /home/dev/src/moptop/ccd/include/ccd_buffer.h
ccd_general.o: /home/dev/src/moptop/ccd/include/ccd_command.h
ccd_general.o: /home/dev/src/moptop/ccd/include/ccd_exposure.h
ccd_general.o: /home/dev/src/moptop/ccd/include/ccd_setup.h
ccd_general.o: /home/dev/src/moptop/ccd/include/ccd_temperature.h
ccd_general.o: /home/dev/src/moptop/ccd/include/ccd_fits_header.h
ccd_general.o: /home/dev/src/cfitsio3310_x64/include/fitsio.h
ccd_general.o: /usr/lib/gcc/x86_64-redhat-linux/4.8.2/include/limits.h
ccd_general.o: /usr/lib/gcc/x86_64-redhat-linux/4.8.2/include/syslimits.h
ccd_general.o: /usr/include/limits.h /usr/include/bits/posix1_lim.h
ccd_general.o: /usr/include/bits/local_lim.h /usr/include/linux/limits.h
ccd_general.o: /usr/include/bits/posix2_lim.h
ccd_general.o: /home/dev/src/cfitsio3310_x64/include/longnam.h
ccd_command.o: /usr/include/errno.h /usr/include/features.h
ccd_command.o: /usr/include/stdc-predef.h /usr/include/sys/cdefs.h
ccd_command.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
ccd_command.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
ccd_command.o: /usr/include/asm/errno.h /usr/include/asm-generic/errno.h
ccd_command.o: /usr/include/asm-generic/errno-base.h /usr/include/pthread.h
ccd_command.o: /usr/include/endian.h /usr/include/bits/endian.h
ccd_command.o: /usr/include/sched.h /usr/include/bits/types.h
ccd_command.o: /usr/include/bits/typesizes.h
ccd_command.o: /usr/lib/gcc/x86_64-redhat-linux/4.8.2/include/stddef.h
ccd_command.o: /usr/include/time.h /usr/include/bits/time.h
ccd_command.o: /usr/include/bits/sched.h /usr/include/bits/pthreadtypes.h
ccd_command.o: /usr/include/bits/setjmp.h /usr/include/stdio.h
ccd_command.o: /usr/include/libio.h /usr/include/_G_config.h
ccd_command.o: /usr/include/wchar.h
ccd_command.o: /usr/lib/gcc/x86_64-redhat-linux/4.8.2/include/stdarg.h
ccd_command.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
ccd_command.o: /usr/include/stdlib.h /usr/include/bits/stdlib-float.h
ccd_command.o: /usr/include/string.h /usr/include/unistd.h
ccd_command.o: /usr/include/bits/posix_opt.h /usr/include/bits/confname.h
ccd_command.o: /usr/include/getopt.h /usr/local/include/atcore.h
ccd_command.o: /usr/local/include/atutility.h
ccd_command.o: /home/dev/src/log_udp/include/log_udp.h
ccd_command.o: /home/dev/src/moptop/ccd/include/ccd_general.h
ccd_command.o: /home/dev/src/moptop/ccd/include/ccd_command.h
ccd_setup.o: /usr/include/errno.h /usr/include/features.h
ccd_setup.o: /usr/include/stdc-predef.h /usr/include/sys/cdefs.h
ccd_setup.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
ccd_setup.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
ccd_setup.o: /usr/include/asm/errno.h /usr/include/asm-generic/errno.h
ccd_setup.o: /usr/include/asm-generic/errno-base.h /usr/include/stdio.h
ccd_setup.o: /usr/lib/gcc/x86_64-redhat-linux/4.8.2/include/stddef.h
ccd_setup.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
ccd_setup.o: /usr/include/libio.h /usr/include/_G_config.h
ccd_setup.o: /usr/include/wchar.h
ccd_setup.o: /usr/lib/gcc/x86_64-redhat-linux/4.8.2/include/stdarg.h
ccd_setup.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
ccd_setup.o: /usr/include/stdlib.h /usr/include/bits/stdlib-float.h
ccd_setup.o: /usr/include/string.h /usr/include/time.h
ccd_setup.o: /usr/include/bits/time.h /home/dev/src/log_udp/include/log_udp.h
ccd_setup.o: /home/dev/src/moptop/ccd/include/ccd_general.h
ccd_setup.o: /home/dev/src/moptop/ccd/include/ccd_command.h
ccd_setup.o: /home/dev/src/moptop/ccd/include/ccd_setup.h
ccd_temperature.o: /usr/include/errno.h /usr/include/features.h
ccd_temperature.o: /usr/include/stdc-predef.h /usr/include/sys/cdefs.h
ccd_temperature.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
ccd_temperature.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
ccd_temperature.o: /usr/include/asm/errno.h /usr/include/asm-generic/errno.h
ccd_temperature.o: /usr/include/asm-generic/errno-base.h /usr/include/stdio.h
ccd_temperature.o: /usr/lib/gcc/x86_64-redhat-linux/4.8.2/include/stddef.h
ccd_temperature.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
ccd_temperature.o: /usr/include/libio.h /usr/include/_G_config.h
ccd_temperature.o: /usr/include/wchar.h
ccd_temperature.o: /usr/lib/gcc/x86_64-redhat-linux/4.8.2/include/stdarg.h
ccd_temperature.o: /usr/include/bits/stdio_lim.h
ccd_temperature.o: /usr/include/bits/sys_errlist.h /usr/include/stdlib.h
ccd_temperature.o: /usr/include/bits/stdlib-float.h /usr/include/string.h
ccd_temperature.o: /home/dev/src/log_udp/include/log_udp.h
ccd_temperature.o: /home/dev/src/moptop/ccd/include/ccd_general.h
ccd_temperature.o: /home/dev/src/moptop/ccd/include/ccd_command.h
ccd_temperature.o: /home/dev/src/moptop/ccd/include/ccd_temperature.h
ccd_buffer.o: /usr/include/errno.h /usr/include/features.h
ccd_buffer.o: /usr/include/stdc-predef.h /usr/include/sys/cdefs.h
ccd_buffer.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
ccd_buffer.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
ccd_buffer.o: /usr/include/asm/errno.h /usr/include/asm-generic/errno.h
ccd_buffer.o: /usr/include/asm-generic/errno-base.h /usr/include/stdio.h
ccd_buffer.o: /usr/lib/gcc/x86_64-redhat-linux/4.8.2/include/stddef.h
ccd_buffer.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
ccd_buffer.o: /usr/include/libio.h /usr/include/_G_config.h
ccd_buffer.o: /usr/include/wchar.h
ccd_buffer.o: /usr/lib/gcc/x86_64-redhat-linux/4.8.2/include/stdarg.h
ccd_buffer.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
ccd_buffer.o: /usr/include/stdlib.h /usr/include/bits/stdlib-float.h
ccd_buffer.o: /usr/include/string.h /usr/include/time.h
ccd_buffer.o: /usr/include/bits/time.h /usr/local/include/atcore.h
ccd_buffer.o: /home/dev/src/log_udp/include/log_udp.h
ccd_buffer.o: /home/dev/src/moptop/ccd/include/ccd_general.h
ccd_buffer.o: /home/dev/src/moptop/ccd/include/ccd_command.h
ccd_buffer.o: /home/dev/src/moptop/ccd/include/ccd_buffer.h
ccd_exposure.o: /usr/include/errno.h /usr/include/features.h
ccd_exposure.o: /usr/include/stdc-predef.h /usr/include/sys/cdefs.h
ccd_exposure.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
ccd_exposure.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
ccd_exposure.o: /usr/include/asm/errno.h /usr/include/asm-generic/errno.h
ccd_exposure.o: /usr/include/asm-generic/errno-base.h /usr/include/stdio.h
ccd_exposure.o: /usr/lib/gcc/x86_64-redhat-linux/4.8.2/include/stddef.h
ccd_exposure.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
ccd_exposure.o: /usr/include/libio.h /usr/include/_G_config.h
ccd_exposure.o: /usr/include/wchar.h
ccd_exposure.o: /usr/lib/gcc/x86_64-redhat-linux/4.8.2/include/stdarg.h
ccd_exposure.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
ccd_exposure.o: /usr/include/stdlib.h /usr/include/bits/stdlib-float.h
ccd_exposure.o: /usr/include/string.h /usr/include/time.h
ccd_exposure.o: /usr/include/bits/time.h
ccd_exposure.o: /home/dev/src/log_udp/include/log_udp.h
ccd_exposure.o: /home/dev/src/moptop/ccd/include/ccd_general.h
ccd_exposure.o: /home/dev/src/moptop/ccd/include/ccd_command.h
ccd_exposure.o: /home/dev/src/moptop/ccd/include/ccd_exposure.h
ccd_fits_filename.o: /usr/include/errno.h /usr/include/features.h
ccd_fits_filename.o: /usr/include/stdc-predef.h /usr/include/sys/cdefs.h
ccd_fits_filename.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
ccd_fits_filename.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
ccd_fits_filename.o: /usr/include/asm/errno.h
ccd_fits_filename.o: /usr/include/asm-generic/errno.h
ccd_fits_filename.o: /usr/include/asm-generic/errno-base.h
ccd_fits_filename.o: /usr/include/stdio.h
ccd_fits_filename.o: /usr/lib/gcc/x86_64-redhat-linux/4.8.2/include/stddef.h
ccd_fits_filename.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
ccd_fits_filename.o: /usr/include/libio.h /usr/include/_G_config.h
ccd_fits_filename.o: /usr/include/wchar.h
ccd_fits_filename.o: /usr/lib/gcc/x86_64-redhat-linux/4.8.2/include/stdarg.h
ccd_fits_filename.o: /usr/include/bits/stdio_lim.h
ccd_fits_filename.o: /usr/include/bits/sys_errlist.h /usr/include/stdlib.h
ccd_fits_filename.o: /usr/include/bits/stdlib-float.h /usr/include/string.h
ccd_fits_filename.o: /usr/include/sys/time.h /usr/include/time.h
ccd_fits_filename.o: /usr/include/bits/time.h /usr/include/sys/select.h
ccd_fits_filename.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
ccd_fits_filename.o: /usr/include/unistd.h /usr/include/bits/posix_opt.h
ccd_fits_filename.o: /usr/include/bits/confname.h /usr/include/getopt.h
ccd_fits_filename.o: /usr/include/dirent.h /usr/include/bits/dirent.h
ccd_fits_filename.o: /usr/include/sys/types.h /usr/include/sys/stat.h
ccd_fits_filename.o: /usr/include/bits/stat.h /usr/include/fcntl.h
ccd_fits_filename.o: /usr/include/bits/fcntl.h
ccd_fits_filename.o: /usr/include/bits/fcntl-linux.h
ccd_fits_filename.o: /home/dev/src/log_udp/include/log_udp.h
ccd_fits_filename.o: /home/dev/src/moptop/ccd/include/ccd_fits_filename.h
ccd_fits_filename.o: /home/dev/src/moptop/ccd/include/ccd_general.h
ccd_fits_header.o: /usr/include/errno.h /usr/include/features.h
ccd_fits_header.o: /usr/include/stdc-predef.h /usr/include/sys/cdefs.h
ccd_fits_header.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
ccd_fits_header.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
ccd_fits_header.o: /usr/include/asm/errno.h /usr/include/asm-generic/errno.h
ccd_fits_header.o: /usr/include/asm-generic/errno-base.h /usr/include/stdio.h
ccd_fits_header.o: /usr/lib/gcc/x86_64-redhat-linux/4.8.2/include/stddef.h
ccd_fits_header.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
ccd_fits_header.o: /usr/include/libio.h /usr/include/_G_config.h
ccd_fits_header.o: /usr/include/wchar.h
ccd_fits_header.o: /usr/lib/gcc/x86_64-redhat-linux/4.8.2/include/stdarg.h
ccd_fits_header.o: /usr/include/bits/stdio_lim.h
ccd_fits_header.o: /usr/include/bits/sys_errlist.h /usr/include/stdlib.h
ccd_fits_header.o: /usr/include/bits/stdlib-float.h /usr/include/string.h
ccd_fits_header.o: /usr/include/time.h /usr/include/bits/time.h
ccd_fits_header.o: /usr/include/unistd.h /usr/include/bits/posix_opt.h
ccd_fits_header.o: /usr/include/bits/confname.h /usr/include/getopt.h
ccd_fits_header.o: /home/dev/src/cfitsio3310_x64/include/fitsio.h
ccd_fits_header.o: /usr/lib/gcc/x86_64-redhat-linux/4.8.2/include/limits.h
ccd_fits_header.o: /usr/lib/gcc/x86_64-redhat-linux/4.8.2/include/syslimits.h
ccd_fits_header.o: /usr/include/limits.h /usr/include/bits/posix1_lim.h
ccd_fits_header.o: /usr/include/bits/local_lim.h /usr/include/linux/limits.h
ccd_fits_header.o: /usr/include/bits/posix2_lim.h
ccd_fits_header.o: /home/dev/src/cfitsio3310_x64/include/longnam.h
ccd_fits_header.o: /home/dev/src/log_udp/include/log_udp.h
ccd_fits_header.o: /home/dev/src/moptop/ccd/include/ccd_fits_header.h
ccd_fits_header.o: /home/dev/src/moptop/ccd/include/ccd_general.h
