#	$NetBSD: Makefile.inc,v 1.8.8.3 2001/01/05 17:34:13 bouyer Exp $

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

