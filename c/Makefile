include ../../Makefile.common
include ../Makefile.common
include ../ccd/Makefile.common
include ../filter_wheel/Makefile.common
include ../pirot/Makefile.common

BINDIR			= $(MOPTOP_BIN_HOME)/c/$(HOSTTYPE)
INCDIR 			= $(MOPTOP_SRC_HOME)/include
DOCSDIR 		= $(MOPTOP_DOC_HOME)/cdocs

DEBUG_CFLAGS		= -DMOPTOP_DEBUG=10

# log_udp library
LOG_UDP_CFLAGS		= -I$(LOG_UDP_SRC_HOME)/include
LOG_UDP_LDFLAGS		= -l$(LOG_UDP_HOME)

# command_server library
COMMAND_SERVER_CFLAGS 	= -I$(COMMANDSERVER_SRC_HOME)/include
COMMAND_SERVER_LDFLAGS 	= -lcommandserver

# ccd library
CCD_CFLAGS 		= -I$(CCD_SRC_HOME)/include
CCD_LDFLAGS 		= -l$(CCD_LIBNAME)

# filter wheel library
FILTER_WHEEL_CFLAGS 	= -I$(FILTER_WHEEL_SRC_HOME)/include
FILTER_WHEEL_LDFLAGS 	= -l$(FILTER_WHEEL_LIBNAME)

# PI rotator library
ROTATOR_CFLAGS 		= -I$(PIROT_SRC_HOME)/include -I$(PI_INCDIR)
ROTATOR_LDFLAGS 	= -l$(PIROT_LIBNAME) -L$(PI_LIBDIR) $(PI_LIB)

# CFITSIO
CFITSIO_CFLAGS		= -I$(CFITSIOINCDIR) 
CFITSIO_LDFLAGS		= -lcfitsio

# NGAT Astro MJD link informatiom
MJD_CFLAGS 		= -I${LT_SRC_HOME}/ngatastro/include
MJD_LDFLAGS		= -lngatastro

# estar_config
CONFIG_CFLAGS		= -I$(eSTAR_SRC_HOME)/config/include/
CONFIG_LDFLAGS		= -L$(eSTAR_LIB_HOME) -lestar_config

CFLAGS 			= -g -I$(INCDIR) $(DEBUG_CFLAGS) $(LOG_UDP_CFLAGS) \
				$(CCD_CFLAGS) $(FILTER_WHEEL_CFLAGS) $(ROTATOR_CFLAGS) $(COMMAND_SERVER_CFLAGS) \
				$(CFITSIO_CFLAGS) $(MJD_CFLAGS) -I$(ANDOR_INCDIR) $(CONFIG_CFLAGS)
DOCFLAGS 		= -static

EXE_SRCS		= moptop_main.c
OBJ_SRCS		= moptop_general.c moptop_config.c moptop_server.c moptop_fits_header.c moptop_command.c \
			  moptop_multrun.c

SRCS			= $(EXE_SRCS) $(OBJ_SRCS)
HEADERS			= $(OBJ_SRCS:%.c=$(INCDIR)/%.h)
EXE_OBJS		= $(EXE_SRCS:%.c=$(BINDIR)/%.o)
OBJS			= $(SRCS:%.c=$(BINDIR)/%.o)
EXES			= $(BINDIR)/moptop
DOCS 			= $(SRCS:%.c=$(DOCSDIR)/%.html)
CONFIG_SRCS		= moptop1.moptop.c.properties moptop2.moptop.c.properties
CONFIG_BINS		= $(CONFIG_SRCS:%.properties=$(BINDIR)/%.properties)

top: $(EXES) $(CONFIG_BINS) docs

$(BINDIR)/moptop: $(OBJS)
	$(CC) $^ -o $@  -L$(LT_LIB_HOME) $(COMMAND_SERVER_LDFLAGS) \
		$(CCD_LDFLAGS) $(FILTER_WHEEL_LDFLAGS) $(ROTATOR_LDFLAGS) \
		$(LOG_UDP_LDFLAGS)  $(CFITSIO_LDFLAGS) $(OBJECT_LDFLAGS) $(MJD_LDFLAGS) \
		-L$(ANDOR_LIBDIR) $(ANDOR_LIBS) \
		$(CONFIG_LDFLAGS) $(TIMELIB) $(SOCKETLIB) -lm -lc
