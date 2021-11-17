# Makefile
# $Header$

include ../../Makefile.common
include ../Makefile.common

DIRS = c test 

top:
	@for i in $(DIRS); \
	do \
		(echo making in $$i...; $(MAKE) -C $$i ); \
	done;

depend:
	@for i in $(DIRS); \
	do \
		(echo depend in $$i...; $(MAKE) -C $$i depend);\
	done;

clean:
	$(RM) $(RM_OPTIONS) $(TIDY_OPTIONS)
	@for i in $(DIRS); \
	do \
		(echo clean in $$i...; $(MAKE) -C $$i clean); \
	done;

tidy:
	$(RM) $(RM_OPTIONS) $(TIDY_OPTIONS)
	@for i in $(DIRS); \
	do \
		(echo tidy in $$i...; $(MAKE) -C $$i tidy); \
	done;

#
# $Log$
#
