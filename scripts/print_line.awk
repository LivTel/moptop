#!/usr/bin/awk -f
# $Header$
# input from stdin or a file.
# call as :
# print_line.awk -v LINE=1 <input file>
# to print line 1 of the file.
# Note does NOT work by default under Solaris, as /bin/awk does not support -v
# Use /usr/xpg4/bin/awk instead: i.e. : /usr/xpg4/bin/awk -f print_line.awk
# $Author: cjm $
# $Revision: 1.1 $
BEGIN {
    lineno=1
}
 {
     if(lineno == LINE )
     {
	 print $0
     }
     lineno = lineno + 1
}
#
# $Log: print_line.awk,v $
# Revision 1.1  2003/03/11 10:45:29  cjm
# Initial revision
#
#
