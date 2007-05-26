#	$NetBSD: Make.tags.inc,v 1.13 2007/05/26 05:34:04 dyoung Exp $
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
COMM!=	find -H ${SYSDIR} \( -path '*/dist/ipf' -o -name arch \) -prune -o -type f -name "*.[ch]" \( \! -name 'usb_port.h' \! -name 'midway*' \! -name 'if_lmc.[ch]' \) -print | \
    sort -t / -u
.endif
