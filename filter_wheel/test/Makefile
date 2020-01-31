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
