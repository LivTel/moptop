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

