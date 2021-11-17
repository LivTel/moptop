# $Id$

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
CFLAGS 		= -g -I$(INCDIR) $(PCO_CFLAGS) $(FITSCFLAGS) $(LOGGING_CFLAGS) $(MUTEX_CFLAGS) \
		$(LOG_UDP_CFLAGS) $(SHARED_LIB_CFLAGS) 
LDFLAGS		= $(PCO_LDFLAGS) $(CFITSIOLIB)
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
	$(RM) $(RM_OPTIONS) $(OBJS) $(LT_LIB_HOME)/lib$(CCD_LIBNAME).so $(TIDY_OPTIONS)

tidy:
	$(RM) $(RM_OPTIONS) $(TIDY_OPTIONS)
	-(cd $(INCDIR); $(RM) $(RM_OPTIONS) $(TIDY_OPTIONS);)

