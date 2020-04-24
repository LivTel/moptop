# Makefile
# $Header$
include ../../../Makefile.common
include ../../../../Makefile.common

MAKE 		= make
PACKAGEDIR	= ngat/moptop/
BINDIR 		= $(MOPTOP_BIN_HOME)/java/$(PACKAGEDIR)
CONFIG_BINDIR 	= $(MOPTOP_BIN_HOME)/java
SCRIPT_BINDIR 	= $(MOPTOP_BIN_HOME)/java
SRCS 		= $(MAIN_SRCS) $(IMPL_SRCS)
MAIN_SRCS 	= MoptopConstants.java \
		Moptop.java MoptopStatus.java MoptopTCPClientConnectionThread.java \
		MoptopTCPServerConnectionThread.java MoptopTCPServer.java MoptopREBOOTQuitThread.java
IMPL_SRCS = $(BASE_IMPL_SRCS) $(CALIBRATE_IMPL_SRCS) $(EXPOSE_IMPL_SRCS) $(INTERRUPT_IMPL_SRCS) $(SETUP_IMPL_SRCS)
BASE_IMPL_SRCS		= CommandImplementation.java JMSCommandImplementation.java UnknownCommandImplementation.java \
			HardwareImplementation.java
CALIBRATE_IMPL_SRCS 	= 
EXPOSE_IMPL_SRCS	= MULTRUNImplementation.java 
INTERRUPT_IMPL_SRCS 	= ABORTImplementation.java REBOOTImplementation.java
#GET_STATUSImplementation.java 
SETUP_IMPL_SRCS		= CONFIGImplementation.java
OBJS 		= $(SRCS:%.java=$(BINDIR)/%.class)

CONFIG_SRCS	= moptop1.moptop.net.properties moptop1.moptop.properties moptop1.fits.properties

CONFIG_OBJS	= $(CONFIG_SRCS:%.properties=$(CONFIG_BINDIR)/%.properties)
SCRIPT_SRCS	= 
SCRIPT_OBJS	= $(SCRIPT_SRCS:%=$(SCRIPT_BINDIR)/%)
DIRS 		= command
#test

top: dirs $(OBJS) config scripts dirs

#
# Java classes
#
$(BINDIR)/%.class:%.java
	$(JAVAC) -sourcepath .:$(MOPTOP_SRC_HOME)/java \
		-classpath $(MOPTOP_BIN_HOME)/java:$(CLASSPATH) $(JAVAC_OPTIONS) -d $(MOPTOP_BIN_HOME)/java $<
#
# configuration files
#
config: $(CONFIG_OBJS) $(INI_OBJS)

$(CONFIG_BINDIR)/%.properties: %.properties
	$(CP) $< $@

#
# run scripts
#
scripts: $(SCRIPT_OBJS)

#
# sub dirs
#
dirs:
	@for i in $(DIRS); \
	do \
		(echo making in $$i...; cd $$i; $(MAKE) ); \
	done;

depend:
	echo no depend in ngat directory.

clean: tidy
	-$(RM) $(RM_OPTIONS) $(OBJS)
	-$(RM) $(RM_OPTIONS) $(JARLIBDIR)/$(JAR_FILE)
	@for i in $(DIRS); \
	do \
		(echo clean in $$i...; cd $$i; $(MAKE) clean); \
	done;

tidy:
	-$(RM) $(RM_OPTIONS) $(TIDY_OPTIONS)
	@for i in $(DIRS); \
	do \
		(echo tidy in $$i...; cd $$i; $(MAKE) tidy); \
	done;

backup: tidy checkin
	@for i in $(DIRS); \
	do \
		(echo backup in $$i...; cd $$i; $(MAKE) backup); \
	done;
#
# $Log$
#
