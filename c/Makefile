include ../../Makefile.common
include ../Makefile.common
include ../ccd-pco/Makefile.common
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
				$(CFITSIO_CFLAGS) $(MJD_CFLAGS) $(CONFIG_CFLAGS)
DOCFLAGS 		= -static

EXE_SRCS		= moptop_main.c
OBJ_SRCS		= moptop_general.c moptop_config.c moptop_server.c moptop_fits_header.c moptop_command.c \
			  moptop_multrun.c moptop_bias_dark.c

SRCS			= $(EXE_SRCS) $(OBJ_SRCS)
HEADERS			= $(OBJ_SRCS:%.c=$(INCDIR)/%.h)
EXE_OBJS		= $(EXE_SRCS:%.c=$(BINDIR)/%.o)
OBJS			= $(SRCS:%.c=$(BINDIR)/%.o)
EXES			= $(BINDIR)/moptop
DOCS 			= $(SRCS:%.c=$(DOCSDIR)/%.html)
CONFIG_SRCS		= moptop1.moptop.c.properties moptop2.moptop.c.properties \
			  moptop3-1.moptop.c.properties moptop3-2.moptop.c.properties
CONFIG_BINS		= $(CONFIG_SRCS:%.properties=$(BINDIR)/%.properties)

top: $(EXES) $(CONFIG_BINS) docs

$(BINDIR)/moptop: $(OBJS)
	$(CC) $^ -o $@  -L$(LT_LIB_HOME) $(COMMAND_SERVER_LDFLAGS) \
		$(CCD_LDFLAGS) $(FILTER_WHEEL_LDFLAGS) $(ROTATOR_LDFLAGS) \
		$(LOG_UDP_LDFLAGS)  $(CFITSIO_LDFLAGS) $(OBJECT_LDFLAGS) $(MJD_LDFLAGS) \
		$(PCO_LDFLAGS) \
		$(CONFIG_LDFLAGS) $(TIMELIB) $(SOCKETLIB) -lm -lc -lstdc++
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

