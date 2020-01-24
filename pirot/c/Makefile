# $Header$

include ../../../Makefile.common
include ../../Makefile.common
include ../Makefile.common

INCDIR 		= $(PIROT_SRC_HOME)/include
BINDIR		= $(PIROT_BIN_HOME)/c/$(HOSTTYPE)
DOCSDIR 	= $(PIROT_DOC_HOME)/cdocs
LIBNAME		= lib$(PIROT_HOME)

# Headers for main tiptilt program.
# Needed for tiptilt_driver.h, which defines the Tiptilt_Driver_Function_Struct to be filled in by
# pitt_driver to dynamically link libpitiptilt into the tiptilt control program
MOPTOP_INCDIR 	= $(MOPTOP_SRC_HOME)/include


LOGGING_CFLAGS	= -DLOGGING=10
MUTEX_CFLAGS	= -DMUTEXED
CFLAGS 		= -g -I$(INCDIR) -I$(MOPTOP_INCDIR) -I$(PI_INCDIR) $(LOGGING_CFLAGS) $(MUTEX_CFLAGS) \
		$(LOG_UDP_CFLAGS) $(SHARED_LIB_CFLAGS) 
LDFLAGS		= -L$(PI_LIBDIR) $(PI_LIB)
DOCFLAGS 	= -static

SRCS 		= pirot_general.c pirot_usb.c pirot_command.c pirot_setup.c pirot_move.c 
#pirot_driver.c
HEADERS		= $(SRCS:%.c=%.h)
OBJS 		= $(SRCS:%.c=$(BINDIR)/%.o)
DOCS 		= $(SRCS:%.c=$(DOCSDIR)/%.html)

top: shared docs

shared: $(LT_LIB_HOME)/$(LIBNAME).so

$(LT_LIB_HOME)/$(LIBNAME).so: $(OBJS)
	$(CC) $(CCSHAREDFLAG) $(LDFLAGS) $(OBJS) -o $@

$(BINDIR)/%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@  

docs: $(DOCS)

$(DOCS): $(SRCS)
	-$(CDOC) -d $(DOCSDIR) -h $(INCDIR) $(DOCFLAGS) $(SRCS)

depend:
	makedepend $(MAKEDEPENDFLAGS) -- $(CFLAGS) -- $(SRCS)

clean:
	$(RM) $(RM_OPTIONS) $(OBJS) $(LT_LIB_HOME)/$(LIBNAME).so $(TIDY_OPTIONS)

tidy:
	$(RM) $(RM_OPTIONS) $(TIDY_OPTIONS)
	-(cd $(INCDIR); $(RM) $(RM_OPTIONS) $(TIDY_OPTIONS);)

backup: tidy
	$(RM) $(RM_OPTIONS) $(OBJS)
# DO NOT DELETE
