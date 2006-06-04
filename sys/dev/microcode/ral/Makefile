#	$NetBSD: Makefile,v 1.1 2006/06/04 20:30:17 rpaulo Exp $

# Based on:
# $OpenBSD: Makefile,v 1.1 2006/01/09 20:03:40 damien Exp $

NOMAN=	# defined

FIRM=	ral-rt2561 ral-rt2561s ral-rt2661

PROG=	build

all: ${FIRM}

${FIRM}: build
	${.OBJDIR}/build

cleanall:
	rm -f ${FIRM}

.include <bsd.prog.mk>