$(BINDIR)/%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@  

$(BINDIR)/%.properties: %.properties
	$(CP) $< $@  

docs: $(DOCS)

$(DOCS): $(SRCS)
	-$(CDOC) -d $(DOCSDIR) -h $(INCDIR) $(DOCFLAGS) $(SRCS)

$(DOCS) : $(SRCS)

depend:
	makedepend $(MAKEDEPENDFLAGS) -p$(BINDIR)/ -- $(CFLAGS) -- $(SRCS)

clean:
	$(RM) $(RM_OPTIONS) $(EXES) $(OBJS) $(TIDY_OPTIONS)

tidy:
	$(RM) $(RM_OPTIONS) $(TIDY_OPTIONS)

# DO NOT DELETE

/home/dev/bin/moptop/c/x86_64-linux/moptop_main.o: /usr/include/signal.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_main.o: /usr/include/features.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_main.o: /usr/include/stdc-predef.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_main.o: /usr/include/stdio.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_main.o: /usr/include/stdlib.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_main.o: /usr/include/alloca.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_main.o: /usr/include/string.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_main.o: /usr/include/strings.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_main.o: /usr/include/time.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_main.o: /home/dev/src/log_udp/include/log_udp.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_main.o: /home/dev/src/commandserver/include/command_server.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_main.o: /home/dev/src/moptop/ccd/include/ccd_fits_filename.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_main.o: /home/dev/src/moptop/ccd/include/ccd_general.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_main.o: /home/dev/src/moptop/ccd/include/ccd_setup.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_main.o: /home/dev/src/moptop/ccd/include/ccd_temperature.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_main.o: /home/dev/src/moptop/filter_wheel/include/filter_wheel_command.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_main.o: /home/dev/src/moptop/filter_wheel/include/filter_wheel_general.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_main.o: /home/dev/src/moptop/pirot/include/pirot_general.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_main.o: /home/dev/src/moptop/pirot/include/pirot_setup.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_main.o: /home/dev/src/moptop/pirot/include/pirot_usb.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_main.o: /home/dev/src/moptop/include/moptop_general.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_main.o: /usr/include/pthread.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_main.o: /usr/include/endian.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_main.o: /usr/include/sched.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_main.o: /home/dev/src/moptop/include/moptop_fits_header.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_main.o: /home/dev/src/moptop/ccd/include/ccd_fits_header.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_main.o: /home/dev/src/cfitsio-3.47/fitsio.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_main.o: /usr/include/limits.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_main.o: /home/dev/src/cfitsio-3.47/longnam.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_main.o: /home/dev/src/moptop/include/moptop_server.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_general.o: /usr/include/errno.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_general.o: /usr/include/features.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_general.o: /usr/include/stdc-predef.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_general.o: /usr/include/pthread.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_general.o: /usr/include/endian.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_general.o: /usr/include/sched.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_general.o: /usr/include/time.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_general.o: /usr/include/stdio.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_general.o: /usr/include/stdlib.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_general.o: /usr/include/alloca.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_general.o: /usr/include/string.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_general.o: /usr/include/strings.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_general.o: /usr/include/unistd.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_general.o: /home/dev/src/log_udp/include/log_create.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_general.o: /home/dev/src/log_udp/include/log_udp.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_general.o: /home/dev/src/log_udp/include/log_general.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_general.o: /home/dev/src/commandserver/include/command_server.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_general.o: /home/dev/src/moptop/ccd/include/ccd_general.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_general.o: /home/dev/src/moptop/include/moptop_general.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_config.o: /usr/include/stdio.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_config.o: /usr/include/stdlib.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_config.o: /usr/include/alloca.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_config.o: /usr/include/features.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_config.o: /usr/include/stdc-predef.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_config.o: /usr/include/string.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_config.o: /usr/include/strings.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_config.o: /usr/include/unistd.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_config.o: /home/dev/src/estar/config/include/estar_config.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_config.o: /home/dev/src/log_udp/include/log_udp.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_config.o: /home/dev/src/moptop/include/moptop_general.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_config.o: /usr/include/pthread.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_config.o: /usr/include/endian.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_config.o: /usr/include/sched.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_config.o: /usr/include/time.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_config.o: /home/dev/src/moptop/include/moptop_config.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_server.o: /usr/include/errno.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_server.o: /usr/include/features.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_server.o: /usr/include/stdc-predef.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_server.o: /usr/include/stdio.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_server.o: /usr/include/stdlib.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_server.o: /usr/include/alloca.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_server.o: /usr/include/string.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_server.o: /usr/include/strings.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_server.o: /usr/include/time.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_server.o: /usr/include/unistd.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_server.o: /home/dev/src/log_udp/include/log_udp.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_server.o: /home/dev/src/commandserver/include/command_server.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_server.o: /home/dev/src/moptop/include/moptop_general.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_server.o: /usr/include/pthread.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_server.o: /usr/include/endian.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_server.o: /usr/include/sched.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_server.o: /home/dev/src/moptop/include/moptop_command.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_server.o: /home/dev/src/moptop/include/moptop_server.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_fits_header.o: /usr/include/stdio.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_fits_header.o: /usr/include/string.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_fits_header.o: /usr/include/strings.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_fits_header.o: /usr/include/features.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_fits_header.o: /usr/include/stdc-predef.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_fits_header.o: /usr/include/time.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_fits_header.o: /home/dev/src/log_udp/include/log_udp.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_fits_header.o: /home/dev/src/moptop/ccd/include/ccd_general.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_fits_header.o: /home/dev/src/moptop/ccd/include/ccd_fits_header.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_fits_header.o: /home/dev/src/cfitsio-3.47/fitsio.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_fits_header.o: /usr/include/stdlib.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_fits_header.o: /usr/include/alloca.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_fits_header.o: /usr/include/limits.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_fits_header.o: /home/dev/src/cfitsio-3.47/longnam.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_fits_header.o: /home/dev/src/moptop/include/moptop_fits_header.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_fits_header.o: /home/dev/src/moptop/include/moptop_general.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_fits_header.o: /usr/include/pthread.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_fits_header.o: /usr/include/endian.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_fits_header.o: /usr/include/sched.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_command.o: /usr/include/errno.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_command.o: /usr/include/features.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_command.o: /usr/include/stdc-predef.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_command.o: /usr/include/stdio.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_command.o: /usr/include/string.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_command.o: /usr/include/strings.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_command.o: /usr/include/time.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_command.o: /usr/include/unistd.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_command.o: /home/dev/src/log_udp/include/log_udp.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_command.o: /home/dev/src/moptop/ccd/include/ccd_exposure.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_command.o: /home/dev/src/moptop/ccd/include/ccd_fits_filename.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_command.o: /home/dev/src/moptop/ccd/include/ccd_fits_header.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_command.o: /home/dev/src/cfitsio-3.47/fitsio.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_command.o: /usr/include/stdlib.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_command.o: /usr/include/alloca.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_command.o: /usr/include/limits.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_command.o: /home/dev/src/cfitsio-3.47/longnam.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_command.o: /home/dev/src/moptop/ccd/include/ccd_general.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_command.o: /home/dev/src/moptop/ccd/include/ccd_setup.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_command.o: /home/dev/src/moptop/ccd/include/ccd_temperature.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_command.o: /home/dev/src/commandserver/include/command_server.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_command.o: /home/dev/src/moptop/include/moptop_fits_header.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_command.o: /home/dev/src/moptop/include/moptop_general.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_command.o: /usr/include/pthread.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_command.o: /usr/include/endian.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_command.o: /usr/include/sched.h
/home/dev/bin/moptop/c/x86_64-linux/moptop_command.o: /home/dev/src/moptop/include/moptop_server.h
