# Makefile
include ../Makefile.common
include ../../../../../Makefile.common
include	../../../../Makefile.common

PACKAGEDIR	= ngat/moptop/command
BINDIR 		= $(MOPTOP_BIN_HOME)/java/$(PACKAGEDIR)
SRCS 		= Command.java BooleanReplyCommand.java DoubleReplyCommand.java IntegerReplyCommand.java \
		AbortCommand.java ConfigFilterCommand.java ConfigBinCommand.java ConfigRotorspeedCommand.java \
		FitsHeaderAddCommand.java FitsHeaderClearCommand.java FitsHeaderDeleteCommand.java \
		MultbiasCommand.java MultdarkCommand.java MultrunSetupCommand.java MultrunCommand.java \
		ShutdownCommand.java \
		StatusExposureCountCommand.java StatusExposureIndexCommand.java \
		StatusExposureLengthCommand.java StatusExposureStartTimeCommand.java\
		StatusExposureMultrunCommand.java StatusExposureRunCommand.java StatusExposureStatusCommand.java \
		StatusExposureWindowCommand.java \
		StatusFilterWheelFilterCommand.java StatusFilterWheelPositionCommand.java \
		StatusFilterWheelStatusCommand.java \
		StatusRotatorPositionCommand.java StatusRotatorSpeedCommand.java StatusRotatorStatusCommand.java \
		StatusTemperatureGetCommand.java StatusTemperatureStatusCommand.java

OBJS 		= $(SRCS:%.java=$(BINDIR)/%.class)

DIRS 		= 

top: $(OBJS)
	@for i in $(DIRS); \
	do \
		(echo making in $$i...; cd $$i; $(MAKE) ); \
	done;

$(BINDIR)/%.class:%.java
	$(JAVAC) -sourcepath .:$(MOPTOP_SRC_HOME)/java \
		-classpath $(MOPTOP_BIN_HOME)/java:$(CLASSPATH) $(JAVAC_OPTIONS) $(JAVAC_VERSION_OPTIONS) -d $(MOPTOP_BIN_HOME)/java $<

clean: tidy
	-$(RM) $(RM_OPTIONS) $(OBJS)
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
