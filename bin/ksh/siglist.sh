#!/bin/sh
#	$NetBSD: siglist.sh,v 1.10 2016/03/16 23:01:33 christos Exp $
#
# Script to generate a sorted, complete list of signals, suitable
# for inclusion in trap.c as array initializer.
#

: ${SED:=sed}

# The trap here to make up for a bug in bash (1.14.3(1)) that calls the trap

${SED} -e '/^[	 ]*#/d' -e 's/^[	 ]*\([^ 	][^ 	]*\)[	 ][	 ]*\(.*[^ 	]\)[ 	]*$/#ifdef SIG\1\
	{ .signal = SIG\1 , .name = "\1", .mess = "\2" },\
#endif/'
