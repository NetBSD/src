#	$NetBSD: Make.tags.inc,v 1.8 1999/02/12 19:52:30 veego Exp $
#
#	from: @(#)Make.tags.inc	8.1 (Berkeley) 6/11/93

# Common files for "make tags".
# Included by the Makefile for each architecture.

# Put the ../sys stuff near the end so that subroutine definitions win when
# there is a struct tag with the same name (eg., vmmeter).  The real
# solution would probably be for ctags to generate "struct vmmeter" tags.

.ifmake tags
# The invocation below returns every directory that contains sources, excluding
# the arch directories. Traditionally, COMM would have been an explicit list.
# This promises to be easier to maintain, considering how often the directory
# structure of the kernel sources has been changing recently.
SYSDIR?= /sys
COMM!=	find -H ${SYSDIR} ! -path "/sys/arch*" -name "*.[ch]" | \
		sed "s@[^/]*\..*@*.\[ch\]@" | sort -t / -u
.endif
