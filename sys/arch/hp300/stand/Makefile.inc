#	$NetBSD: Makefile.inc,v 1.12 2001/01/02 04:14:34 simonb Exp $

BINDIR=			/usr/mdec

release:

S?=		${.CURDIR}/../../../..

.PHONY: machine-links
beforedepend: machine-links
# ${MACHINE} then ${MACHINE_ARCH}
machine-links:
	-rm -f machine && \
		ln -s $S/arch/hp300/include machine
	-rm -f hp300 && \
		ln -s $S/arch/hp300/include hp300
CLEANFILES+=machine hp300

realall: machine-links ${PROG}

