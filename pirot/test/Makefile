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
