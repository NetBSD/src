#	$NetBSD: Make.tags.inc,v 1.8.8.1 2001/04/21 17:46:26 bouyer Exp $
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
SYSDIR?= ${.CURDIR:H:H}
COMM!=	find -H ${SYSDIR} -name arch -prune -o -name "*.[ch]" -print | \
		sed 's@\(.*/\).*\..*@\1*.\[ch\]@' | sort -t / -u
.endif
